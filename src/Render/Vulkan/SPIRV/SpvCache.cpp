/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "SpvCache.h"
#include "SpvCompiler.h"
#include <experimental/filesystem>
#include "json11.hpp"
#include "Base/HashUtils.h"
#include "Base/FileUtils.h"

namespace SoftGL {

namespace fs = std::experimental::filesystem;
const std::string SHADER_SPV_CACHE_DIR = "./cache/SPV/";

std::string SpvCache::getShaderHashKey(const std::string &source) {
  return HashUtils::getHashMD5(source);
}

std::string SpvCache::getCacheFilePath(const std::string &hashKey) {
  if (!fs::exists(SHADER_SPV_CACHE_DIR)) {
    fs::create_directories(SHADER_SPV_CACHE_DIR);
  }
  return SHADER_SPV_CACHE_DIR + hashKey;
}

void SpvCache::writeToFile(const std::string &hashKey, const ShaderCompilerResult &compileRet) {
  auto cachePathBase = getCacheFilePath(hashKey);
  std::string jsonFilePath = cachePathBase + ".json";

  json11::Json::object obj;
  json11::Json::object uniformsObj;
  for (const auto &kv : compileRet.uniformsDesc) {
    const auto &name = kv.first;
    const auto &desc = kv.second;
    json11::Json::object descObj;
    descObj["type"] = desc.type;
    descObj["location"] = desc.location;
    descObj["binding"] = desc.binding;
    descObj["set"] = desc.set;
    uniformsObj[name] = descObj;
  }
  obj["uniformsDesc"] = uniformsObj;

  std::string spvFilePath = cachePathBase + "_codes.spv";
  FileUtils::writeBytes(spvFilePath, reinterpret_cast<const char *>(compileRet.spvCodes.data()),
                        compileRet.spvCodes.size() * sizeof(uint32_t));
  obj["spvCodesFilePath"] = spvFilePath;

  auto jsonStr = json11::Json(obj).dump();
  FileUtils::writeText(jsonFilePath, jsonStr);
}

ShaderCompilerResult SpvCache::readFromFile(const std::string &hashKey) {
  ShaderCompilerResult result;
  auto cachePathBase = getCacheFilePath(hashKey);
  std::string jsonFilePath = cachePathBase + ".json";

  if (!fs::exists(jsonFilePath)) {
    return result;
  }

  std::string err;
  const auto json = json11::Json::parse(FileUtils::readText(jsonFilePath), err);
  const auto &uniformsObj = json["uniformsDesc"].object_items();
  for (const auto &kv : uniformsObj) {
    const auto &name = kv.first;
    const auto &descJson = kv.second;
    ShaderUniformDesc desc;
    desc.name = name;
    desc.type = static_cast<ShaderUniformType>(descJson["type"].int_value());
    desc.location = descJson["location"].int_value();
    desc.binding = descJson["binding"].int_value();
    desc.set = descJson["set"].int_value();
    result.uniformsDesc[name] = desc;
  }

  std::string spvFilePath = json["spvCodesFilePath"].string_value();
  auto bytes = FileUtils::readBytes(spvFilePath);
  if (bytes.empty() || bytes.size() % sizeof(uint32_t) != 0) {
    LOGE("read spv cache code error: invalid length");
    return result;
  }
  result.spvCodes.resize(bytes.size() / sizeof(uint32_t));
  memcpy(result.spvCodes.data(), bytes.data(), bytes.size());

  // TODO check md5
  return result;
}

}