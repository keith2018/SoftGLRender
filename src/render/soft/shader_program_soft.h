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
    defines_buffer_ = MemoryUtils::MakeBuffer<uint8_t>(define_desc.size());
    vertex_shader_->BindDefines(defines_buffer_.get());
    fragment_shader_->BindDefines(defines_buffer_.get());

    memset(defines_buffer_.get(), 0, sizeof(uint8_t) * define_desc.size());
    for (auto &name : defines_) {
      for (int i = 0; i < define_desc.size(); i++) {
        if (define_desc[i] == name) {
          defines_buffer_.get()[i] = 1;
        }
      }
    }

    // builtin
    vertex_shader_->BindBuiltin(&builtin_);
    fragment_shader_->BindBuiltin(&builtin_);

    // uniforms
    uniform_buffer_ = MemoryUtils::MakeBuffer<uint8_t>(vertex_shader_->GetShaderUniformsSize());
    vertex_shader_->BindShaderUniforms(uniform_buffer_.get());
    fragment_shader_->BindShaderUniforms(uniform_buffer_.get());

    return true;
  }

  inline void BindVertexAttributes(void *ptr) {
    vertex_shader_->BindShaderAttributes(ptr);
  }

  inline void BindUniformBlockBuffer(void *data, size_t len, int location) {
    int offset = vertex_shader_->GetUniformOffset(location);
    memcpy(uniform_buffer_.get() + offset, data, len);
  }

  inline void BindUniformSampler(std::shared_ptr<SamplerSoft> &sampler, int location) {
    int offset = vertex_shader_->GetUniformOffset(location);
    auto **ptr = reinterpret_cast<SamplerSoft **>(uniform_buffer_.get() + offset);
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

  inline void PrepareFragmentShader() {
    fragment_shader_->PrepareExecMain();
  }

  inline void ExecFragmentShader() {
    fragment_shader_->SetupSamplerDerivative();
    fragment_shader_->ShaderMain();
  }

  inline std::shared_ptr<ShaderProgramSoft> clone() const {
    auto ret = std::make_shared<ShaderProgramSoft>(*this);

    ret->vertex_shader_ = vertex_shader_->clone();
    ret->fragment_shader_ = fragment_shader_->clone();
    ret->vertex_shader_->BindBuiltin(&ret->builtin_);
    ret->fragment_shader_->BindBuiltin(&ret->builtin_);

    return ret;
  }

 private:
  ShaderBuiltin builtin_;
  std::vector<std::string> defines_;

  std::shared_ptr<ShaderSoft> vertex_shader_;
  std::shared_ptr<ShaderSoft> fragment_shader_;

  std::shared_ptr<uint8_t> defines_buffer_;  // 0->false; 1->true
  std::shared_ptr<uint8_t> uniform_buffer_;

 private:
  int uuid_ = -1;
  static int uuid_counter_;
};

}
