/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "shader_utils.h"
#include <vector>
#include "base/file_utils.h"

namespace SoftGL {


void ShaderGLSL::SetHeader(const std::string &header) {
  header_ = header;
}

void ShaderGLSL::AddDefines(const std::string &def) {
  defines_ = def;
}

bool ShaderGLSL::LoadSource(const std::string &source) {
  id_ = glCreateShader(type_);
  std::string shader_str = header_ + defines_ + source;
  const char *shader_str_ptr = shader_str.c_str();
  GLint length = shader_str.length();
  glShaderSource(id_, 1, &shader_str_ptr, &length);
  glCompileShader(id_);

  GLint isCompiled = 0;
  glGetShaderiv(id_, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE) {
    GLint maxLength = 0;
    glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &maxLength);
    std::vector<GLchar> infoLog(maxLength);
    glGetShaderInfoLog(id_, maxLength, &maxLength, &infoLog[0]);
    LOGE("compile shader failed: %s", &infoLog[0]);

    Destroy();
    return false;
  }

  return true;
}

bool ShaderGLSL::LoadFile(const std::string &path) {
  std::string source = FileUtils::ReadAll(path);
  if (source.length() <= 0) {
    LOGE("read shader source failed");
    return false;
  }

  return LoadSource(source);
}

void ShaderGLSL::Destroy() {
  glDeleteShader(id_);
}

void ProgramGLSL::AddDefine(const std::string &def) {
  defines_ += ("#define " + def + " \n");
}

bool ProgramGLSL::LoadShader(ShaderGLSL &vs, ShaderGLSL &fs) {
  id_ = glCreateProgram();
  glAttachShader(id_, vs.GetId());
  glAttachShader(id_, fs.GetId());
  glLinkProgram(id_);
  glValidateProgram(id_);

  GLint isLinked = 0;
  glGetProgramiv(id_, GL_LINK_STATUS, (int *) &isLinked);
  if (isLinked == GL_FALSE) {
    GLint maxLength = 0;
    glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &maxLength);

    std::vector<GLchar> infoLog(maxLength);
    glGetProgramInfoLog(id_, maxLength, &maxLength, &infoLog[0]);
    LOGE("link program failed: %s", &infoLog[0]);

    Destroy();
    return false;
  }

  return true;
}

bool ProgramGLSL::LoadSource(const std::string &vsSource, const std::string &fsSource) {
  ShaderGLSL vs(GL_VERTEX_SHADER);
  ShaderGLSL fs(GL_FRAGMENT_SHADER);

  vs.AddDefines(defines_);
  fs.AddDefines(defines_);

  if (!vs.LoadSource(vsSource)) {
    LOGE("load vertex shader source failed");
    return false;
  }

  if (!fs.LoadSource(fsSource)) {
    LOGE("load fragment shader source failed");
    return false;
  }

  return LoadShader(vs, fs);
}

bool ProgramGLSL::LoadFile(const std::string &vsPath, const std::string &fsPath) {
  ShaderGLSL vs(GL_VERTEX_SHADER);
  ShaderGLSL fs(GL_FRAGMENT_SHADER);

  vs.AddDefines(defines_);
  fs.AddDefines(defines_);

  if (!vs.LoadFile(vsPath)) {
    LOGE("load vertex shader file failed: %s", vsPath.c_str());
    return false;
  }

  if (!fs.LoadFile(fsPath)) {
    LOGE("load fragment shader file failed: %s", fsPath.c_str());
    return false;
  }

  return LoadShader(vs, fs);
}

void ProgramGLSL::Use() {
  if (id_) {
    glUseProgram(id_);
  } else {
    LOGE("failed to use program, not ready");
  }
}

void ProgramGLSL::Destroy() {
  glDeleteProgram(id_);
}

}
