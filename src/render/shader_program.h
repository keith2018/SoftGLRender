/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include <set>
#include "uniform.h"

namespace SoftGL {

class ShaderProgram {
 public:
  virtual int GetId() const = 0;
  virtual void AddDefine(const std::string &def) = 0;
  virtual bool CompileAndLink(const std::string &vsSource, const std::string &fsSource) = 0;
  virtual void Use() = 0;

  virtual void AddDefines(const std::set<std::string> &defs) {
    for (auto &str : defs) {
      AddDefine(str);
    }
  }

  virtual void BindUniforms(std::vector<std::shared_ptr<Uniform>> &uniforms) {
    int binding = 0;
    for (auto &uniform : uniforms) {
      bool success = SetUniform(*uniform, binding);
      if (success) {
        binding++;
      }
    }
  }

 protected:
  virtual bool SetUniform(Uniform &uniform, int binding) {
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

 protected:
  std::unordered_map<int, int> uniform_locations_;
};

}
