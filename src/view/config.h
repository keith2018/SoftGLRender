/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include "base/glm_inc.h"

namespace SoftGL {
namespace View {

enum AAType {
  AAType_NONE,
  AAType_SSAA,
  AAType_FXAA,
};

enum RendererType {
  Renderer_SOFT,
  Renderer_OPENGL,
};

class Config {
 public:
  std::string model_name;
  std::string model_path;
  std::string skybox_name;
  std::string skybox_path;

  size_t triangle_count_ = 0;

  bool wireframe = false;
  bool wireframe_show_clip = false;
  bool world_axis = true;
  bool show_skybox = false;
  bool cull_face = true;
  bool depth_test = true;
  bool early_z = false;

  glm::vec4 clear_color = {0.f, 0.f, 0.f, 0.f};
  glm::vec3 ambient_color = {0.5f, 0.5f, 0.5f};

  bool show_light = true;
  glm::vec3 point_light_position = {0.f, 0.f, 0.f};
  glm::vec3 point_light_color = {0.5f, 0.5f, 0.5f};

  int aa_type = AAType_NONE;
  int renderer_type = Renderer_SOFT;
};

}
}
