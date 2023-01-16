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
  static bool ConvertEquirectangular(std::shared_ptr<Texture2D> &tex_in,
                                     std::shared_ptr<TextureCube> &tex_out);

  static bool GenerateIrradianceMap(std::shared_ptr<TextureCube> &tex_in,
                                    std::shared_ptr<TextureCube> &tex_out);

  static bool GeneratePrefilterMap(std::shared_ptr<TextureCube> &tex_in,
                                   std::shared_ptr<TextureCube> &tex_out);
};

}
}
