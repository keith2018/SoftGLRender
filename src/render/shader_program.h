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

  virtual void AddDefines(const std::set<std::string> &defs) {
    for (auto &str : defs) {
      AddDefine(str);
    }
  }

  virtual void BindUniforms(ShaderUniforms &uniforms) {
    for (auto &kv : uniforms.blocks) {
      BindUniform(*kv.second);
    }

    for (auto &kv : uniforms.samplers) {
      BindUniform(*kv.second);
    }
  }

 protected:
  virtual bool BindUniform(Uniform &uniform) {
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

    uniform.BindProgram(*this, loc);
    return true;
  };

 protected:
  std::unordered_map<int, int> uniform_locations_;
};

}
