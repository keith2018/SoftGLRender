/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "texture_soft.h"

namespace SoftGL {

class SamplerSoft {
 public:
  virtual TextureType Type() = 0;
  virtual void SetTexture(const std::shared_ptr<Texture> &tex) = 0;
};

class Sampler2DSoft : public SamplerSoft {
 public:
  TextureType Type() override {
    return TextureType_2D;
  }

  void SetTexture(const std::shared_ptr<Texture> &tex) override {
    // TODO
  }

  glm::vec4 Texture2D(glm::vec2 coord) {
    // TODO
    return {};
  }
};

class SamplerCubeSoft : public SamplerSoft {
 public:
  TextureType Type() override {
    return TextureType_CUBE;
  }

  void SetTexture(const std::shared_ptr<Texture> &tex) override {
    // TODO
  }

  glm::vec4 TextureCube(glm::vec3 coord) {
    // TODO
    return {};
  }
};

}
