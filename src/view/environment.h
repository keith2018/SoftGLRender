/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/texture.h"

namespace SoftGL {
namespace View {

class Environment {
 public:
  static void ConvertEquirectangular(Texture2D &tex_in, TextureCube &tex_out, int width = 0, int height = 0);

  static void GenerateIrradianceMap(TextureCube &tex_in, TextureCube &tex_out);

  static void GeneratePrefilterMap(TextureCube &tex_in, TextureCube &tex_out);
};

}
}
