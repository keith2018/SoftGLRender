/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


namespace SoftGL {

const char *PBR_IBL_VS = R"(
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;

out vec2 v_texCoord;
out vec3 v_normalVector;
out vec3 v_worldPos;

out vec3 v_cameraDirection;
out vec3 v_lightDirection;

layout (std140) uniform UniformsMVP {
  mat4 u_modelMatrix;
  mat4 u_modelViewProjectionMatrix;
  mat3 u_inverseTransposeModelMatrix;
};

layout(std140) uniform UniformsLight {
  bool u_showPointLight;
  bool u_enableIBL;

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
}
)";

const char *PBR_IBL_FS = R"(
in vec2 v_texCoord;
in vec3 v_normalVector;
in vec3 v_worldPos;

in vec3 v_cameraDirection;
in vec3 v_lightDirection;

out vec4 FragColor;

layout (std140) uniform UniformsMVP {
  mat4 u_modelMatrix;
  mat4 u_modelViewProjectionMatrix;
  mat3 u_inverseTransposeModelMatrix;
};

layout(std140) uniform UniformsLight {
  bool u_showPointLight;
  bool u_enableIBL;

  vec3 u_ambientColor;

  vec3 u_cameraPosition;
  vec3 u_pointLightPosition;
  vec3 u_pointLightColor;
};

#if defined(ALPHA_DISCARD)
layout(std140) uniform UniformsAlphaMask {
  float u_alpha_cutoff;
};
#endif

uniform sampler2D u_albedoMap;
uniform sampler2D u_metalRoughnessMap;
uniform samplerCube u_irradianceMap;
uniform samplerCube u_prefilterMap;

#if defined(NORMAL_MAP)
uniform sampler2D u_normalMap;
#endif

#if defined(EMISSIVE_MAP)
uniform sampler2D u_emissiveMap;
#endif

#if defined(AO_MAP)
uniform sampler2D u_aoMap;
#endif

const float PI = 3.14159265359;

vec3 GetNormalFromMap() {
#if defined(NORMAL_MAP)
  vec3 tangentNormal = texture(u_normalMap, v_texCoord).rgb * 2.0 - 1.0;

  vec3 Q1  = dFdx(v_worldPos);
  vec3 Q2  = dFdy(v_worldPos);
  vec2 st1 = dFdx(v_texCoord);
  vec2 st2 = dFdy(v_texCoord);

  vec3 N   = normalize(v_normalVector);
  vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
  vec3 B  = -normalize(cross(N, T));
  mat3 TBN = mat3(T, B, N);

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

  vec4 albedo_rgba = texture(u_albedoMap, v_texCoord);
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
  if (u_showPointLight) {
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

  // alpha discard
#if defined(ALPHA_DISCARD)
  if (FragColor.a < u_alpha_cutoff) {
    discard = true;
  }
#endif
}
)";

}
