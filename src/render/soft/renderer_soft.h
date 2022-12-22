/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "../renderer.h"
#include "base/thread_pool.h"
#include "shader.h"
#include "graphic.h"
#include "render_context.h"

namespace SoftGL {

class RendererSoft : public Renderer {
 public:
  void Create(int width, int height, float near, float far) override;
  void Clear(float r, float g, float b, float a) override;

  void DrawMeshTextured(ModelMesh &mesh) override;
  void DrawMeshWireframe(ModelMesh &mesh) override;
  void DrawLines(ModelLines &lines) override;
  void DrawPoints(ModelPoints &points) override;

  inline std::shared_ptr<Buffer<glm::u8vec4>> &GetFrameColor() { return fbo_.color; };
  inline ShaderContext &GetShaderContext() { return shader_context_; }

 private:
  void ProcessVertexShader();
  void ProcessFaceFrustumClip();
  void ProcessFaceScreenMapping();
  void ProcessFaceFrontBackCull();
  void ProcessFaceRasterization();
  void ProcessFaceWireframe(bool clip_space);

  void VertexShaderImpl(VertexHolder &vh);
  void TriangularRasterization(FaceHolder &face_holder, bool early_z_pass);
  void PixelQuadBarycentric(PixelQuadContext &quad, bool early_z_pass);
  void BarycentricCorrect(PixelQuadContext &quad);
  void PixelShading(glm::vec4 &screen_pos, bool front_facing, BaseFragmentShader *frag_shader);

  inline void DrawFramePointWithDepth(int x, int y, float depth, const glm::u8vec4 &color);
  inline void DrawFramePoint(int x, int y, const glm::u8vec4 &color);
  inline bool DepthTestPoint(int x, int y, float depth);
  void DrawLineFrustumClip(glm::vec4 p0, int mask0, glm::vec4 p1, int mask1, const glm::u8vec4 &color);

  void ViewportTransform(glm::vec4 &pos) const;
  static glm::vec4 Viewport(float x, float y, float w, float h);
  static void PerspectiveDivide(glm::vec4 &pos);
  static int CountFrustumClipMask(glm::vec4 &clip_pos);
  static BoundingBox VertexBoundingBox(glm::vec4 *vert, size_t w, size_t h);

  static void VaryingInterpolate(float *out_vary,
                                 const float *in_varyings[],
                                 size_t elem_cnt,
                                 glm::aligned_vec4 &bc);

 public:
  DepthRange depth_range;
  DepthFunc depth_func;
  bool depth_test = true;
  bool depth_mask = true;
  bool frustum_clip = true;
  bool cull_face_back = true;
  bool wireframe_show_clip = true;
  bool blend_enabled = true;
  bool early_z = true;

 private:
  glm::vec4 viewport_;
  FrameBuffer fbo_;

  Graphic graphic_;
  RenderContext render_ctx_;
  ShaderContext shader_context_;

  int raster_block_size_ = 32;
  ThreadPool thread_pool_;
};

}
