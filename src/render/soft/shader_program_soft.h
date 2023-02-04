/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include <unordered_map>
#include "render/shader_program.h"

namespace SoftGL {

class UniformBlockDesc {
 public:
  UniformBlockDesc(const char *name, int offset)
      : name(name), offset(offset) {};

 public:
  std::string name;
  int offset = -1;
};

struct ShaderBuiltin {
  // vertex shader output
  glm::vec4 Position;

  // fragment shader input
  glm::vec4 FragCoord;
  bool FrontFacing;

  // fragment shader output
  float FragDepth;
  glm::vec4 FragColor;
  bool discard = false;
};

class ShaderSoft {
 public:
  virtual void ShaderMain() = 0;

  virtual void BindBuiltin(void *ptr) = 0;
  virtual void BindVertexAttributes(void *ptr) = 0;
  virtual void BindUniformBlocks(void *ptr) = 0;

  virtual std::vector<UniformBlockDesc> &GetUniformBlockDesc() = 0;
  virtual size_t GetUniformBlocksSize() = 0;

  virtual int GetUniformBlockLocation(const std::string &name) {
    auto &desc = GetUniformBlockDesc();
    for (int i = 0; i < desc.size(); i++) {
      if (desc[i].name == name) {
        return i;
      }
    }
    return -1;
  };

  virtual int GetUniformBlockOffset(int loc) {
    auto &desc = GetUniformBlockDesc();
    if (loc < 0 || loc > desc.size()) {
      return -1;
    }
    return desc[loc].offset;
  };
};

class ShaderProgramSoft : public ShaderProgram {
 public:
  ShaderProgramSoft() : uuid_(uuid_counter_++) {}

  int GetId() const override {
    return uuid_;
  }

  void AddDefine(const std::string &def) override {}

  bool SetShaders(std::shared_ptr<ShaderSoft> vs, std::shared_ptr<ShaderSoft> fs) {
    vertex_shader_ = std::move(vs);
    fragment_shader_ = std::move(fs);

    // builtin
    vertex_shader_->BindBuiltin(&builtin_);
    fragment_shader_->BindBuiltin(&builtin_);

    // uniform blocks
    uniform_block_buffer_.resize(vertex_shader_->GetUniformBlocksSize());
    vertex_shader_->BindUniformBlocks(uniform_block_buffer_.data());
    fragment_shader_->BindUniformBlocks(uniform_block_buffer_.data());

    return true;
  }

  inline void BindVertexAttributes(void *ptr) {
    vertex_shader_->BindVertexAttributes(ptr);
  }

  inline void BindUniformBlockBuffer(void *data, size_t len, int location) {
    int offset = vertex_shader_->GetUniformBlockOffset(location);
    memcpy(uniform_block_buffer_.data() + offset, data, len);
  }

  inline int GetUniformBlockLocation(const std::string &name) {
    return vertex_shader_->GetUniformBlockLocation(name);
  }

  inline ShaderBuiltin &GetShaderBuiltin() {
    return builtin_;
  }

  inline void ExecVertexShader() {
    vertex_shader_->ShaderMain();
  }

  inline void ExecFragmentShader() {
    fragment_shader_->ShaderMain();
  }

 private:
  std::shared_ptr<ShaderSoft> vertex_shader_;
  std::shared_ptr<ShaderSoft> fragment_shader_;

  std::vector<uint8_t> uniform_block_buffer_;
  ShaderBuiltin builtin_;

 private:
  int uuid_ = -1;
  static int uuid_counter_;
};

}
