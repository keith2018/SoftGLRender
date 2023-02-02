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

  bool CompileAndLink(const std::string &vsSource, const std::string &fsSource) override {
    bool ret = program_glsl_.LoadSource(vsSource, fsSource);
    programId_ = program_glsl_.GetId();
    return ret;
  }

  void Use() {
    program_glsl_.Use();
  }

  void BindUniforms(ShaderUniforms &uniforms) {
    int binding = 0;
    for (auto &uniform : uniforms.blocks) {
      bool success = SetUniform(*uniform, binding);
      if (success) {
        binding++;
      }
    }

    binding = 0;
    for (auto &kv : uniforms.samplers) {
      bool success = SetUniform(*kv.second, binding);
      if (success) {
        binding++;
      }
    }
  }

 protected:
  bool SetUniform(Uniform &uniform, int binding) {
    int hash = uniform.GetHash();
    int loc = -1;
    if (uniform_locations_.find(hash) == uniform_locations_.end()) {
      loc = uniform.GetLocation(*this);
      uniform_locations_[hash] = loc;
    } else {
      loc = uniform_locations_[hash];
    }

    if (loc < 0) {
      return false;
    }

    uniform.BindProgram(*this, loc, binding);
    return true;
  };

 private:
  GLuint programId_ = 0;
  ProgramGLSL program_glsl_;
  std::unordered_map<int, int> uniform_locations_;
};

}
