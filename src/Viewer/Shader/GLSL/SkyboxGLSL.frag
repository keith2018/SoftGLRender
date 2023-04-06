/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

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
