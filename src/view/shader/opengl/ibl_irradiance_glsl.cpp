/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


namespace SoftGL {

const char *IBL_IRRADIANCE_VS = R"(
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in vec3 a_tangent;

out vec3 v_worldPos;

layout (std140) uniform UniformsMVP {
    mat4 u_modelMatrix;
    mat4 u_modelViewProjectionMatrix;
    mat3 u_inverseTransposeModelMatrix;
};

void main() {
    vec4 pos = u_modelViewProjectionMatrix * vec4(a_position, 1.0);
    gl_Position = pos.xyww;

    v_worldPos = a_position;
}
)";

const char *IBL_IRRADIANCE_FS = R"(
in vec3 v_worldPos;

out vec4 FragColor;

uniform samplerCube u_cubeMap;

const float PI = 3.14159265359;

void main() {
    vec3 N = normalize(v_worldPos);
    vec3 irradiance = vec3(0.0f);

    // tangent space calculation from origin point
    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sampleDelta = 0.025f;
    float nrSamples = 0.0f;
    for (float phi = 0.0f; phi < 2.0f * PI; phi += sampleDelta) {
        for (float theta = 0.0f; theta < 0.5f * PI; theta += sampleDelta) {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += texture(u_cubeMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0f / float(nrSamples));

    FragColor = vec4(irradiance, 1.0f);
}
)";

}
