/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <set>
#include "Uniform.h"

namespace SoftGL {

class ShaderProgram {
 public:
  virtual int getId() const = 0;

  virtual void addDefine(const std::string &def) = 0;

  virtual void addDefines(const std::set<std::string> &defs) {
    for (auto &str : defs) {
      addDefine(str);
    }
  }

  virtual void bindUniforms(ShaderUniforms &uniforms) {
    for (auto &kv : uniforms.blocks) {
      bindUniform(*kv.second);
    }

    for (auto &kv : uniforms.samplers) {
      bindUniform(*kv.second);
    }
  }

 protected:
  virtual bool bindUniform(Uniform &uniform) {
    int hash = uniform.getHash();
    int loc = -1;
    if (uniformLocations_.find(hash) == uniformLocations_.end()) {
      loc = uniform.getLocation(*this);
      uniformLocations_[hash] = loc;
    } else {
      loc = uniformLocations_[hash];
    }

    if (loc < 0) {
      return false;
    }

    uniform.bindProgram(*this, loc);
    return true;
  };

 protected:
  std::unordered_map<int, int> uniformLocations_;
};

}
