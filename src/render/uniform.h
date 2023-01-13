/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include <string>
#include "texture.h"


namespace SoftGL {

class ShaderProgram;

class Uniform {
 public:
  explicit Uniform(const std::string &name) : name(name) {}

  int GetHash() {
    if (uuid_ < 0) {
      uuid_ = uuid_counter_++;
    }
    return uuid_;
  }

  virtual int GetLocation(ShaderProgram &program) = 0;
  virtual void BindProgram(ShaderProgram &program, int location, int binding) = 0;

 public:
  std::string name;

 private:
  int uuid_ = -1;
  static int uuid_counter_;
};

class UniformBlock : public Uniform {
 public:
  UniformBlock(const std::string &name, int size) : Uniform(name), block_size(size) {}

  virtual void SetSubData(void *data, int len, int offset) = 0;
  virtual void SetData(void *data, int len) = 0;

 protected:
  int block_size;
};

class UniformSampler : public Uniform {
 public:
  explicit UniformSampler(const std::string &name) : Uniform(name) {}
  virtual void SetTexture2D(Texture2D &tex) = 0;
  virtual void SetTextureCube(TextureCube &tex) = 0;
};

}
