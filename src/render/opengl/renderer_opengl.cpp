/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "renderer_opengl.h"
#include "opengl_utils.h"

namespace SoftGL {

void VertexGLSL::Create(std::vector<Vertex> &vertexes, std::vector<int> &indices) {
  if (vertexes.empty() || indices.empty()) {
    return;
  }

  // vao
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  // vbo
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, vertexes.size() * sizeof(Vertex), &vertexes[0], GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, a_texCoord));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, a_normal));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, a_tangent));
  glEnableVertexAttribArray(3);

  // ebo
  glGenBuffers(1, &ebo_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);
}

void VertexGLSL::UpdateVertexData(std::vector<Vertex> &vertexes) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, vertexes.size() * sizeof(Vertex), &vertexes[0], GL_STATIC_DRAW);
}

bool VertexGLSL::Empty() const {
  return 0 == vao_;
}

void VertexGLSL::BindVAO() {
  if (Empty()) {
    return;
  }
  glBindVertexArray(vao_);
}

VertexGLSL::~VertexGLSL() {
  if (Empty()) {
    return;
  }

  glDeleteBuffers(1, &vbo_);
  glDeleteBuffers(1, &ebo_);
  glDeleteVertexArrays(1, &vao_);
}

void RendererOpenGL::Create(int w, int h, float near, float far) {
  Renderer::Create(w, h, near, far);
  uniforms_.Init();
}

void RendererOpenGL::Clear(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RendererOpenGL::DrawMeshTextured(ModelMesh &mesh) {
  if (!mesh.render_handle) {
    mesh.render_handle = std::make_shared<OpenGLRenderHandler>();
  }
  InitVertex(mesh);
  InitTextures(mesh);
  InitMaterial(mesh);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  DrawImpl(mesh, GL_TRIANGLES);
}

void RendererOpenGL::DrawMeshWireframe(ModelMesh &mesh) {
  if (!mesh.render_handle) {
    mesh.render_handle = std::make_shared<OpenGLRenderHandler>();
  }
  InitVertex(mesh);
  InitTextures(mesh);
  InitMaterialWithType(mesh, MaterialType_BaseColor);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glLineWidth(1.f);
  DrawImpl(mesh, GL_TRIANGLES);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RendererOpenGL::DrawLines(ModelLines &lines) {
  glLineWidth(lines.line_width);

  if (!lines.render_handle) {
    lines.render_handle = std::make_shared<OpenGLRenderHandler>();
  }
  InitVertex(lines);
  InitMaterial(lines);
  DrawImpl(lines, GL_LINES);
}

void RendererOpenGL::DrawPoints(ModelPoints &points) {
  glPointSize(points.point_size);

  if (!points.render_handle) {
    points.render_handle = std::make_shared<OpenGLRenderHandler>();
  }
  InitVertex(points, true);
  InitMaterial(points);
  DrawImpl(points, GL_POINTS);
}

void RendererOpenGL::InitVertex(ModelBase &model, bool needUpdate) {
  auto *render_handle = (OpenGLRenderHandler *) (model.render_handle.get());
  if (!render_handle->vertex_handler) {
    auto handle = std::make_shared<VertexGLSL>();
    handle->Create(model.vertexes, model.indices);
    render_handle->vertex_handler = std::move(handle);
  }

  if (needUpdate) {
    render_handle->vertex_handler->UpdateVertexData(model.vertexes);
  }
}

MaterialType RendererOpenGL::GetMaterialType(ModelBase &model) {
  MaterialType material_type = MaterialType_BaseColor;
  switch (model.shading_type) {
    case ShadingType_PBR_BRDF: {
      material_type = MaterialType_PBR;
      break;
    }
    case ShadingType_BLINN_PHONG: {
      material_type = MaterialType_BlinnPhong;
      break;
    }
    case ShadingType_SKYBOX: {
      material_type = MaterialType_Skybox;
      break;
    }
    default:break;
  }
  return material_type;
}

void RendererOpenGL::DrawImpl(ModelBase &model, GLenum mode) {
  auto *render_handle = (OpenGLRenderHandler *) (model.render_handle.get());
  render_handle->vertex_handler->BindVAO();
  render_handle->material_handler->Use(uniforms_, render_handle->texture_handler);
  glDrawElements(mode, model.indices.size(), GL_UNSIGNED_INT, nullptr);
  CheckGLError();
}

void RendererOpenGL::InitTextures(ModelMesh &mesh) {
  auto *render_handle = (OpenGLRenderHandler *) (mesh.render_handle.get());
  if (render_handle->texture_handler.empty()) {
    // normal textures
    for (auto &kv : mesh.textures) {
      auto texture = std::make_shared<TextureGLSL>();
      texture->Create2D(kv.second);
      render_handle->texture_handler[kv.first] = std::move(texture);
    }

    // skybox textures
    if (mesh.skybox_tex) {
      auto texture = std::make_shared<TextureGLSL>();
      if (mesh.skybox_tex->type == Skybox_Cube) {
        texture->CreateCube(mesh.skybox_tex->cube);
        render_handle->texture_handler[TextureType_CUBE] = std::move(texture);
      } else {
        texture->Create2D(mesh.skybox_tex->equirectangular);
        render_handle->texture_handler[TextureType_EQUIRECTANGULAR] = std::move(texture);
      }
    }
  }
}

void RendererOpenGL::InitMaterial(ModelBase &model) {
  MaterialType material_type = GetMaterialType(model);
  InitMaterialWithType(model, material_type);
}

void RendererOpenGL::InitMaterialWithType(ModelBase &model, MaterialType material_type) {
  auto *render_handle = (OpenGLRenderHandler *) (model.render_handle.get());

  // if material type not equal, reset material_handler
  if (render_handle->material_handler && render_handle->material_handler->Type() != material_type) {
    render_handle->material_handler = nullptr;
  }

  if (!render_handle->material_handler) {
    std::shared_ptr<BaseMaterial> material = nullptr;
    switch (material_type) {
      case MaterialType_BaseColor: {
        material = std::make_shared<MaterialBaseColor>();
        break;
      }
      case MaterialType_Skybox: {
        auto materialSkybox = std::make_shared<MaterialSkybox>();
        SkyboxTexture *skybox_tex = ((ModelMesh &) model).skybox_tex;
        if (skybox_tex->type == Skybox_Equirectangular) {
          materialSkybox->SetEnableEquirectangularMap(true);
        }
        material = materialSkybox;
        break;
      }
      default: {
        material = CreateMeshMaterial((ModelMesh &) model, material_type);
        break;
      }
    }

    material->Init();
    render_handle->material_handler = std::move(material);
  }
}

std::shared_ptr<BaseMaterial> RendererOpenGL::CreateMeshMaterial(ModelMesh &mesh, MaterialType type) {
  auto *render_handle = (OpenGLRenderHandler *) (mesh.render_handle.get());
  auto &tex_handler = render_handle->texture_handler;
  std::shared_ptr<MaterialBlinnPhong> material = nullptr;

  switch (type) {
    case MaterialType_BlinnPhong: {
      material = std::make_shared<MaterialBlinnPhong>();
      break;
    }
    case MaterialType_PBR: {
      material = std::make_shared<MaterialPBR>();
      break;
    }
    default:break;
  }

  material->SetEnableDiffuseMap(tex_handler.find(TextureType_DIFFUSE) != tex_handler.end());
  material->SetEnableNormalMap(tex_handler.find(TextureType_NORMALS) != tex_handler.end());
  material->SetEnableAoMap(tex_handler.find(TextureType_PBR_AMBIENT_OCCLUSION) != tex_handler.end());
  material->SetEnableEmissiveMap(tex_handler.find(TextureType_EMISSIVE) != tex_handler.end());
  material->SetEnableAlphaDiscard(mesh.alpha_mode == Alpha_Mask);
  return material;
}

}
