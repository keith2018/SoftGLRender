/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "GLSLUtils.h"
#include <vector>
#include <regex>
#include "Base/FileUtils.h"
#include "Render/OpenGL/OpenGLUtils.h"

namespace SoftGL {

void ShaderGLSL::setHeader(const std::string &header) {
  header_ = header;
}

void ShaderGLSL::addDefines(const std::string &def) {
  defines_ = def;
}

bool ShaderGLSL::loadSource(const std::string &source) {
  id_ = glCreateShader(type_);
  std::string shaderStr;
  if (type_ == GL_VERTEX_SHADER) {
    shaderStr = compatibleVertexPreprocess(header_ + defines_ + source);
  } else if (type_ == GL_FRAGMENT_SHADER) {
    shaderStr = compatibleFragmentPreprocess(header_ + defines_ + source);
  }
  if (shaderStr.empty()) {
    LOGE("ShaderGLSL::loadSource failed: empty source");
    return false;
  }

  const char *shaderStrPtr = shaderStr.c_str();
  auto length = (GLint) shaderStr.length();
  GL_CHECK(glShaderSource(id_, 1, &shaderStrPtr, &length));
  GL_CHECK(glCompileShader(id_));

  GLint isCompiled = 0;
  GL_CHECK(glGetShaderiv(id_, GL_COMPILE_STATUS, &isCompiled));
  if (isCompiled == GL_FALSE) {
    GLint maxLength = 0;
    GL_CHECK(glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &maxLength));
    std::vector<GLchar> infoLog(maxLength);
    GL_CHECK(glGetShaderInfoLog(id_, maxLength, &maxLength, &infoLog[0]));
    LOGE("compile shader failed: %s", &infoLog[0]);

    destroy();
    return false;
  }

  return true;
}

bool ShaderGLSL::loadFile(const std::string &path) {
  std::string source = FileUtils::readText(path);
  if (source.length() <= 0) {
    LOGE("read shader source failed");
    return false;
  }

  return loadSource(source);
}

void ShaderGLSL::destroy() {
  if (id_) {
    GL_CHECK(glDeleteShader(id_));
    id_ = 0;
  }
}

std::string ShaderGLSL::compatibleVertexPreprocess(const std::string &source) {
  std::regex outLocationRegex(R"(layout\s*\(\s*location\s*=\s*\d+\s*\)\s*out\s*)");
  std::regex std140Regex(R"(layout\s*\(.*std140.*\)\s*uniform\s*)");
  std::regex uniformBindingRegex(R"(layout\s*\(.*binding\s*=.*\)\s*uniform\s*)");

  std::string result = std::regex_replace(source, outLocationRegex, "out ");
  result = std::regex_replace(result, std140Regex, "layout (std140) uniform ");
  result = std::regex_replace(result, uniformBindingRegex, "uniform ");

  return result;
}

std::string ShaderGLSL::compatibleFragmentPreprocess(const std::string &source) {
  std::regex inLocationRegex(R"(layout\s*\(\s*location\s*=\s*\d+\s*\)\s*in\s*)");
  std::regex outLocationRegex(R"(layout\s*\(\s*location\s*=\s*\d+\s*\)\s*out\s*)");
  std::regex std140Regex(R"(layout\s*\(.*std140.*\)\s*uniform\s*)");
  std::regex uniformBindingRegex(R"(layout\s*\(.*binding\s*=.*\)\s*uniform\s*)");

  std::string result = std::regex_replace(source, inLocationRegex, "in ");
  result = std::regex_replace(result, outLocationRegex, "out ");
  result = std::regex_replace(result, std140Regex, "layout (std140) uniform ");
  result = std::regex_replace(result, uniformBindingRegex, "uniform ");

  return result;
}

void ProgramGLSL::addDefine(const std::string &def) {
  if (def.empty()) {
    return;
  }
  defines_ += ("#define " + def + " \n");
}

bool ProgramGLSL::loadShader(ShaderGLSL &vs, ShaderGLSL &fs) {
  id_ = glCreateProgram();
  GL_CHECK(glAttachShader(id_, vs.getId()));
  GL_CHECK(glAttachShader(id_, fs.getId()));
  GL_CHECK(glLinkProgram(id_));
  GL_CHECK(glValidateProgram(id_));

  GLint isLinked = 0;
  GL_CHECK(glGetProgramiv(id_, GL_LINK_STATUS, (int *) &isLinked));
  if (isLinked == GL_FALSE) {
    GLint maxLength = 0;
    GL_CHECK(glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &maxLength));

    std::vector<GLchar> infoLog(maxLength);
    GL_CHECK(glGetProgramInfoLog(id_, maxLength, &maxLength, &infoLog[0]));
    LOGE("link program failed: %s", &infoLog[0]);

    destroy();
    return false;
  }

  return true;
}

bool ProgramGLSL::loadSource(const std::string &vsSource, const std::string &fsSource) {
  ShaderGLSL vs(GL_VERTEX_SHADER);
  ShaderGLSL fs(GL_FRAGMENT_SHADER);

  vs.addDefines(defines_);
  fs.addDefines(defines_);

  if (!vs.loadSource(vsSource)) {
    LOGE("load vertex shader source failed");
    return false;
  }

  if (!fs.loadSource(fsSource)) {
    LOGE("load fragment shader source failed");
    return false;
  }

  return loadShader(vs, fs);
}

bool ProgramGLSL::loadFile(const std::string &vsPath, const std::string &fsPath) {
  ShaderGLSL vs(GL_VERTEX_SHADER);
  ShaderGLSL fs(GL_FRAGMENT_SHADER);

  vs.addDefines(defines_);
  fs.addDefines(defines_);

  if (!vs.loadFile(vsPath)) {
    LOGE("load vertex shader file failed: %s", vsPath.c_str());
    return false;
  }

  if (!fs.loadFile(fsPath)) {
    LOGE("load fragment shader file failed: %s", fsPath.c_str());
    return false;
  }

  return loadShader(vs, fs);
}

void ProgramGLSL::use() const {
  if (id_) {
    GL_CHECK(glUseProgram(id_));
  } else {
    LOGE("failed to use program, not ready");
  }
}

void ProgramGLSL::destroy() {
  if (id_) {
    GL_CHECK(glDeleteProgram(id_));
    id_ = 0;
  }
}

}
