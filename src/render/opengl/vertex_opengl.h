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
    if (!vertex_array.vertexes_buffer || !vertex_array.indices_buffer) {
      return;
    }
    indices_cnt_ = vertex_array.indices_buffer_length / sizeof(int32_t);

    // vao
    GL_CHECK(glGenVertexArrays(1, &vao_));
    GL_CHECK(glBindVertexArray(vao_));

    // vbo
    GL_CHECK(glGenBuffers(1, &vbo_));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          vertex_array.vertexes_buffer_length,
                          vertex_array.vertexes_buffer,
                          GL_STATIC_DRAW));

    for (int i = 0; i < vertex_array.vertexes_desc.size(); i++) {
      auto &desc = vertex_array.vertexes_desc[i];
      GL_CHECK(glVertexAttribPointer(i, desc.size, GL_FLOAT, GL_FALSE, desc.stride, (void *) desc.offset));
      GL_CHECK(glEnableVertexAttribArray(i));
    }

    // ebo
    GL_CHECK(glGenBuffers(1, &ebo_));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                          vertex_array.indices_buffer_length,
                          vertex_array.indices_buffer,
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

  void UpdateVertexData(void *data, size_t length) override {
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, length, data, GL_STATIC_DRAW));
  }

  int GetId() const override {
    return (int) vao_;
  }

  void Bind() const {
    if (vao_) {
      GL_CHECK(glBindVertexArray(vao_));
    }
  }

  size_t GetIndicesCnt() const {
    return indices_cnt_;
  }

 private:
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint ebo_ = 0;
  size_t indices_cnt_ = 0;
};

}
