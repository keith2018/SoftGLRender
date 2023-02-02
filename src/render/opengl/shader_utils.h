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
  explicit ShaderGLSL(GLenum type) : type_(type) {
    header_ = "#version 330 core\n";
  };
  ~ShaderGLSL() { Destroy(); }

  void SetHeader(const std::string &header);
  void AddDefines(const std::string &def);
  bool LoadSource(const std::string &source);
  bool LoadFile(const std::string &path);
  void Destroy();

  inline bool Empty() const { return 0 == id_; };
  inline GLuint GetId() const { return id_; };

 private:
  GLenum type_;
  GLuint id_ = 0;
  std::string header_;
  std::string defines_;
};

class ProgramGLSL {
 public:
  ~ProgramGLSL() { Destroy(); }

  void AddDefine(const std::string &def);
  bool LoadSource(const std::string &vsSource, const std::string &fsSource);
  bool LoadFile(const std::string &vsPath, const std::string &fsPath);
  void Use();
  void Destroy();

  inline bool Empty() const { return 0 == id_; };
  inline GLuint GetId() const { return id_; };

 private:
  bool LoadShader(ShaderGLSL &vs, ShaderGLSL &fs);

 private:
  GLuint id_ = 0;
  std::string defines_;
};

}