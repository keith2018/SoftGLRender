/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "renderer_opengl.h"
#include "shader/basic_glsl.h"

namespace SoftGL {

void VertexGLSL::Create(std::vector<Vertex> &vertexes, std::vector<int> &indices) {
  // vao
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  // vbo
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, vertexes.size() * sizeof(Vertex), vertexes.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (sizeof(glm::vec3)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (sizeof(glm::vec3) + sizeof(glm::vec2)));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *) (2 * sizeof(glm::vec3) + sizeof(glm::vec2)));

  // ebo
  glGenBuffers(1, &ebo_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);
}

bool VertexGLSL::Empty() const {
  return 0 == vao_;
}

void VertexGLSL::BindVAO() {
  glBindVertexArray(vao_);
}

VertexGLSL::~VertexGLSL() {
  glDeleteBuffers(1, &vbo_);
  glDeleteBuffers(1, &ebo_);
  glDeleteVertexArrays(1, &vao_);
}

void UniformsGLSL::Create(GLuint program, const char *blockName, GLint blockSize) {
  GLuint uniformBlockIndex = glGetUniformBlockIndex(program, blockName);
  glUniformBlockBinding(program, uniformBlockIndex, 0);

  // ubo
  glGenBuffers(1, &ubo_);
  glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
  glBufferData(GL_UNIFORM_BUFFER, blockSize, nullptr, GL_STATIC_DRAW);
  glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo_, 0, blockSize);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

bool UniformsGLSL::Empty() const {
  return 0 == ubo_;
}

UniformsGLSL::~UniformsGLSL() {
  glDeleteBuffers(1, &ubo_);
}

void BasicUniforms::Init(GLuint program) {
  Create(program, "Uniforms", sizeof(glm::mat4) + sizeof(glm::vec4));
}

void BasicUniforms::UpdateData() {
  glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &u_modelViewProjectionMatrix[0][0]);
  glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::vec4), &u_fragColor[0]);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void RendererOpenGL::Create(int w, int h, float near, float far) {
  Renderer::Create(w, h, near, far);
}

void RendererOpenGL::Clear(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RendererOpenGL::DrawMeshTextured(ModelMesh &mesh) {

}

void RendererOpenGL::DrawMeshWireframe(ModelMesh &mesh) {

}

void RendererOpenGL::DrawLines(ModelLines &lines) {
  if (!lines.handle) {
    auto handle = std::make_shared<VertexGLSL>();
    handle->Create(lines.vertexes, lines.indices);
    lines.handle = std::move(handle);
  }

  if (program_basic.Empty()) {
    program_basic.LoadSource(BASIC_VS, BASIC_FS);
  }

  if (uniforms_basic.Empty()) {
    uniforms_basic.Init(program_basic.GetId());
  }

  program_basic.Use();
  uniforms_basic.UpdateData();
  ((VertexGLSL *) (lines.handle.get()))->BindVAO();

  glLineWidth(lines.line_width);
  glDrawElements(GL_LINES, lines.indices.size(), GL_UNSIGNED_INT, nullptr);
}

void RendererOpenGL::DrawPoints(ModelPoints &points) {
  if (!points.handle) {
    auto handle = std::make_shared<VertexGLSL>();
    handle->Create(points.vertexes, points.indices);
    points.handle = std::move(handle);
  }

  if (program_basic.Empty()) {
    program_basic.LoadSource(BASIC_VS, BASIC_FS);
  }

  if (uniforms_basic.Empty()) {
    uniforms_basic.Init(program_basic.GetId());
  }

  program_basic.Use();
  uniforms_basic.UpdateData();
  ((VertexGLSL *) (points.handle.get()))->BindVAO();

  glPointSize(points.point_size);
  glDrawElements(GL_POINTS, points.indices.size(), GL_UNSIGNED_INT, nullptr);
}

}
