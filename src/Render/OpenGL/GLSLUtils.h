/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include <glad/glad.h>

namespace SoftGL {

constexpr char const *OpenGL_GLSL_VERSION = "#version 330 core";
constexpr char const *OpenGL_GLSL_DEFINE = "OpenGL";

class ShaderGLSL {
 public:
  explicit ShaderGLSL(GLenum type) : type_(type) {
    header_ = OpenGL_GLSL_VERSION;
    header_ += "\n";
  };
  ~ShaderGLSL() { destroy(); }

  void setHeader(const std::string &header);
  void addDefines(const std::string &def);
  bool loadSource(const std::string &source);
  bool loadFile(const std::string &path);
  void destroy();

  inline bool empty() const { return 0 == id_; };
  inline GLuint getId() const { return id_; };

 private:
  static std::string compatibleVertexPreprocess(const std::string &source);
  static std::string compatibleFragmentPreprocess(const std::string &source);

 private:
  GLenum type_;
  GLuint id_ = 0;
  std::string header_;
  std::string defines_;
};

class ProgramGLSL {
 public:
  ProgramGLSL() { addDefine(OpenGL_GLSL_DEFINE); }
  ~ProgramGLSL() { destroy(); }

  void addDefine(const std::string &def);
  bool loadSource(const std::string &vsSource, const std::string &fsSource);
  bool loadFile(const std::string &vsPath, const std::string &fsPath);
  void use() const;
  void destroy();

  inline bool empty() const { return 0 == id_; };
  inline GLuint getId() const { return id_; };

 private:
  bool loadShader(ShaderGLSL &vs, ShaderGLSL &fs);

 private:
  GLuint id_ = 0;
  std::string defines_;
};

}
