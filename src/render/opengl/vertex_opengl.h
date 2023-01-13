/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "render/vertex.h"

namespace SoftGL {

class VertexArrayObjectOpenGL : public VertexArrayObject {
 public:
  explicit VertexArrayObjectOpenGL(VertexArray &vertex_array) {
    if (vertex_array.vertexes.empty() || vertex_array.indices.empty()) {
      return;
    }

    // vao
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // vbo
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertex_array.vertexes.size() * sizeof(Vertex),
                 &vertex_array.vertexes[0],
                 GL_STATIC_DRAW);

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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 vertex_array.indices.size() * sizeof(int),
                 &vertex_array.indices[0],
                 GL_STATIC_DRAW);
  }

  ~VertexArrayObjectOpenGL() {
    if (vbo_) {
      glDeleteBuffers(1, &vbo_);
    }
    if (ebo_) {
      glDeleteBuffers(1, &ebo_);
    }
    if (vao_) {
      glDeleteVertexArrays(1, &vao_);
    }
  }

  void UpdateVertexData(std::vector<Vertex> &vertexes) override {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertexes.size() * sizeof(Vertex), &vertexes[0], GL_STATIC_DRAW);
  }

  inline GLuint GetId() const {
    return vao_;
  }

  void Bind() override {
    if (vao_) {
      glBindVertexArray(vao_);
    }
  }

 private:
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint ebo_ = 0;
};

}