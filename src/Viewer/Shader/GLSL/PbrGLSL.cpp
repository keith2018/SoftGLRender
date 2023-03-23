/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


namespace SoftGL {

const char *PBR_IBL_VS = R"(
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in vec3 a_tangent;

layout (location = 0) out vec2 v_texCoord;
layout (location = 1) out vec3 v_normalVector;
layout (location = 2) out vec3 v_worldPos;
layout (location = 3) out vec3 v_cameraDirection;
layout (location = 4) out vec3 v_lightDirection;

#if defined(NORMAL_MAP)
layout (location = 5) out vec3 v_normal;
layout (location = 6) out vec3 v_tangent;
#endif

layout (binding = 0, std140) uniform UniformsModel {
    bool u_reverseZ;
    mat4 u_modelMatrix;
    mat4 u_modelViewProjectionMatrix;
    mat3 u_inverseTransposeModelMatrix;
    mat4 u_shadowMVPMatrix;
};

layout (binding = 1, std140) uniform UniformsScene {
    vec3 u_ambientColor;
    vec3 u_cameraPosition;
    vec3 u_pointLightPosition;
    vec3 u_pointLightColor;
};

void main() {
    vec4 position = vec4(a_position, 1.0);
    gl_Position = u_modelViewProjectionMatrix * position;
    v_texCoord = a_texCoord;

    // world space
    v_worldPos = vec3(u_modelMatrix * position);
    v_normalVector = mat3(u_modelMatrix) * a_normal;
    v_lightDirection = u_pointLightPosition - v_worldPos;
    v_cameraDirection = u_cameraPosition - v_worldPos;

    #if defined(NORMAL_MAP)
    vec3 N = normalize(u_inverseTransposeModelMatrix * a_normal);
    vec3 T = normalize(u_inverseTransposeModelMatrix * a_tangent);
    v_normal = N;
    v_tangent = normalize(T - dot(T, N) * N);
    #endif
}
)";

const char *PBR_IBL_FS = R"(
layout (location = 0) in vec2 v_texCoord;
layout (location = 1) in vec3 v_normalVector;
layout (location = 2) in vec3 v_worldPos;
layout (location = 3) in vec3 v_cameraDirection;
layout (location = 4) in vec3 v_lightDirection;

#if defined(NORMAL_MAP)
layout (location = 5) in vec3 v_normal;
layout (location = 6) in vec3 v_tangent;
#endif

layout (location = 0) out vec4 FragColor;

layout (binding = 0, std140) uniform UniformsModel {
    bool u_reverseZ;
    mat4 u_modelMatrix;
    mat4 u_modelViewProjectionMatrix;
    mat3 u_inverseTransposeModelMatrix;
    mat4 u_shadowMVPMatrix;
};

layout (binding = 1, std140) uniform UniformsScene {
    vec3 u_ambientColor;
    vec3 u_cameraPosition;
    vec3 u_pointLightPosition;
    vec3 u_pointLightColor;
};

layout (binding = 2, std140) uniform UniformsMaterial {
    bool u_enableLight;
    bool u_enableIBL;
    bool u_enableShadow;

    float u_kSpecular;
    vec4 u_baseColor;
};

#if defined(ALBEDO_MAP)
layout (binding = 3) uniform sampler2D u_albedoMap;
#endif

#if defined(NORMAL_MAP)
layout (binding = 4) uniform sampler2D u_normalMap;
#endif

#if defined(EMISSIVE_MAP)
layout (binding = 5) uniform sampler2D u_emissiveMap;
#endif

#if defined(AO_MAP)
layout (binding = 6) uniform sampler2D u_aoMap;
#endif

#if defined(METALROUGHNESS_MAP)
layout (binding = 7) uniform sampler2D u_metalRoughnessMap;
#endif

layout (binding = 8) uniform samplerCube u_irradianceMap;
layout (binding = 9) uniform samplerCube u_prefilterMap;

const float PI = 3.14159265359;

vec3 GetNormalFromMap() {
    #if defined(NORMAL_MAP)
    vec3 N = normalize(v_normal);
    vec3 T = normalize(v_tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(T, N);
    mat3 TBN = mat3(T, B, N);

    vec3 tangentNormal = texture(u_normalMap, v_texCoord).rgb * 2.0 - 1.0;
    return normalize(TBN * tangentNormal);
    #else
    return normalize(v_normalVector);
    #endif
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

vec3 EnvBRDFApprox(vec3 SpecularColor, float Roughness, float NdotV) {
    // [ Lazarov 2013, "Getting More Physical in Call of Duty: Black Ops II" ]
    // Adaptation to fit our G term.
    const vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1, 0.0425, 1.04, -0.04);
    vec4 r = Roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28f * NdotV)) * r.x + r.y;
    vec2 AB = vec2(-1.04f, 1.04f) * a004 + vec2(r.z, r.w);

    // Anything less than 2% is physically impossible and is instead considered to be shadowing
    // Note: this is needed for the 'specular' show flag to work, since it uses a SpecularColor of 0
    AB.y *= max(0.f, min(1.f, 50.0f * SpecularColor.g));

    return SpecularColor * AB.x + AB.y;
}

void main() {
    float pointLightRangeInverse = 1.0f / 5.f;

    #if defined(ALBEDO_MAP)
    vec4 albedo_rgba = texture(u_albedoMap, v_texCoord);
    #else
    vec4 albedo_rgba = u_baseColor;
    #endif

    vec3 albedo = pow(albedo_rgba.rgb, vec3(2.2f));

    vec4 metalRoughness = texture(u_metalRoughnessMap, v_texCoord);
    float metallic = metalRoughness.b;
    float roughness = metalRoughness.g;

    float ao = 1.f;
    #if defined(AO_MAP)
    ao = texture(u_aoMap, v_texCoord).r;
    #endif

    vec3 N = GetNormalFromMap();
    vec3 V = normalize(v_cameraDirection);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0f);

    // Light begin ---------------------------------------------------------------
    if (u_enableLight) {
        // calculate per-light radiance
        vec3 L = normalize(v_lightDirection);
        vec3 H = normalize(V + L);

        vec3 lDir = v_lightDirection * pointLightRangeInverse;
        float attenuation = clamp(1.0f - dot(lDir, lDir), 0.0f, 1.0f);
        vec3 radiance = u_pointLightColor * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

        vec3 numerator = NDF * G * F;
        // + 0.0001 to prevent divide by zero
        float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;
        kD *= 1.0f - metallic;

        float NdotL = max(dot(N, L), 0.0f);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    // Light end ---------------------------------------------------------------

    // Ambient begin ---------------------------------------------------------------
    vec3 ambient = vec3(0.f);
    if (u_enableIBL) {
        vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0f), F0, roughness);

        vec3 kS = F;
        vec3 kD = 1.0f - kS;
        kD *= (1.0f - metallic);

        vec3 irradiance = texture(u_irradianceMap, N).rgb;
        vec3 diffuse = irradiance * albedo;

        // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
        const float MAX_REFLECTION_LOD = 4.0f;
        vec3 prefilteredColor = textureLod(u_prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec3 specular = prefilteredColor * EnvBRDFApprox(F, roughness, max(dot(N, V), 0.0f));
        ambient = (kD * diffuse + specular) * ao;
    } else {
        ambient = u_ambientColor * albedo * ao;
    }
    // Ambient end ---------------------------------------------------------------

    vec3 color = ambient + Lo;
    // gamma correct
    color = pow(color, vec3(1.0f / 2.2f));

    // emissive
    vec3 emissive = vec3(0.f);
    #if defined(EMISSIVE_MAP)
    emissive = texture(u_emissiveMap, v_texCoord).rgb;
    #endif

    FragColor = vec4(color + emissive, albedo_rgba.a);
}
)";

}
