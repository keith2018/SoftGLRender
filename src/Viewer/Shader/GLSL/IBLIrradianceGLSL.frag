layout (location = 0) in vec3 v_worldPos;

layout (location = 0) out vec4 FragColor;

layout (binding = 1) uniform samplerCube u_cubeMap;

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
