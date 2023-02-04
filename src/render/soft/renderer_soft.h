/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/renderer.h"
#include "render/soft/vertex_soft.h"
#include "render/soft/framebuffer_soft.h"
#include "render/soft/shader_program_soft.h"

namespace SoftGL {

struct DepthRange {
  float near = 0.f;
  float far = 1.f;
  float diff = 1.f;   // far - near
  float sum = 1.f;    // far + near
};

struct VertexHolder {
  size_t index = 0;
  bool discard = true;  // default true, set false via ProcessClipping()
  glm::vec4 position;
};

struct Primitive {
  bool discard = false;    // default true, set false via ProcessClipping()
};

struct PrimitivePoint : Primitive {
  VertexHolder *vertexes[1];
};

struct PrimitiveLine : Primitive {
  VertexHolder *vertexes[2];
};

struct PrimitiveTriangle : Primitive {
  VertexHolder *vertexes[3];
  bool front_facing = true;
};

class RendererSoft : public Renderer {
 public:
  // config
  bool ReverseZ() const override {
    return true;
  }

  // framebuffer
  std::shared_ptr<FrameBuffer> CreateFrameBuffer() override;

  // texture
  std::shared_ptr<Texture2D> CreateTexture2D() override;
  std::shared_ptr<TextureCube> CreateTextureCube() override;
  std::shared_ptr<TextureDepth> CreateTextureDepth() override;

  // vertex
  std::shared_ptr<VertexArrayObject> CreateVertexArrayObject(const VertexArray &vertex_array) override;

  // shader program
  std::shared_ptr<ShaderProgram> CreateShaderProgram() override;

  // uniform
  std::shared_ptr<UniformBlock> CreateUniformBlock(const std::string &name, int size) override;
  std::shared_ptr<UniformSampler> CreateUniformSampler(const std::string &name) override;

  // pipeline
  void SetFrameBuffer(FrameBuffer &frame_buffer) override;
  void SetViewPort(int x, int y, int width, int height) override;
  void Clear(const ClearState &state) override;
  void SetRenderState(const RenderState &state) override;
  void SetVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) override;
  void SetShaderProgram(std::shared_ptr<ShaderProgram> &program) override;
  void SetShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) override;
  void Draw(PrimitiveType type) override;

 public:
  void SetDepthRange(float near, float far);

 private:
  void ProcessVertexShader();
  void ProcessPrimitiveAssembly();
  void ProcessFaceCulling();
  void ProcessClipping();
  void ProcessPerspectiveDivide();
  void ProcessViewportTransform();
  void ProcessRasterization();
  void ProcessFragmentShader(glm::vec4 &screen_pos, bool front_facing);
  bool ProcessDepthTest(int x, int y, float depth);

  void RasterizationPoint();
  void RasterizationLine();
  void RasterizationTriangle();

  inline glm::u8vec4 GetFrameColor(int x, int y);
  inline void SetFrameColor(int x, int y, const glm::u8vec4 &color);

  inline static bool DepthTest(float &a, float &b, DepthFunc func);

 private:
  DepthRange depth_range_;
  glm::vec4 viewport_;
  PrimitiveType primitive_type_;
  FrameBufferSoft *fbo_ = nullptr;
  const RenderState *render_state_ = nullptr;
  VertexArrayObjectSoft *vao_ = nullptr;
  ShaderProgramSoft *shader_program_ = nullptr;

  std::shared_ptr<BufferRGBA> fbo_color_ = nullptr;
  std::shared_ptr<BufferDepth> fbo_depth_ = nullptr;

  std::vector<VertexHolder> vertexes_;
  std::vector<PrimitivePoint> primitive_points_;
  std::vector<PrimitiveLine> primitive_lines_;
  std::vector<PrimitiveTriangle> primitive_triangles_;
};

}
