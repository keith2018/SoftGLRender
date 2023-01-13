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
  void AddDefine(const std::string &def) override {
    program_glsl_.AddDefine(def);
  }

  bool CompileAndLink(const std::string &vsSource, const std::string &fsSource) override {
    bool ret = program_glsl_.LoadSource(vsSource, fsSource);
    programId_ = program_glsl_.GetId();
    return ret;
  }

  void Use() override {
    program_glsl_.Use();
  };

  inline GLuint GetId() const {
    return programId_;
  }

 private:
  GLuint programId_ = 0;
  ProgramGLSL program_glsl_;
};

}
