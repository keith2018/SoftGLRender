/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/memory_utils.h"
#include "render/renderer.h"
#include "render/soft/vertex_soft.h"
#include "render/soft/framebuffer_soft.h"
#include "render/soft/shader_program_soft.h"
#include "render/bounding_box.h"

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
  float *varyings;
  int clip_mask;
  glm::vec4 position;
};

struct PrimitiveHolder {
  bool discard;
  VertexHolder *vertexes[3];
  bool front_facing;
};

struct PixelContext {
  bool inside = false;
  float *varyings_frag;
  glm::aligned_vec4 position;
  glm::aligned_vec4 barycentric;
};

class PixelQuadContext {
 public:
  void Init(int x, int y, size_t varyings_size);
  bool QuadInside();

 public:
  /**
   *   p2--p3
   *   |   |
   *   p0--p1
   */
  PixelContext pixels[4];

  // triangle vertex screen space position
  glm::aligned_vec4 screen_pos[3];
  glm::aligned_vec4 vert_flat[4];

  // triangle vertex clip space z, clip_z = { v0.clip_z, v1.clip_z, v2.clip_z, 1.f}
  glm::aligned_vec4 clip_z{1.f};

  // triangle Facing
  bool front_facing = true;

  // triangle vertex shader varyings
  const float *varyings_vert[3];

 private:
  std::shared_ptr<float> varyings_pool_;
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
  void ProcessFragmentShader(glm::vec4 &screen_pos, bool front_facing, void *varyings);
  void ProcessPerSampleOperations(int x, int y, ShaderBuiltin &builtin);
  bool ProcessDepthTest(int x, int y, float depth);
  void ProcessColorBlending(int x, int y, glm::vec4 &color);

  void ClippingPoint(PrimitiveHolder &point);
  void ClippingLine(PrimitiveHolder &line);
  void ClippingTriangle(PrimitiveHolder &triangle);

  void RasterizationPolygon(PrimitiveHolder &primitive);
  void RasterizationPoint(glm::vec4 &pos, float point_size);
  void RasterizationLine(glm::vec4 &pos0, glm::vec4 &pos1, float line_width);
  void RasterizationTriangle(PrimitiveHolder &triangle);
  void RasterizationPixelQuad(PixelQuadContext &quad);

 private:
  inline glm::u8vec4 GetFrameColor(int x, int y);
  inline void SetFrameColor(int x, int y, const glm::u8vec4 &color);

  int CountFrustumClipMask(glm::vec4 &clip_pos);
  BoundingBox TriangleBoundingBox(glm::vec4 *vert, float width, float height);
  bool Barycentric(glm::aligned_vec4 *vert, glm::aligned_vec4 &v0, glm::aligned_vec4 &p, glm::aligned_vec4 &bc);
  void BarycentricCorrect(PixelQuadContext &quad);
  void VaryingsInterpolate(float *out_vary,
                           const float *in_varyings[],
                           size_t elem_cnt,
                           glm::aligned_vec4 &bc);

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

  std::shared_ptr<float> varyings_ = nullptr;
  size_t varyings_cnt_ = 0;
  size_t varyings_buffer_size_ = 0;

  int raster_block_size_ = 32;
};

}
