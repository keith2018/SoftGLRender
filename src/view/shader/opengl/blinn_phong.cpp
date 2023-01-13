/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


namespace SoftGL {

const char *BLINN_PHONG_VS = R"(
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;

out vec2 v_texCoord;
out vec3 v_normalVector;
out vec3 v_cameraDirection;
out vec3 v_lightDirection;

layout (std140) uniform UniformsMVP {
  mat4 u_modelMatrix;
  mat4 u_modelViewProjectionMatrix;
  mat3 u_inverseTransposeModelMatrix;
};

layout(std140) uniform UniformsScene {
  bool u_enablePointLight;
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
  vec3 fragWorldPos = vec3(u_modelMatrix * position);
  v_normalVector = normalize(u_inverseTransposeModelMatrix * a_normal);
  v_lightDirection = u_pointLightPosition - fragWorldPos;
  v_cameraDirection = u_cameraPosition - fragWorldPos;

#if defined(NORMAL_MAP)
  // TBN
  vec3 N = normalize(u_inverseTransposeModelMatrix * a_normal);
  vec3 T = normalize(u_inverseTransposeModelMatrix * a_tangent);
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(T, N);
  mat3 TBN = transpose(mat3(T, B, N));

  // TBN space
  v_lightDirection = TBN * v_lightDirection;
  v_cameraDirection = TBN * v_cameraDirection;
#endif
}
)";

const char *BLINN_PHONG_FS = R"(
in vec2 v_texCoord;
in vec3 v_normalVector;
in vec3 v_cameraDirection;
in vec3 v_lightDirection;

out vec4 FragColor;

layout (std140) uniform UniformsMVP {
  mat4 u_modelMatrix;
  mat4 u_modelViewProjectionMatrix;
  mat3 u_inverseTransposeModelMatrix;
};

layout(std140) uniform UniformsScene {
  bool u_enablePointLight;
  bool u_enableIBL;

  vec3 u_ambientColor;
  vec3 u_cameraPosition;
  vec3 u_pointLightPosition;
  vec3 u_pointLightColor;
};

layout (std140) uniform UniformsColor {
  vec4 u_baseColor;
};

#if defined(ALBEDO_MAP)
uniform sampler2D u_albedoMap;
#endif

#if defined(NORMAL_MAP)
uniform sampler2D u_normalMap;
#endif

#if defined(EMISSIVE_MAP)
uniform sampler2D u_emissiveMap;
#endif

#if defined(AO_MAP)
uniform sampler2D u_aoMap;
#endif

vec3 GetNormalFromMap() {
#if defined(NORMAL_MAP)
  vec3 normalVector = texture(u_normalMap, v_texCoord).rgb;
  return normalize(normalVector * 2.f - 1.f);
#else
  return normalize(v_normalVector);
#endif
}

void main() {
  float pointLightRangeInverse = 1.0f / 5.f;
  float specularExponent = 128.f;

#if defined(ALBEDO_MAP)
  vec4 baseColor = texture(u_albedoMap, v_texCoord);
#else
  vec4 baseColor = u_baseColor;
#endif

  vec3 normalVector = GetNormalFromMap();

  // ambient
  float ao = 1.f;
#if defined(AO_MAP)
  ao = texture(u_aoMap, v_texCoord).r;
#endif
  vec3 ambientColor = baseColor.rgb * u_ambientColor * ao;
  vec3 diffuseColor = vec3(0.f);
  vec3 specularColor = vec3(0.f);
  vec3 emissiveColor = vec3(0.f);

  if (u_enablePointLight) {
    // diffuse
    vec3 lDir = v_lightDirection * pointLightRangeInverse;
    float attenuation = clamp(1.0f - dot(lDir, lDir), 0.0f, 1.0f);

    vec3 lightDirection = normalize(v_lightDirection);
    float diffuse = max(dot(normalVector, lightDirection), 0.0f);
    diffuseColor = u_pointLightColor * baseColor.rgb * diffuse * attenuation;

    // specular
    vec3 cameraDirection = normalize(v_cameraDirection);
    vec3 halfVector = normalize(lightDirection + cameraDirection);
    float specularAngle = max(dot(normalVector, halfVector), 0.0f);
    specularColor = vec3(pow(specularAngle, specularExponent));
  }

#if defined(EMISSIVE_MAP)
  emissiveColor = texture(u_emissiveMap, v_texCoord).rgb;
#endif

  FragColor = vec4(ambientColor + diffuseColor + specularColor + emissiveColor, baseColor.a);
}
)";

}
