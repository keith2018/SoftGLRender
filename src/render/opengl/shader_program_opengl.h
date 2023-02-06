/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include "render/shader_program.h"
#include "shader_utils.h"

namespace SoftGL {

class ShaderProgramOpenGL : public ShaderProgram {
 public:
  int GetId() const override {
    return (int) programId_;
  }

  void AddDefine(const std::string &def) override {
    program_glsl_.AddDefine(def);
  }

  bool CompileAndLink(const std::string &vsSource, const std::string &fsSource) {
    bool ret = program_glsl_.LoadSource(vsSource, fsSource);
    programId_ = program_glsl_.GetId();
    return ret;
  }

  inline void Use() {
    program_glsl_.Use();
    uniform_block_binding_ = 0;
    uniform_sampler_binding_ = 0;
  }

  inline int GetUniformBlockBinding() {
    return uniform_block_binding_++;
  }

  inline int GetUniformSamplerBinding() {
    return uniform_sampler_binding_++;
  }

 private:
  GLuint programId_ = 0;
  ProgramGLSL program_glsl_;

  int uniform_block_binding_ = 0;
  int uniform_sampler_binding_ = 0;
};

}
