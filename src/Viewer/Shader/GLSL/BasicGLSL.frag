/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

layout (location = 0) out vec4 FragColor;

layout (binding = 1, std140) uniform UniformsMaterial {
    bool u_enableLight;
    bool u_enableIBL;
    bool u_enableShadow;

    float u_pointSize;
    float u_kSpecular;
    vec4 u_baseColor;
};

void main() {
    FragColor = u_baseColor;
}
