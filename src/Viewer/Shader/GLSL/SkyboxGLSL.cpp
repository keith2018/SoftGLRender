/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


namespace SoftGL {

const char *SKYBOX_VS = R"(
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in vec3 a_tangent;

layout (location = 0) out vec3 v_worldPos;

layout (binding = 0, std140) uniform UniformsModel {
    bool u_reverseZ;
    mat4 u_modelMatrix;
    mat4 u_modelViewProjectionMatrix;
    mat3 u_inverseTransposeModelMatrix;
    mat4 u_shadowMVPMatrix;
};

void main() {
    vec4 pos = u_modelViewProjectionMatrix * vec4(a_position, 1.0);
    gl_Position = pos.xyww;

    if (u_reverseZ) {
        gl_Position.z = 0.0;
    }

    v_worldPos = a_position;
}
)";

const char *SKYBOX_FS = R"(
layout (location = 0) in vec3 v_worldPos;

layout (location = 0) out vec4 FragColor;

#if defined(EQUIRECTANGULAR_MAP)
layout (binding = 1) uniform sampler2D u_equirectangularMap;
#else
layout (binding = 2) uniform samplerCube u_cubeMap;
#endif

vec2 SampleSphericalMap(vec3 dir) {
    vec2 uv = vec2(atan(dir.z, dir.x), asin(-dir.y));
    uv *= vec2(0.1591f, 0.3183f);
    uv += 0.5f;
    return uv;
}

void main() {
    #if defined(EQUIRECTANGULAR_MAP)
    vec2 uv = SampleSphericalMap(normalize(v_worldPos));
    FragColor = texture(u_equirectangularMap, uv);
    #else
    FragColor = texture(u_cubeMap, v_worldPos);
    #endif
}
)";

}
