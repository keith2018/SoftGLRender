/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/render_state.h"

namespace SoftGL {

template<typename T>
T CalcBlendFactor(const T &src, const float &src_alpha,
                  const T &dst, const float &dst_alpha,
                  const BlendFactor &factor) {
  switch (factor) {
    case BlendFactor_ZERO:                return T(0.f);
    case BlendFactor_ONE:                 return T(1.f);
    case BlendFactor_SRC_COLOR:           return src;
    case BlendFactor_SRC_ALPHA:           return T(src_alpha);
    case BlendFactor_DST_COLOR:           return dst;
    case BlendFactor_DST_ALPHA:           return T(dst_alpha);
    case BlendFactor_ONE_MINUS_SRC_COLOR: return T(1.f) - src;
    case BlendFactor_ONE_MINUS_SRC_ALPHA: return T(1.f - src_alpha);
    case BlendFactor_ONE_MINUS_DST_COLOR: return T(1.f) - dst;
    case BlendFactor_ONE_MINUS_DST_ALPHA: return T(1.f - dst_alpha);
  }
  return T(0.f);
}

template<typename T>
T CalcBlendFunc(const T &src, const T &dst, const BlendFunction &func) {
  switch (func) {
    case BlendFunc_ADD:               return src + dst;
    case BlendFunc_SUBTRACT:          return src - dst;
    case BlendFunc_REVERSE_SUBTRACT:  return dst - src;
    case BlendFunc_MIN:               return glm::min(src, dst);
    case BlendFunc_MAX:               return glm::max(src, dst);
  }
  return src + dst;
}

glm::vec4 CalcBlendColor(glm::vec4 &src, glm::vec4 &dst, const BlendParameters &params) {
  auto src_rgb = glm::vec3(src);
  auto dst_rgb = glm::vec3(dst);
  auto src_rgb_f = CalcBlendFactor<glm::vec3>(src_rgb, src.a, dst_rgb, dst.a, params.blend_src_rgb);
  auto dst_rgb_f = CalcBlendFactor<glm::vec3>(src_rgb, src.a, dst_rgb, dst.a, params.blend_dst_rgb);
  auto ret_rgb = CalcBlendFunc<glm::vec3>(src_rgb * src_rgb_f, dst_rgb * dst_rgb_f, params.blend_func_rgb);

  auto src_alpha_f = CalcBlendFactor<float>(src.a, src.a, dst.a, dst.a, params.blend_src_alpha);
  auto dst_alpha_f = CalcBlendFactor<float>(src.a, src.a, dst.a, dst.a, params.blend_dst_alpha);
  auto ret_alpha = CalcBlendFunc<float>(src.a * src_alpha_f, dst.a * dst_alpha_f, params.blend_func_alpha);

  return {ret_rgb, ret_alpha};
}

}
