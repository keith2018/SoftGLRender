/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


namespace SoftGL {

const char *BLINN_PHONG_VS = R"(
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in vec3 a_tangent;

layout (location = 0) out vec2 v_texCoord;
layout (location = 1) out vec3 v_normalVector;
layout (location = 2) out vec3 v_worldPos;
layout (location = 3) out vec3 v_cameraDirection;
layout (location = 4) out vec3 v_lightDirection;
layout (location = 5) out vec4 v_shadowFragPos;

#if defined(NORMAL_MAP)
layout (location = 6) out vec3 v_normal;
layout (location = 7) out vec3 v_tangent;
#endif

layout (binding = 0, std140) uniform UniformsModel {
    bool u_reverseZ;
    float u_pointSize;
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
    v_shadowFragPos = u_shadowMVPMatrix * position;

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

const char *BLINN_PHONG_FS = R"(
layout (location = 0) in vec2 v_texCoord;
layout (location = 1) in vec3 v_normalVector;
layout (location = 2) in vec3 v_worldPos;
layout (location = 3) in vec3 v_cameraDirection;
layout (location = 4) in vec3 v_lightDirection;
layout (location = 5) in vec4 v_shadowFragPos;

#if defined(NORMAL_MAP)
layout (location = 6) in vec3 v_normal;
layout (location = 7) in vec3 v_tangent;
#endif

layout (location = 0) out vec4 FragColor;

layout (binding = 0, std140) uniform UniformsModel {
    bool u_reverseZ;
    float u_pointSize;
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

layout (binding = 7) uniform sampler2D u_shadowMap;

const float depthBiasCoeff  = 0.00025;
const float depthBiasMin    = 0.00005;

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

float ShadowCalculation(vec4 fragPos, vec3 normal) {
    vec3 projCoords = fragPos.xyz / fragPos.w;
    float currentDepth = projCoords.z;

    #if defined(OpenGL)
    currentDepth = currentDepth * 0.5 + 0.5;
    #endif

    if (currentDepth < 0.0 || currentDepth > 1.0) {
        return 0.0;
    }

    float bias = max(depthBiasCoeff * (1.0 - dot(normal, normalize(v_lightDirection))), depthBiasMin);
    float shadow = 0.0;

    // PCF
    vec2 pixelOffset = 1.0 / textureSize(u_shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_shadowMap, projCoords.xy + vec2(x, y) * pixelOffset).r;
            if (u_reverseZ) {
                pcfDepth = 1.0 - pcfDepth;
            }
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main() {
    float pointLightRangeInverse = 1.0f / 5.f;
    float specularExponent = 128.f;

    #if defined(ALBEDO_MAP)
    vec4 baseColor = texture(u_albedoMap, v_texCoord);
    #else
    vec4 baseColor = u_baseColor;
    #endif

    vec3 N = GetNormalFromMap();

    // ambient
    float ao = 1.f;
    #if defined(AO_MAP)
    ao = texture(u_aoMap, v_texCoord).r;
    #endif
    vec3 ambientColor = baseColor.rgb * u_ambientColor * ao;
    vec3 diffuseColor = vec3(0.f);
    vec3 specularColor = vec3(0.f);
    vec3 emissiveColor = vec3(0.f);

    if (u_enableLight) {
        // diffuse
        vec3 lDir = v_lightDirection * pointLightRangeInverse;
        float attenuation = clamp(1.0f - dot(lDir, lDir), 0.0f, 1.0f);

        vec3 lightDirection = normalize(v_lightDirection);
        float diffuse = max(dot(N, lightDirection), 0.0f);
        diffuseColor = u_pointLightColor * baseColor.rgb * diffuse * attenuation;

        // specular
        vec3 cameraDirection = normalize(v_cameraDirection);
        vec3 halfVector = normalize(lightDirection + cameraDirection);
        float specularAngle = max(dot(N, halfVector), 0.0f);
        specularColor = u_kSpecular * vec3(pow(specularAngle, specularExponent));

        if (u_enableShadow) {
            // calculate shadow
            float shadow = 1.0 - ShadowCalculation(v_shadowFragPos, N);
            diffuseColor *= shadow;
            specularColor *= shadow;
        }
    }

    #if defined(EMISSIVE_MAP)
    emissiveColor = texture(u_emissiveMap, v_texCoord).rgb;
    #endif

    FragColor = vec4(ambientColor + diffuseColor + specularColor + emissiveColor, baseColor.a);
}
)";

}
