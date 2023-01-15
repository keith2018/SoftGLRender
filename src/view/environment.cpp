/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "environment.h"
#include "camera.h"

namespace SoftGL {
namespace View {

#define IrradianceMapSize 32
#define PrefilterMaxMipLevels 5
#define PrefilterMapSize 128

struct LookAtParam {
  glm::vec3 eye;
  glm::vec3 center;
  glm::vec3 up;
};

void Environment::ConvertEquirectangular(Texture2D &tex_in, TextureCube &tex_out, int width, int height) {

}

void Environment::GenerateIrradianceMap(TextureCube &tex_in, TextureCube &tex_out) {

}

void Environment::GeneratePrefilterMap(TextureCube &tex_in, TextureCube &tex_out) {

}

}
}
