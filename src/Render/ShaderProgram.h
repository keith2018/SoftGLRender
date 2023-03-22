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

  virtual void bindResources(ShaderResources &resources) {
    for (auto &kv : resources.blocks) {
      bindUniform(*kv.second);
    }

    for (auto &kv : resources.samplers) {
      bindUniform(*kv.second);
    }
  }

 protected:
  virtual bool bindUniform(Uniform &uniform) {
    int hash = uniform.getHash();
    int binding = -1;
    if (uniformBindings_.find(hash) == uniformBindings_.end()) {
      binding = uniform.getBinding(*this);
      uniformBindings_[hash] = binding;
    } else {
      binding = uniformBindings_[hash];
    }

    if (binding < 0) {
      return false;
    }

    uniform.bindProgram(*this, binding);
    return true;
  };

 protected:
  std::unordered_map<int, int> uniformBindings_;
};

}
