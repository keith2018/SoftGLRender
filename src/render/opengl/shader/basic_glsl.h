/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

namespace SoftGL {

const char *BASIC_VS = R"(
#version 330 core
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in vec3 a_tangent;

layout (std140) uniform Uniforms
{
  mat4 u_modelViewProjectionMatrix;
  vec4 u_fragColor;
};

void main()
{
  gl_Position = u_modelViewProjectionMatrix * vec4(a_position, 1.0);
}
)";

const char *BASIC_FS = R"(
#version 330 core
out vec4 FragColor;

layout (std140) uniform Uniforms
{
  mat4 u_modelViewProjectionMatrix;
  vec4 u_fragColor;
};

void main()
{
  FragColor = u_fragColor;
}
)";

}
