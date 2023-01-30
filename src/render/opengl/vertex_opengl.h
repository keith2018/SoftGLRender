/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "render/vertex.h"
#include "render/opengl/opengl_utils.h"


namespace SoftGL {

class VertexArrayObjectOpenGL : public VertexArrayObject {
 public:
  explicit VertexArrayObjectOpenGL(const VertexArray &vertex_array) {
    if (vertex_array.vertexes.empty() || vertex_array.indices.empty()) {
      return;
    }

    // vao
    GL_CHECK(glGenVertexArrays(1, &vao_));
    GL_CHECK(glBindVertexArray(vao_));

    // vbo
    GL_CHECK(glGenBuffers(1, &vbo_));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          vertex_array.vertexes.size() * sizeof(Vertex),
                          &vertex_array.vertexes[0],
                          GL_STATIC_DRAW));

    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, a_texCoord)));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, a_normal)));
    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, a_tangent)));
    GL_CHECK(glEnableVertexAttribArray(3));

    // ebo
    GL_CHECK(glGenBuffers(1, &ebo_));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                          vertex_array.indices.size() * sizeof(int),
                          &vertex_array.indices[0],
                          GL_STATIC_DRAW));
  }

  ~VertexArrayObjectOpenGL() {
    if (vbo_) {
      GL_CHECK(glDeleteBuffers(1, &vbo_));
    }
    if (ebo_) {
      GL_CHECK(glDeleteBuffers(1, &ebo_));
    }
    if (vao_) {
      GL_CHECK(glDeleteVertexArrays(1, &vao_));
    }
  }

  void UpdateVertexData(std::vector<Vertex> &vertexes) override {
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, vertexes.size() * sizeof(Vertex), &vertexes[0], GL_STATIC_DRAW));
  }

  int GetId() const override {
    return (int) vao_;
  }

  void Bind() {
    if (vao_) {
      GL_CHECK(glBindVertexArray(vao_));
    }
  }

 private:
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint ebo_ = 0;
};

}