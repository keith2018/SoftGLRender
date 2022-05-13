/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "renderer/Shader.h"

namespace SoftGL {

#define IrradianceShaderAttributes Vertex

struct IrradianceShaderUniforms : BaseShaderUniforms {
  SamplerCube u_cubeMap;
};

struct IrradianceShaderVaryings : BaseShaderVaryings {
  glm::vec3 v_texCoord;
};

struct IrradianceVertexShader : BaseVertexShader {

  void shader_main() override {
    auto *a = (IrradianceShaderAttributes *) attributes;
    auto *u = (IrradianceShaderUniforms *) uniforms;
    auto *v = (IrradianceShaderVaryings *) varyings;

    glm::vec4 position = glm::vec4(a->a_position, 1.0f);
    gl_Position = u->u_modelViewProjectionMatrix * position;
    gl_Position.z = gl_Position.w;  // depth -> 1.0

    v->v_texCoord = a->a_position;
  }

  CLONE_VERTEX_SHADER(IrradianceVertexShader)
};

struct IrradianceFragmentShader : BaseFragmentShader {
  IrradianceShaderUniforms *u = nullptr;
  IrradianceShaderVaryings *v = nullptr;

  void shader_main() override {
    BaseFragmentShader::shader_main();
    u = (IrradianceShaderUniforms *) uniforms;
    v = (IrradianceShaderVaryings *) varyings;

    glm::vec3 N = normalize(v->v_texCoord);

    glm::vec3 irradiance = glm::vec3(0.0f);

    // tangent space calculation from origin point
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(up, N));
    up = glm::normalize(glm::cross(N, right));

    float sampleDelta = 0.025f;
    float nrSamples = 0.0f;
    for (float phi = 0.0f; phi < 2.0f * PI; phi += sampleDelta) {
      for (float theta = 0.0f; theta < 0.5f * PI; theta += sampleDelta) {
        // spherical to cartesian (in tangent space)
        glm::vec3 tangentSample = glm::vec3(glm::sin(theta) * glm::cos(phi), glm::sin(theta) * glm::sin(phi), glm::cos(theta));
        // tangent space to world
        glm::vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

        irradiance += glm::vec3(u->u_cubeMap.textureCube(sampleVec)) * glm::cos(theta) * glm::sin(theta);
        nrSamples++;
      }
    }
    irradiance = PI * irradiance * (1.0f / float(nrSamples));

    gl_FragColor = glm::vec4(irradiance, 1.0f);
  }

  CLONE_FRAGMENT_SHADER(IrradianceFragmentShader)
};

}
