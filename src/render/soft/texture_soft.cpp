/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#include "texture_soft.h"
#include "sampler_soft.h"

namespace SoftGL {

int Texture2DSoft::uuid_counter_ = 0;
int TextureCubeSoft::uuid_counter_ = 0;
int TextureDepthSoft::uuid_counter_ = 0;

void TextureImageSoft::GenerateMipmap(bool sample) {
  BaseSampler<uint8_t>::GenerateMipmaps(this, sample);
}

}
