/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include "Base/UUID.h"
#include "Render/ShaderProgram.h"
#include "ShaderSoft.h"

namespace SoftGL {

class ShaderProgramSoft : public ShaderProgram {
 public:
  int getId() const override {
    return uuid_.get();
  }

  void addDefine(const std::string &def) override {
    defines_.emplace_back(def);
  }

  bool SetShaders(std::shared_ptr<ShaderSoft> vs, std::shared_ptr<ShaderSoft> fs) {
    vertexShader_ = std::move(vs);
    fragmentShader_ = std::move(fs);

    // defines
    auto &defineDesc = vertexShader_->getDefines();
    definesBuffer_ = MemoryUtils::makeBuffer<uint8_t>(defineDesc.size());
    vertexShader_->bindDefines(definesBuffer_.get());
    fragmentShader_->bindDefines(definesBuffer_.get());

    memset(definesBuffer_.get(), 0, sizeof(uint8_t) * defineDesc.size());
    for (auto &name : defines_) {
      for (int i = 0; i < defineDesc.size(); i++) {
        if (defineDesc[i] == name) {
          definesBuffer_.get()[i] = 1;
        }
      }
    }

    // builtin
    vertexShader_->bindBuiltin(&builtin_);
    fragmentShader_->bindBuiltin(&builtin_);

    // uniforms
    uniformBuffer_ = MemoryUtils::makeBuffer<uint8_t>(vertexShader_->getShaderUniformsSize());
    vertexShader_->bindShaderUniforms(uniformBuffer_.get());
    fragmentShader_->bindShaderUniforms(uniformBuffer_.get());

    return true;
  }

  inline void bindVertexAttributes(void *ptr) {
    vertexShader_->bindShaderAttributes(ptr);
  }

  inline void bindUniformBlockBuffer(void *data, size_t len, int location) {
    int offset = vertexShader_->GetUniformOffset(location);
    memcpy(uniformBuffer_.get() + offset, data, len);
  }

  inline void bindUniformSampler(std::shared_ptr<SamplerSoft> &sampler, int location) {
    int offset = vertexShader_->GetUniformOffset(location);
    auto **ptr = reinterpret_cast<SamplerSoft **>(uniformBuffer_.get() + offset);
    *ptr = sampler.get();
  }

  inline void bindVertexShaderVaryings(void *ptr) {
    vertexShader_->bindShaderVaryings(ptr);
  }

  inline void bindFragmentShaderVaryings(void *ptr) {
    fragmentShader_->bindShaderVaryings(ptr);
  }

  inline size_t getShaderVaryingsSize() {
    return vertexShader_->getShaderVaryingsSize();
  }

  inline int getUniformLocation(const std::string &name) {
    return vertexShader_->getUniformLocation(name);
  }

  inline ShaderBuiltin &getShaderBuiltin() {
    return builtin_;
  }

  inline void execVertexShader() {
    vertexShader_->shaderMain();
  }

  inline void prepareFragmentShader() {
    fragmentShader_->prepareExecMain();
  }

  inline void execFragmentShader() {
    fragmentShader_->setupSamplerDerivative();
    fragmentShader_->shaderMain();
  }

  inline std::shared_ptr<ShaderProgramSoft> clone() const {
    auto ret = std::make_shared<ShaderProgramSoft>(*this);

    ret->vertexShader_ = vertexShader_->clone();
    ret->fragmentShader_ = fragmentShader_->clone();
    ret->vertexShader_->bindBuiltin(&ret->builtin_);
    ret->fragmentShader_->bindBuiltin(&ret->builtin_);

    return ret;
  }

 private:
  ShaderBuiltin builtin_;
  std::vector<std::string> defines_;

  std::shared_ptr<ShaderSoft> vertexShader_;
  std::shared_ptr<ShaderSoft> fragmentShader_;

  std::shared_ptr<uint8_t> definesBuffer_;  // 0->false; 1->true
  std::shared_ptr<uint8_t> uniformBuffer_;

 private:
  UUID<ShaderProgramSoft> uuid_;
};

}
