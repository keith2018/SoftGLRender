/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "../renderer.h"
#include "glad/glad.h"
#include "material.h"

namespace SoftGL {

class VertexGLSL {
 public:
  void Create(std::vector<Vertex> &vertexes, std::vector<int> &indices);
  void UpdateVertexData(std::vector<Vertex> &vertexes);
  bool Empty() const;
  void BindVAO();
  virtual ~VertexGLSL();

 private:
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint ebo_ = 0;
};

class OpenGLRenderHandler : public RenderHandler {
 public:
  std::shared_ptr<VertexGLSL> vertex_handler;
  std::shared_ptr<BaseMaterial> material_handler;
  TextureMap texture_handler;
};

class RendererOpenGL : public Renderer {
 public:
  void Create(int width, int height, float near, float far) override;
  void Clear(float r, float g, float b, float a) override;

  void DrawMeshTextured(ModelMesh &mesh) override;
  void DrawMeshWireframe(ModelMesh &mesh) override;
  void DrawLines(ModelLines &lines) override;
  void DrawPoints(ModelPoints &points) override;

  RendererUniforms &GetRendererUniforms() {
    return uniforms_;
  }

 private:
  void InitVertex(ModelBase &model, bool needUpdate = false);
  void InitTextures(ModelMesh &mesh);
  void InitMaterial(ModelBase &model);
  void InitMaterialWithType(ModelBase &model, MaterialType material_type);
  void DrawImpl(ModelBase &model, GLenum mode);

  MaterialType GetMaterialType(ModelBase &model);
  std::shared_ptr<BaseMaterial> CreateMeshMaterial(ModelMesh &mesh, MaterialType type);

 private:
  RendererUniforms uniforms_;
};

}
