/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <unordered_map>
#include "render/shader_program.h"

namespace SoftGL {

class ShaderProgramSoft : public ShaderProgram {
 public:
  ShaderProgramSoft() : uuid_(uuid_counter_++) {}

  int GetId() const override {
    return uuid_;
  }

  void AddDefine(const std::string &def) override {
  }

 private:
  int uuid_ = -1;
  static int uuid_counter_;
};

}
