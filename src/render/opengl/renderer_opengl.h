/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "../renderer.h"
#include "glad/glad.h"
#include "base/shader_utils.h"


namespace SoftGL {

class VertexGLSL : public VertexHandler {
 public:
  void Create(std::vector<Vertex> &vertexes, std::vector<int> &indices);
  bool Empty() const;
  void BindVAO();
  virtual ~VertexGLSL();

 private:
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint ebo_ = 0;
};

class UniformsGLSL {
 public:
  bool Empty() const;
  virtual ~UniformsGLSL();
  virtual void Init(GLuint program) = 0;
  virtual void UpdateData() = 0;

 protected:
  void Create(GLuint program, const char *blockName, GLint blockSize);

 public:
  GLuint ubo_ = GL_NONE;
};

class BasicUniforms : public UniformsGLSL {
 public:
  void Init(GLuint program) override;
  void UpdateData() override;

 public:
  glm::mat4 u_modelViewProjectionMatrix;
  glm::vec4 u_fragColor;
};

class RendererOpenGL : public Renderer {
 public:
  void Create(int width, int height, float near, float far) override;
  void Clear(float r, float g, float b, float a) override;

  void DrawMeshTextured(ModelMesh &mesh) override;
  void DrawMeshWireframe(ModelMesh &mesh) override;
  void DrawLines(ModelLines &lines) override;
  void DrawPoints(ModelPoints &points) override;

  BasicUniforms &GetPointUniforms() {
    return uniforms_basic;
  }

  BasicUniforms &GetLineUniforms() {
    return uniforms_basic;
  }

 private:
  ProgramGLSL program_basic;
  BasicUniforms uniforms_basic;
};

}
