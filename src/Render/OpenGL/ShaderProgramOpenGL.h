/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include "Base/FileUtils.h"
#include "Render/ShaderProgram.h"
#include "GLSLUtils.h"

namespace SoftGL {

class ShaderProgramOpenGL : public ShaderProgram {
 public:
  int getId() const override {
    return (int) programId_;
  }

  void addDefine(const std::string &def) override {
    programGLSL_.addDefine(def);
  }

  bool compileAndLinkFile(const std::string &vsPath, const std::string &fsPath) {
    return compileAndLink(FileUtils::readText(vsPath), FileUtils::readText(fsPath));
  }

  bool compileAndLink(const std::string &vsSource, const std::string &fsSource) {
    bool ret = programGLSL_.loadSource(vsSource, fsSource);
    programId_ = programGLSL_.getId();
    return ret;
  }

  inline void use() {
    programGLSL_.use();
    uniformBlockBinding_ = 0;
    uniformSamplerBinding_ = 0;
  }

  inline int getUniformBlockBinding() {
    return uniformBlockBinding_++;
  }

  inline int getUniformSamplerBinding() {
    return uniformSamplerBinding_++;
  }

 private:
  GLuint programId_ = 0;
  ProgramGLSL programGLSL_;

  int uniformBlockBinding_ = 0;
  int uniformSamplerBinding_ = 0;
};

}
