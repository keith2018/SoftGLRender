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

struct Viewport {
  float x;
  float y;
  float width;
  float height;
  float depth_near;
  float depth_far;

  // ref: https://registry.khronos.org/vulkan/specs/1.0/html/chap24.html#vertexpostproc-viewport
  glm::vec4 inner_o;
  glm::vec4 inner_p;
};

struct VertexHolder {
  bool discard;
  size_t index;
  int clip_mask;
  glm::vec4 position;
};

struct PrimitiveHolder {
  bool discard;
  VertexHolder *vertexes[3];
  bool front_facing;
};

class RendererSoft : public Renderer {
 public:
  RendererSoft() {
    viewport_.depth_near = 0.f;
    viewport_.depth_far = 1.f;
  }

  // config
  bool ReverseZ() const override {
    return false;
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
  std::shared_ptr<UniformSampler> CreateUniformSampler(const std::string &name, TextureType type) override;

  // pipeline
  void SetFrameBuffer(FrameBuffer &frame_buffer) override;
  void SetViewPort(int x, int y, int width, int height) override;
  void Clear(const ClearState &state) override;
  void SetRenderState(const RenderState &state) override;
  void SetVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) override;
  void SetShaderProgram(std::shared_ptr<ShaderProgram> &program) override;
  void SetShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) override;
  void Draw(PrimitiveType type) override;

 private:
  void ProcessVertexShader();
  void ProcessPrimitiveAssembly();
  void ProcessClipping();
  void ProcessPerspectiveDivide();
  void ProcessViewportTransform();
  void ProcessFaceCulling();
  void ProcessRasterization();
  void ProcessFragmentShader(glm::vec4 &screen_pos, bool front_facing);
  void ProcessPerSampleOperations(int x, int y, ShaderBuiltin &builtin);
  bool ProcessDepthTest(int x, int y, float depth);
  void ProcessColorBlending(int x, int y, glm::vec4 &color);

  void ClippingPoint(PrimitiveHolder &point);
  void ClippingLine(PrimitiveHolder &line);
  void ClippingTriangle(PrimitiveHolder &triangle);

  void RasterizationPolygon(PrimitiveHolder &primitive);
  void RasterizationPoint(glm::vec4 &pos, float point_size);
  void RasterizationLine(glm::vec4 &pos0, glm::vec4 &pos1, float line_width);
  void RasterizationTriangle(glm::vec4 &pos0, glm::vec4 &pos1, glm::vec4 &pos2, bool front_facing);

 private:
  inline glm::u8vec4 GetFrameColor(int x, int y);
  inline void SetFrameColor(int x, int y, const glm::u8vec4 &color);

  int CountFrustumClipMask(glm::vec4 &clip_pos);

 private:
  Viewport viewport_;
  PrimitiveType primitive_type_;
  FrameBufferSoft *fbo_ = nullptr;
  const RenderState *render_state_ = nullptr;
  VertexArrayObjectSoft *vao_ = nullptr;
  ShaderProgramSoft *shader_program_ = nullptr;

  std::shared_ptr<BufferRGBA> fbo_color_ = nullptr;
  std::shared_ptr<BufferDepth> fbo_depth_ = nullptr;

  std::vector<VertexHolder> vertexes_;
  std::vector<PrimitiveHolder> primitives_;
};

}
