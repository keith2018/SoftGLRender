/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include <glad/glad.h>

namespace SoftGL {

class ShaderGLSL {
 public:
  explicit ShaderGLSL(GLenum type) : type_(type) {};
  ~ShaderGLSL() { Destroy(); }

  bool LoadSource(const std::string &source);
  bool LoadFile(const std::string &path);
  void Destroy();

  inline bool Empty() const { return 0 == id_; };
  inline GLuint GetId() const { return id_; };

 private:
  GLenum type_;
  GLuint id_ = 0;
};

class ProgramGLSL {
 public:
  ~ProgramGLSL() { Destroy(); }

  bool LoadShader(ShaderGLSL &vs, ShaderGLSL &fs);
  bool LoadSource(const std::string &vsSource, const std::string &fsSource);
  bool LoadFile(const std::string &vsPath, const std::string &fsPath);
  void Use();
  void Destroy();

  inline bool Empty() const { return 0 == id_; };
  inline GLuint GetId() const { return id_; };

 private:
  GLuint id_ = 0;
};

}