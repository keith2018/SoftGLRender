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

  virtual bool CompileAndLink(const std::string &vsSource, const std::string &fsSource) = 0;
};

}
