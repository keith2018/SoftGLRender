/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/glm_inc.h"

namespace SoftGL {
namespace Image {

#define kMaxSamplerLodBias 64.f
#define kMaxSamplerLod 64.f

enum ImageFormat {
  FORMAT_UNDEFINED = 0,
  FORMAT_R8_UNORM = 9,
  FORMAT_R8G8_UNORM = 16,
  FORMAT_R8G8B8_UNORM = 23,
  FORMAT_R8G8B8A8_UNORM = 37,
};

enum ImageBorderColor {
  BORDER_COLOR_FLOAT_TRANSPARENT_BLACK = 0,
  BORDER_COLOR_INT_TRANSPARENT_BLACK = 1,
  BORDER_COLOR_FLOAT_OPAQUE_BLACK = 2,
  BORDER_COLOR_INT_OPAQUE_BLACK = 3,
  BORDER_COLOR_FLOAT_OPAQUE_WHITE = 4,
  BORDER_COLOR_INT_OPAQUE_WHITE = 5,
};

enum ImageDim {
  Dim1D = 0,
  Dim2D = 1,
  Dim3D = 2,
  DimCube = 3,
  SpvDimMax = 0x7fffffff,
};

struct ImageInfo {
  ImageDim dim;
  ImageFormat format;
  int32_t width;
  int32_t height;
  int32_t depth;
  bool mipmaps;
  int32_t baseMipLevel;
  int32_t mipLevels;
  bool arrayed;
  int32_t baseArrayLayer;
  int32_t arrayLayers;
  int32_t samples;
};

enum SamplerFilterMode {
  SpvSamplerFilterModeNearest = 0,
  SpvSamplerFilterModeLinear = 1,
  SpvSamplerFilterModeMax = 0x7fffffff,
};

enum SamplerAddressingMode {
  SpvSamplerAddressingModeNone = 0,
  SpvSamplerAddressingModeClampToEdge = 1,
  SpvSamplerAddressingModeClamp = 2,
  SpvSamplerAddressingModeRepeat = 3,
  SpvSamplerAddressingModeRepeatMirrored = 4,
  SpvSamplerAddressingModeMax = 0x7fffffff,
};

struct SamplerAttribute {
  SamplerAddressingMode addressingMode;
  bool normalized;
  SamplerFilterMode filterMode;
};

struct SamplerInfo {
  SamplerFilterMode magFilter;
  SamplerFilterMode minFilter;
  SamplerFilterMode mipmapMode;
  SamplerAddressingMode addressModeU;
  SamplerAddressingMode addressModeV;
  SamplerAddressingMode addressModeW;
  float mipLodBias;
  bool compareEnable;
  float minLod;
  float maxLod;
  ImageBorderColor borderColor;
  bool unnormalizedCoordinates;
};

struct Sampler {
  SamplerInfo info;
};

struct ImageLevel {
  int32_t level;
  int32_t width;
  int32_t height;
  int32_t depth;
  int32_t dataSize;
  uint8_t *data;
};

struct ImageLayer {
  int32_t layer;
  int32_t width;
  int32_t height;
  int32_t depth;
  int32_t levelCount;
  ImageLevel *levels;
};

struct Image {
  ImageInfo info;
  int32_t layerCount;
  ImageLayer *layers;
};

struct SampledImage {
  Image *image;
  Sampler *sampler;
};

struct ImageOperands {
  float *bias;
  float *lod;
  glm::vec4 *dx, *dy;
  float *sample;
  float *minLod;
  glm::vec4 *offset;
};

class ImageContext {
 public:
  virtual glm::vec4 GetDPdx(int32_t P) = 0;
  virtual glm::vec4 GetDPdy(int32_t P) = 0;
};

Image *CreateImage(ImageInfo *imageInfo);
Sampler *CreateSampler(SamplerInfo *samplerInfo = nullptr);
Sampler *CreateSamplerConstant(SamplerAttribute *attribute);
SampledImage *CreateSampledImage(Image *image, Sampler *sampler);

void DestroyImage(Image *image);
void DestroySampler(Sampler *sampler);
void DestroySampledImager(SampledImage *sampledImage);

void GenerateMipmaps(Image *image, SamplerFilterMode filterMode);
void GenerateMipmaps(SampledImage *sampledImage);

bool UploadImageData(Image *image, uint8_t *dataPtr, int32_t dataSize,
                     int32_t width, int32_t height, int32_t depth = 1,
                     int32_t mipLevel = 0, int32_t arrayLayer = 0);

glm::vec4 SampleImage(ImageContext *ctx,
                      Image *image,
                      Sampler *sampler,
                      glm::vec4 uvwa,
                      int32_t coordinateId,
                      ImageOperands *operands);

glm::vec4 FetchImage(Image *image,
                     glm::ivec4 uvwa,
                     ImageOperands *operands);

glm::ivec4 QueryImageSizeLod(Image *image, int32_t lod);
glm::ivec4 QueryImageSize(Image *image);
glm::vec2 QueryImageLod(ImageContext *ctx,
                        Image *image,
                        Sampler *sampler,
                        int32_t coordinateId);

}
}
