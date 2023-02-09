/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "image_soft.h"

#include <cmath>
#include "base/logger.h"

namespace SoftGL {
namespace Image {

// image coordinates(i,j,k) with sample weight
struct ImageCoord {
  int32_t coord;
  float weight;
};

#define ImageCoordNull {0, 1.f}

/**
 * image format bytes per pixel
 */
int32_t GetImageFormatBPP(ImageFormat format) {
  switch (format) {
    case FORMAT_R8_UNORM:
      return 1;
    case FORMAT_R8G8_UNORM:
      return 2;
    case FORMAT_R8G8B8_UNORM:
      return 3;
    case FORMAT_R8G8B8A8_UNORM:
      return 4;
    default:
      LOGE("image format not support");
      break;
  }
  return 0;
}

glm::vec4 GetBorderColorRGBA(ImageBorderColor borderColor) {
  switch (borderColor) {
    case BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
    case BORDER_COLOR_INT_TRANSPARENT_BLACK:
      return {0, 0, 0, 0};
    case BORDER_COLOR_FLOAT_OPAQUE_BLACK:
    case BORDER_COLOR_INT_OPAQUE_BLACK:
      return {0, 0, 0, 1};
    case BORDER_COLOR_FLOAT_OPAQUE_WHITE:
    case BORDER_COLOR_INT_OPAQUE_WHITE:
      return {1, 1, 1, 1};
    default:
      break;
  }
  return {0, 0, 0, 0};
}

glm::vec4 ReadPixelColorRGBA(ImageFormat format, const uint8_t *texelPtr) {
  glm::vec4 texel;
  switch (format) {
    case FORMAT_R8_UNORM: {
      texel.r = (float) texelPtr[0] / 255.f;
      texel.g = 0.f;
      texel.b = 0.f;
      texel.a = 1.f;
      break;
    }
    case FORMAT_R8G8_UNORM: {
      texel.r = (float) texelPtr[0] / 255.f;
      texel.g = (float) texelPtr[1] / 255.f;
      texel.b = 0.f;
      texel.a = 1.f;
      break;
    }
    case FORMAT_R8G8B8_UNORM: {
      texel.r = (float) texelPtr[0] / 255.f;
      texel.g = (float) texelPtr[1] / 255.f;
      texel.b = (float) texelPtr[2] / 255.f;
      texel.a = 1.f;
      break;
    }
    case FORMAT_R8G8B8A8_UNORM: {
      texel.r = (float) texelPtr[0] / 255.f;
      texel.g = (float) texelPtr[1] / 255.f;
      texel.b = (float) texelPtr[2] / 255.f;
      texel.a = (float) texelPtr[3] / 255.f;
      break;
    }
    default:
      LOGE("image format not support");
      break;
  }

  return texel;
}

void WritePixelColorRGBA(ImageFormat format, uint8_t *dstTexelPtr, glm::vec4 srcTexel) {
  switch (format) {
    case FORMAT_R8_UNORM: {
      dstTexelPtr[0] = (uint8_t) (srcTexel.r * 255);
      break;
    }
    case FORMAT_R8G8_UNORM: {
      dstTexelPtr[0] = (uint8_t) (srcTexel.r * 255);
      dstTexelPtr[1] = (uint8_t) (srcTexel.g * 255);
      break;
    }
    case FORMAT_R8G8B8_UNORM: {
      dstTexelPtr[0] = (uint8_t) (srcTexel.r * 255);
      dstTexelPtr[1] = (uint8_t) (srcTexel.g * 255);
      dstTexelPtr[2] = (uint8_t) (srcTexel.b * 255);
      break;
    }
    case FORMAT_R8G8B8A8_UNORM: {
      dstTexelPtr[0] = (uint8_t) (srcTexel.r * 255);
      dstTexelPtr[1] = (uint8_t) (srcTexel.g * 255);
      dstTexelPtr[2] = (uint8_t) (srcTexel.b * 255);
      dstTexelPtr[3] = (uint8_t) (srcTexel.a * 255);
      break;
    }
    default:
      LOGE("image format not support");
      break;
  }
}

int32_t GetAddressMirror(int32_t idx) {
  return idx >= 0 ? idx : -(1 + idx);
}

int32_t GetWrappingAddress(int32_t idx, int32_t size, SamplerAddressingMode wrappingMode) {
  switch (wrappingMode) {
    case SpvSamplerAddressingModeClampToEdge:
      idx = glm::clamp(idx, 0, size - 1);
      break;
    case SpvSamplerAddressingModeClamp:
      idx = glm::clamp(idx, -1, size);
      break;
    case SpvSamplerAddressingModeRepeat:
      idx %= size;
      break;
    case SpvSamplerAddressingModeRepeatMirrored:
      idx = (size - 1) - GetAddressMirror((idx % (2 * size)) - size);
      break;
    default:
      break;
  }

  return idx;
}

inline bool CheckCoordinateValid(int32_t i, int32_t j, int32_t k, ImageLevel *imageLevel) {
  if (i < 0 || i >= imageLevel->width
      || j < 0 || j >= imageLevel->height
      || k < 0 || k >= imageLevel->depth) {
    return false;
  }
  return true;
}

int32_t GetImageDimCoordinatesCount(ImageDim dim) {
  switch (dim) {
    case Dim1D:
      return 1;
    case Dim2D:
      return 2;
    case Dim3D:
    case DimCube:
      return 3;
    default:
      LOGE("GetImageDimCoordinatesCount, image dim not support");
      break;
  }
  return 0;
}

uint8_t *PixelDataPtr(ImageLevel *imageLevel, int32_t i, int32_t j, int32_t k, ImageFormat imageFormat) {
  int32_t ptrOffset = k * (imageLevel->width * imageLevel->height)
      + j * imageLevel->width
      + i;
  return imageLevel->data + ptrOffset * GetImageFormatBPP(imageFormat);
}

glm::vec4 SampleImageLevel(ImageLevel *imageLevel, ImageCoord iIdx, ImageCoord jIdx, ImageCoord kIdx,
                           SamplerInfo *samplerInfo, ImageFormat imageFormat) {
  if (samplerInfo) {
    iIdx.coord = GetWrappingAddress(iIdx.coord, (int32_t) imageLevel->width, samplerInfo->addressModeU);
    jIdx.coord = GetWrappingAddress(jIdx.coord, (int32_t) imageLevel->height, samplerInfo->addressModeV);
    kIdx.coord = GetWrappingAddress(kIdx.coord, (int32_t) imageLevel->depth, samplerInfo->addressModeW);
  }

  glm::vec4 texel;
  if (!CheckCoordinateValid(iIdx.coord, jIdx.coord, kIdx.coord, imageLevel)) {
    texel = GetBorderColorRGBA(samplerInfo ? samplerInfo->borderColor : (ImageBorderColor) 0);
  } else {
    uint8_t *texelPtr = PixelDataPtr(imageLevel, iIdx.coord, jIdx.coord, kIdx.coord, imageFormat);
    texel = ReadPixelColorRGBA(imageFormat, texelPtr);
  }

  texel *= iIdx.weight * jIdx.weight * kIdx.weight;
  return texel;
}

glm::vec4 SampleImageNearest(ImageLevel *imageLevel,
                             glm::vec4 uvwa,
                             ImageInfo *imageInfo,
                             SamplerInfo *samplerInfo) {
  ImageCoord iIdx{(int32_t) floor(uvwa.r), 1.f};
  ImageCoord jIdx{(int32_t) floor(uvwa.g), 1.f};
  ImageCoord kIdx{(int32_t) floor(uvwa.b), 1.f};

  return SampleImageLevel(imageLevel, iIdx, jIdx, kIdx, samplerInfo, imageInfo->format);
}

glm::vec4 SampleImageLinear(ImageLevel *imageLevel,
                            glm::vec4 uvwa,
                            ImageInfo *imageInfo,
                            SamplerInfo *samplerInfo) {
  ImageCoord iIdx0{(int32_t) glm::floor(uvwa.r - 0.5f), 1.f};
  ImageCoord jIdx0{(int32_t) glm::floor(uvwa.g - 0.5f), 1.f};
  ImageCoord kIdx0{(int32_t) glm::floor(uvwa.b - 0.5f), 1.f};

  ImageCoord iIdx1 = {iIdx0.coord + 1, glm::fract(uvwa.r - 0.5f)};
  ImageCoord jIdx1 = {jIdx0.coord + 1, glm::fract(uvwa.g - 0.5f)};
  ImageCoord kIdx1 = {kIdx0.coord + 1, glm::fract(uvwa.b - 0.5f)};

  iIdx0.weight = 1.f - iIdx1.weight;
  jIdx0.weight = 1.f - jIdx1.weight;
  kIdx0.weight = 1.f - kIdx1.weight;

#define ADD_IMAGE_SAMPLE_TEXEL(i, j, k) \
  retTexel += SampleImageLevel(imageLevel, i, j, k, samplerInfo, imageInfo->format)

  glm::vec4 retTexel{0, 0, 0, 0};
  switch (imageInfo->dim) {
    case Dim1D: {
      ADD_IMAGE_SAMPLE_TEXEL(iIdx0, ImageCoordNull, ImageCoordNull);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx1, ImageCoordNull, ImageCoordNull);
      break;
    }
    case Dim2D: {
      ADD_IMAGE_SAMPLE_TEXEL(iIdx0, jIdx0, ImageCoordNull);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx0, jIdx1, ImageCoordNull);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx1, jIdx0, ImageCoordNull);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx1, jIdx1, ImageCoordNull);
      break;
    }
    case Dim3D: {
      ADD_IMAGE_SAMPLE_TEXEL(iIdx0, jIdx0, kIdx0);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx0, jIdx0, kIdx1);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx0, jIdx1, kIdx0);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx0, jIdx1, kIdx1);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx1, jIdx0, kIdx0);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx1, jIdx0, kIdx1);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx1, jIdx1, kIdx0);
      ADD_IMAGE_SAMPLE_TEXEL(iIdx1, jIdx1, kIdx1);
      break;
    }
    default:
      LOGE("sample image error: dim not support");
      break;
  }

  return retTexel;
}

glm::vec4 SampleImageFiltered(ImageLevel *imageLevel,
                              glm::vec4 uvwa,
                              SamplerFilterMode filterMode,
                              ImageInfo *imageInfo,
                              SamplerInfo *samplerInfo) {
  glm::vec4 retTexel{0, 0, 0, 0};
  switch (filterMode) {
    case SpvSamplerFilterModeNearest: {
      retTexel = SampleImageNearest(imageLevel, uvwa, imageInfo, samplerInfo);
      break;
    }
    case SpvSamplerFilterModeLinear: {
      retTexel = SampleImageLinear(imageLevel, uvwa, imageInfo, samplerInfo);
      break;
    }
    default:
      break;
  }
  return retTexel;
}

glm::vec4 SampleImageLayerLod(ImageLayer *imageLayer,
                              int32_t level,
                              glm::vec4 uvwa,
                              SamplerFilterMode filterMode,
                              ImageOperands *operands,
                              ImageInfo *imageInfo,
                              SamplerInfo *samplerInfo) {
  ImageLevel *imageLevel = &imageLayer->levels[level - imageInfo->baseMipLevel];

  if (imageLevel->width <= 0 || imageLevel->height <= 0 || imageLevel->data == nullptr) {
    LOGE("image level not uploaded with pixel data");
    return {0, 0, 0, 0};
  }

  // (s,t,r) -> (u,v,w)
  if (!samplerInfo->unnormalizedCoordinates) {
    uvwa.r *= (float) imageLevel->width;
    uvwa.g *= (float) imageLevel->height;
    uvwa.b *= (float) imageLevel->depth;
  }

  // add offset
  if (operands && operands->offset) {
    uvwa += *operands->offset;
  }

  return SampleImageFiltered(imageLevel, uvwa, filterMode, imageInfo, samplerInfo);
}

void SampleMipmapLevel(ImageLevel *dstLevel, ImageLevel *srcLevel,
                       ImageInfo *imageInfo, SamplerFilterMode filterMode) {
  glm::vec4 uvwa;
  float ratioW = (float) srcLevel->width / (float) dstLevel->width;
  float ratioH = (float) srcLevel->height / (float) dstLevel->height;
  float ratioD = (float) srcLevel->depth / (float) dstLevel->depth;

  for (int32_t dd = 0; dd < dstLevel->depth; dd++) {
    for (int32_t dh = 0; dh < dstLevel->height; dh++) {
      for (int32_t dw = 0; dw < dstLevel->width; dw++) {
        uvwa.r = (float) dw * ratioW;
        uvwa.g = (float) dh * ratioH;
        uvwa.b = (float) dd * ratioD;
        glm::vec4 srcTexel = SampleImageFiltered(srcLevel, uvwa, filterMode, imageInfo, nullptr);
        uint8_t *dstTexelPtr = PixelDataPtr(dstLevel, dw, dh, dd, imageInfo->format);
        WritePixelColorRGBA(imageInfo->format, dstTexelPtr, srcTexel);
      }
    }
  }
}

Image *CreateImage(ImageInfo *imageInfo) {
  auto *ret = (Image *) malloc(sizeof(Image));
  ret->info = *imageInfo;
  ret->layerCount = imageInfo->arrayLayers;
  ret->layers = (ImageLayer *) malloc(ret->layerCount * sizeof(ImageLayer));

  // init layers
  for (int32_t i = 0; i < ret->layerCount; i++) {
    ImageLayer *layer = &ret->layers[i];
    layer->layer = imageInfo->baseArrayLayer + i;
    layer->width = imageInfo->width;
    layer->height = imageInfo->height;
    layer->depth = imageInfo->depth;
    layer->levelCount = imageInfo->mipLevels;
    layer->levels = (ImageLevel *) malloc(layer->levelCount * sizeof(ImageLevel));

    // init levels
    for (int32_t j = 0; j < layer->levelCount; j++) {
      ImageLevel *level = &layer->levels[j];
      level->level = imageInfo->baseMipLevel + j;
      level->width = 0;
      level->height = 0;
      level->depth = 0;
      level->dataSize = 0;
      level->data = nullptr;
    }
  }
  return ret;
}

Sampler *CreateSampler(SamplerInfo *samplerInfo) {
  auto *ret = (Sampler *) malloc(sizeof(Sampler));
  if (samplerInfo) {
    ret->info = *samplerInfo;
  } else {
    // default sampler
    ret->info.magFilter = SpvSamplerFilterModeNearest;
    ret->info.minFilter = SpvSamplerFilterModeNearest;
    ret->info.mipmapMode = SpvSamplerFilterModeNearest;
    ret->info.addressModeU = SpvSamplerAddressingModeClampToEdge;
    ret->info.addressModeV = SpvSamplerAddressingModeClampToEdge;
    ret->info.addressModeW = SpvSamplerAddressingModeClampToEdge;
    ret->info.mipLodBias = 0.f;
    ret->info.compareEnable = false;
    ret->info.minLod = 0.f;
    ret->info.maxLod = kMaxSamplerLod;
    ret->info.borderColor = BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    ret->info.unnormalizedCoordinates = false;
  }
  return ret;
}

Sampler *CreateSamplerConstant(SamplerAttribute *attribute) {
  auto *ret = (Sampler *) malloc(sizeof(Sampler));
  ret->info.magFilter = attribute->filterMode;
  ret->info.minFilter = attribute->filterMode;
  ret->info.mipmapMode = attribute->filterMode;
  ret->info.addressModeU = attribute->addressingMode;
  ret->info.addressModeV = attribute->addressingMode;
  ret->info.addressModeW = attribute->addressingMode;
  ret->info.mipLodBias = 0.f;
  ret->info.compareEnable = false;
  ret->info.minLod = 0.f;
  ret->info.maxLod = kMaxSamplerLod;
  ret->info.borderColor = BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
  ret->info.unnormalizedCoordinates = !attribute->normalized;
  return ret;
}

SampledImage *CreateSampledImage(Image *image, Sampler *sampler) {
  auto *ret = (SampledImage *) malloc(sizeof(SampledImage));
  ret->image = image;
  ret->sampler = sampler;
  return ret;
}

void DestroyImage(Image *image) {
  for (int32_t i = 0; i < image->layerCount; i++) {
    ImageLayer *layer = &image->layers[i];
    for (int32_t j = 0; j < layer->levelCount; j++) {
      ImageLevel *level = &layer->levels[j];
      if (level->data) {
        free(level->data);
        level->data = nullptr;
      }
    }
    free(layer->levels);
  }
  free(image->layers);
  free(image);
}

void DestroySampler(Sampler *sampler) {
  free(sampler);
}

void DestroySampledImager(SampledImage *sampledImage) {
  DestroyImage(sampledImage->image);
  DestroySampler(sampledImage->sampler);
  free(sampledImage);
}

void GenerateMipmaps(Image *image, SamplerFilterMode filterMode) {
  for (uint32_t layer = 0; layer < image->layerCount; layer++) {
    ImageLayer *imageLayer = &image->layers[layer];
    ImageLevel *srcLevel = &imageLayer->levels[0];   // based on first level
    if (srcLevel->dataSize <= 0 || srcLevel->data == nullptr) {
      LOGE("GenerateMipmaps error: levels[0] not upload with image data, layer index: %d", layer);
      continue;
    }
    for (uint32_t levelIdx = 1; levelIdx < imageLayer->levelCount; levelIdx++) {
      ImageLevel *dstLevel = &imageLayer->levels[levelIdx];
      dstLevel->width = glm::max(imageLayer->width >> dstLevel->level, 1);
      dstLevel->height = glm::max(imageLayer->height >> dstLevel->level, 1);
      dstLevel->depth = glm::max(imageLayer->depth >> dstLevel->level, 1);
      dstLevel->dataSize = dstLevel->width * dstLevel->height * dstLevel->depth * GetImageFormatBPP(image->info.format);
      dstLevel->data = (uint8_t *) malloc(dstLevel->dataSize);

      SampleMipmapLevel(dstLevel, srcLevel, &image->info, filterMode);
      srcLevel = dstLevel;
    }
  }
}

void GenerateMipmaps(SampledImage *sampledImage) {
  GenerateMipmaps(sampledImage->image, sampledImage->sampler->info.mipmapMode);
}

bool UploadImageData(Image *image, uint8_t *dataPtr, int32_t dataSize,
                     int32_t width, int32_t height, int32_t depth,
                     int32_t mipLevel, int32_t arrayLayer) {
  if (mipLevel < image->info.baseMipLevel
      || mipLevel >= image->info.baseMipLevel + image->info.mipLevels) {
    LOGE("UploadImageData error: mipLevel invalid");
    return false;
  }

  if (arrayLayer < image->info.baseArrayLayer
      || arrayLayer >= image->info.baseArrayLayer + image->layerCount) {
    LOGE("UploadImageData error: arrayLayer invalid");
    return false;
  }

  ImageLevel *level = &image->layers[arrayLayer].levels[mipLevel];
  level->width = width;
  level->height = height;
  level->depth = depth;
  level->dataSize = dataSize;

  free(level->data);
  level->data = (uint8_t *) malloc(dataSize);
  memcpy(level->data, dataPtr, dataSize);

  return true;
}

// calculate ρ_max
float GetDerivativeRhoMax(ImageContext *ctx, int32_t coordinateId, ImageOperands *operands, ImageInfo *imageInfo) {
  glm::vec4 mx{0, 0, 0, 0};
  glm::vec4 my{0, 0, 0, 0};
  if (operands && operands->dx && operands->dy) {
    mx = *operands->dx;
    my = *operands->dy;
  } else {
    mx = ctx->GetDPdx(coordinateId);
    my = ctx->GetDPdy(coordinateId);
  }

  glm::vec4 sizeInfo = {imageInfo->width, imageInfo->height, imageInfo->depth, 0};
  mx *= sizeInfo;
  my *= sizeInfo;

  float rhoX = glm::sqrt(glm::dot(mx, mx));
  float rhoY = glm::sqrt(glm::dot(my, my));
  return glm::max(rhoX, rhoY);
}

float GetLodLambda(ImageContext *ctx,
                   Image *image,
                   Sampler *sampler,
                   int32_t coordinateId,
                   ImageOperands *operands) {
  float lambdaBase;
  if (operands && operands->lod) {
    lambdaBase = *operands->lod;
  } else {
    lambdaBase = log2f(GetDerivativeRhoMax(ctx, coordinateId, operands, &image->info));   // isotropic, η = 1
  }
  float samplerBias = sampler->info.mipLodBias;
  float shaderOpBias = (operands && operands->bias) ? *operands->bias : 0.f;
  float samplerLodMin = sampler->info.minLod;
  float shaderOpLodMin = (operands && operands->minLod) ? *operands->minLod : 0.f;
  float lodMin = glm::max(samplerLodMin, shaderOpLodMin);
  float lodMax = sampler->info.maxLod;

  lambdaBase += glm::clamp(samplerBias + shaderOpBias, -kMaxSamplerLodBias, kMaxSamplerLodBias);
  return glm::clamp(lambdaBase, lodMin, lodMax);
}

float GetComputeAccessedLod(Image *image, Sampler *sampler, float lambda) {
  if (!image->info.mipmaps) {
    return 0.f;
  }

  float d = (float) image->info.baseMipLevel + glm::clamp(lambda, 0.f, (float) image->info.mipLevels - 1);
  if (sampler->info.mipmapMode == SpvSamplerFilterModeNearest) {
    return glm::ceil(d + 0.5f) - 1.0f;
  }
  return d;
}

glm::ivec4 GetImageSizeLod(Image *image, int32_t lod) {
  glm::ivec4 ret{0, 0, 0, 0};
  uint32_t idx = 0;
  ImageInfo *info = &image->info;
  if (lod < info->baseMipLevel || lod >= (info->baseMipLevel + info->mipLevels)) {
    LOGE("GetImageSizeLod invalid lod");
    return ret;
  }
  ImageLevel *imageLevel = &image->layers[0].levels[lod - info->baseMipLevel];
  switch (info->dim) {
    case Dim1D:
      ret.data[idx++] = imageLevel->width;
      break;
    case Dim2D:
    case DimCube:
      ret.data[idx++] = imageLevel->width;
      ret.data[idx++] = imageLevel->height;
      break;
    case Dim3D:
      ret.data[idx++] = imageLevel->width;
      ret.data[idx++] = imageLevel->height;
      ret.data[idx++] = imageLevel->depth;
      break;
    default:
      LOGE("GetImageSizeLod dim not support");
      break;
  }
  if (info->arrayed) {
    ret.data[idx++] = info->arrayLayers;
  }

  return ret;
}

glm::ivec4 GetImageSize(Image *image) {
  glm::ivec4 ret{0, 0, 0, 0};
  uint32_t idx = 0;
  switch (image->info.dim) {
    case Dim2D:
      ret.data[idx++] = image->info.width;
      ret.data[idx++] = image->info.height;
      break;
    case Dim3D:
      ret.data[idx++] = image->info.width;
      ret.data[idx++] = image->info.height;
      ret.data[idx++] = image->info.depth;
      break;
    default:
      LOGE("GetImageSize dim not support");
      break;
  }
  if (image->info.arrayed) {
    ret.data[idx++] = image->info.arrayLayers;
  }

  return ret;
}

glm::vec4 SampleImage(ImageContext *ctx,
                      Image *image,
                      Sampler *sampler,
                      glm::vec4 uvwa,
                      int32_t coordinateId,
                      ImageOperands *operands) {
  glm::vec4 retTexel;
  if (!image || !sampler) {
    LOGE("SampleImage error: image or sampler is null");
    return retTexel;
  }
  // layer
  int32_t layerIdx = image->info.baseArrayLayer;
  if (sampler->info.unnormalizedCoordinates) {
    layerIdx = glm::clamp((int32_t) glm::roundEven(uvwa.w), (int32_t) image->info.baseArrayLayer,
                          (int32_t) (image->info.baseArrayLayer + image->info.arrayLayers - 1));
  }
  ImageLayer *imageLayer = &image->layers[layerIdx - image->info.baseArrayLayer];

  // lod & level
  float lodLambda = GetLodLambda(ctx, image, sampler, coordinateId, operands);
  float lodLevel = GetComputeAccessedLod(image, sampler, lodLambda);
  SamplerFilterMode filterMode = (lodLambda <= 0) ? sampler->info.magFilter : sampler->info.minFilter;

  if (sampler->info.mipmapMode == SpvSamplerFilterModeNearest) {
    retTexel = SampleImageLayerLod(imageLayer, (int32_t) lodLevel, uvwa, filterMode, operands,
                                   &image->info, &sampler->info);
  } else {
    float levelHi = glm::floor(lodLevel);
    float levelLo = glm::min(levelHi + 1, (float) (image->info.baseMipLevel + image->info.mipLevels - 1));
    float delta = lodLevel - levelHi;
    glm::vec4 texelHi = SampleImageLayerLod(imageLayer, (int32_t) levelHi, uvwa, filterMode, operands,
                                            &image->info, &sampler->info);
    glm::vec4 texelLo = SampleImageLayerLod(imageLayer, (int32_t) levelLo, uvwa, filterMode, operands,
                                            &image->info, &sampler->info);
    retTexel = texelHi * (1.f - delta);
    retTexel = retTexel + (texelLo * delta);
  }

  return retTexel;
}

glm::vec4 FetchImage(Image *image,
                     glm::ivec4 uvwa,
                     ImageOperands *operands) {
  glm::vec4 retTexel;
  if (!image) {
    LOGE("SampleImage error: image or sampler is null");
    return retTexel;
  }

  // layer
  int32_t layerIdx = glm::clamp((int32_t) uvwa.w, (int32_t) image->info.baseArrayLayer,
                                (int32_t) (image->info.baseArrayLayer + image->info.arrayLayers - 1));
  ImageLayer *imageLayer = &image->layers[layerIdx];

  // level
  int32_t level = image->info.baseMipLevel;
  if (operands && operands->lod) {
    level = glm::clamp((int32_t) *operands->lod, image->info.baseMipLevel,
                       image->info.baseMipLevel + image->info.mipLevels - 1);
  }
  ImageLevel *imageLevel = &imageLayer->levels[level - image->info.baseMipLevel];
  if (imageLevel->width <= 0 || imageLevel->height <= 0 || imageLevel->data == nullptr) {
    LOGE("image level not uploaded with pixel data");
    return retTexel;
  }

  // add offset
  if (operands && operands->offset) {
    uvwa += *operands->offset;
  }

  uint8_t *texelPtr = PixelDataPtr(imageLevel, uvwa.r, uvwa.g, uvwa.b, image->info.format);
  retTexel = ReadPixelColorRGBA(image->info.format, texelPtr);
  return retTexel;
}

glm::ivec4 QueryImageSizeLod(Image *image, int32_t lod) {
  if (!image) {
    LOGE("QueryImageSizeLod error: image is null");
    return {0, 0, 0, 0};
  }
  return GetImageSizeLod(image, lod);
}

glm::ivec4 QueryImageSize(Image *image) {
  if (!image) {
    LOGE("QueryImageSize error: image is null");
    return {0, 0, 0, 0};
  }
  return GetImageSize(image);
}

glm::vec2 QueryImageLod(ImageContext *ctx,
                        Image *image,
                        Sampler *sampler,
                        int32_t coordinateId) {
  if (!image || !sampler) {
    LOGE("QueryImageLod error: image or sampler is null");
    return {0, 0};
  }
  float lodLambda = GetLodLambda(ctx, image, sampler, coordinateId, nullptr);
  float lodLevel = GetComputeAccessedLod(image, sampler, lodLambda);
  return {lodLambda, lodLevel};
}

}
}
