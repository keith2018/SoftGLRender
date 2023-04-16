/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include <cstdint>

namespace SoftGL {

struct ShaderCompilerResult;

class SpvCache {
 public:
  static std::string getShaderHashKey(const std::string &source);

  static std::string getCacheFilePath(const std::string &hashKey);

  static void writeToFile(const std::string &hashKey, const ShaderCompilerResult &compileRet);

  static ShaderCompilerResult readFromFile(const std::string &hashKey);
};

}
