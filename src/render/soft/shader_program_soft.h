/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include "render/shader_program.h"
#include "shader_soft.h"

namespace SoftGL {

class ShaderProgramSoft : public ShaderProgram {
 public:
  ShaderProgramSoft() : uuid_(uuid_counter_++) {}

  int GetId() const override {
    return uuid_;
  }

  void AddDefine(const std::string &def) override {
    defines_.emplace_back(def);
  }

  bool SetShaders(std::shared_ptr<ShaderSoft> vs, std::shared_ptr<ShaderSoft> fs) {
    vertex_shader_ = std::move(vs);
    fragment_shader_ = std::move(fs);

    // defines
    auto &define_desc = vertex_shader_->GetDefines();
    defines_buffer_.resize(define_desc.size());
    vertex_shader_->BindDefines(defines_buffer_.data());
    fragment_shader_->BindDefines(defines_buffer_.data());

    memset(defines_buffer_.data(), 0, defines_buffer_.size());
    for (auto &name : defines_) {
      for (int i = 0; i < define_desc.size(); i++) {
        if (define_desc[i] == name) {
          defines_buffer_[i] = 1;
        }
      }
    }

    // builtin
    vertex_shader_->BindBuiltin(&builtin_);
    fragment_shader_->BindBuiltin(&builtin_);

    // uniforms
    uniform_buffer_.resize(vertex_shader_->GetShaderUniformsSize());
    vertex_shader_->BindShaderUniforms(uniform_buffer_.data());
    fragment_shader_->BindShaderUniforms(uniform_buffer_.data());

    return true;
  }

  inline void BindVertexAttributes(void *ptr) {
    vertex_shader_->BindShaderAttributes(ptr);
  }

  inline void BindUniformBlockBuffer(void *data, size_t len, int location) {
    int offset = vertex_shader_->GetUniformOffset(location);
    memcpy(uniform_buffer_.data() + offset, data, len);
  }

  inline void BindUniformSampler(std::shared_ptr<SamplerSoft> &sampler, int location) {
    int offset = vertex_shader_->GetUniformOffset(location);
    auto **ptr = reinterpret_cast<SamplerSoft **>(uniform_buffer_.data() + offset);
    *ptr = sampler.get();
  }

  inline void BindVertexShaderVaryings(void *ptr) {
    vertex_shader_->BindShaderVaryings(ptr);
  }

  inline void BindFragmentShaderVaryings(void *ptr) {
    fragment_shader_->BindShaderVaryings(ptr);
  }

  inline size_t GetShaderVaryingsSize() {
    return vertex_shader_->GetShaderVaryingsSize();
  }

  inline int GetUniformLocation(const std::string &name) {
    return vertex_shader_->GetUniformLocation(name);
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

  std::vector<std::string> defines_;
  ShaderBuiltin builtin_;

  std::vector<uint8_t> defines_buffer_;  // 0->false; 1->true
  std::vector<uint8_t> uniform_buffer_;

 private:
  int uuid_ = -1;
  static int uuid_counter_;
};

}
