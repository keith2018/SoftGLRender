/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <set>
#include "uniform.h"
#include "base/logger.h"


namespace SoftGL {

class ShaderProgram {
 public:
  virtual int GetId() const = 0;
  virtual void AddDefine(const std::string &def) = 0;
  virtual bool CompileAndLink(const std::string &vsSource, const std::string &fsSource) = 0;

  virtual void AddDefines(const std::set<std::string> &defs) {
    for (auto &str : defs) {
      AddDefine(str);
    }
  }

  virtual void AddUniformBlock(const std::shared_ptr<UniformBlock> &uniform) {
    uniform_blocks_.push_back(uniform);
  }

  virtual void AddUniformSampler(const int key, const std::shared_ptr<UniformSampler> &uniform) {
    uniform_samplers_[key] = uniform;
  }

  std::vector<std::shared_ptr<UniformBlock>> &GetUniformBlocks() {
    return uniform_blocks_;
  }

  std::unordered_map<int, std::shared_ptr<UniformSampler>> &GetUniformSamplers() {
    return uniform_samplers_;
  }

 protected:
  std::vector<std::shared_ptr<UniformBlock>> uniform_blocks_;
  std::unordered_map<int, std::shared_ptr<UniformSampler>> uniform_samplers_;
};

}
