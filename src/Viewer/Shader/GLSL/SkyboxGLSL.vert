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
