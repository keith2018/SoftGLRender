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
  int getId() const override {
    return (int) programId_;
  }

  void addDefine(const std::string &def) override {
    programGLSL_.addDefine(def);
  }

  bool CompileAndLink(const std::string &vsSource, const std::string &fsSource) {
    bool ret = programGLSL_.loadSource(vsSource, fsSource);
    programId_ = programGLSL_.getId();
    return ret;
  }

  inline void Use() {
    programGLSL_.use();
    uniformBlockBinding_ = 0;
    uniformSamplerBinding_ = 0;
  }

  inline int GetUniformBlockBinding() {
    return uniformBlockBinding_++;
  }

  inline int GetUniformSamplerBinding() {
    return uniformSamplerBinding_++;
  }

 private:
  GLuint programId_ = 0;
  ProgramGLSL programGLSL_;

  int uniformBlockBinding_ = 0;
  int uniformSamplerBinding_ = 0;
};

}
