/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "../shader.h"

namespace SoftGL {

#define SkyboxShaderAttributes Vertex

struct SkyboxShaderUniforms : BaseShaderUniforms {
  // textures
  SamplerCube u_cubeMap;
  Sampler2D u_equirectangularMap;
};

struct SkyboxShaderVaryings : BaseShaderVaryings {
  glm::vec3 v_texCoord;
};

struct SkyboxVertexShader : BaseVertexShader {

  void shader_main() override {
    auto *a = (SkyboxShaderAttributes *) attributes;
    auto *u = (SkyboxShaderUniforms *) uniforms;
    auto *v = (SkyboxShaderVaryings *) varyings;

    glm::vec4 position = glm::vec4(a->a_position, 1.0f);
    gl_Position = u->u_modelViewProjectionMatrix * position;
    gl_Position.z = gl_Position.w;  // depth -> 1.0

    v->v_texCoord = a->a_position;
  }

  CLONE_VERTEX_SHADER(SkyboxVertexShader)
};

struct SkyboxFragmentShader : BaseFragmentShader {
  SkyboxShaderUniforms *u = nullptr;
  SkyboxShaderVaryings *v = nullptr;

  const glm::vec2 invAtan = glm::vec2(0.1591f, 0.3183f);
  glm::vec2 SampleSphericalMap(glm::vec3 dir) {
    glm::vec2 uv = glm::vec2(glm::atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    uv += 0.5f;
    return uv;
  }

  void shader_main() override {
    BaseFragmentShader::shader_main();
    u = (SkyboxShaderUniforms *) uniforms;
    v = (SkyboxShaderVaryings *) varyings;

    if (!u->u_equirectangularMap.Empty()) {
      glm::vec2 uv = SampleSphericalMap(normalize(v->v_texCoord));
      gl_FragColor = u->u_equirectangularMap.texture2D(uv);
    } else if (!u->u_cubeMap.Empty()) {
      gl_FragColor = u->u_cubeMap.textureCube(v->v_texCoord);
    } else {
      discard = true;
    }
  }

  CLONE_FRAGMENT_SHADER(SkyboxFragmentShader)
};

}
