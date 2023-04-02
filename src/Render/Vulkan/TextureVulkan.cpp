/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "TextureVulkan.h"

namespace SoftGL {

TextureVulkan::TextureVulkan(VKContext &ctx, const TextureDesc &desc) : vkCtx_(ctx) {
  device_ = ctx.device();

  width = desc.width;
  height = desc.height;
  type = desc.type;
  format = desc.format;
  usage = desc.usage;
  useMipmaps = desc.useMipmaps;
  multiSample = desc.multiSample;

  // only multi sample color attachment need to be resolved
  needResolve_ = multiSample && (usage & TextureUsage_AttachmentColor);

  createImage();
  bool memoryReady = false;
  if (needResolve_) {
    // check if lazy allocate available
    memoryReady = vkCtx_.createImageMemory(memory_, image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
  }
  if (!memoryReady) {
    vkCtx_.createImageMemory(memory_, image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }
  createImageView(view_, image_);

  // resolve
  if (needResolve_) {
    createImageResolve();
    vkCtx_.createImageMemory(memoryResolve_, imageResolve_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    createImageView(viewResolve_, imageResolve_);
  }
}

TextureVulkan::~TextureVulkan() {
  vkDestroySampler(device_, sampler_, nullptr);

  vkDestroyImageView(device_, view_, nullptr);
  vkDestroyImage(device_, image_, nullptr);
  vkFreeMemory(device_, memory_, nullptr);

  vkDestroyImageView(device_, viewResolve_, nullptr);
  vkDestroyImage(device_, imageResolve_, nullptr);
  vkFreeMemory(device_, memoryResolve_, nullptr);

  vkDestroyImage(device_, hostImage_, nullptr);
  vkFreeMemory(device_, hostMemory_, nullptr);
}

void TextureVulkan::dumpImage(const char *path) {
  if (multiSample) {
    return;
  }

  readPixels([&](uint8_t *buffer, uint32_t width, uint32_t height, uint32_t rowStride) -> void {
    auto *pixels = new uint8_t[width * height * 4];
    for (uint32_t i = 0; i < height; i++) {
      memcpy(pixels + i * rowStride, buffer + i * rowStride, width * getPixelByteSize());
    }

    // convert float to rgba
    if (format == TextureFormat_FLOAT32) {
      ImageUtils::convertFloatImage(reinterpret_cast<RGBA *>(pixels), reinterpret_cast<float *>(pixels),
                                    width, height);
    }
    ImageUtils::writeImage(path, (int) width, (int) height, 4, pixels, (int) width * 4, true);
    delete[] pixels;
  });
}

VkSampler &TextureVulkan::getSampler() {
  if (sampler_ != VK_NULL_HANDLE) {
    return sampler_;
  }

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK::cvtFilter(samplerDesc_.filterMag);
  samplerInfo.minFilter = VK::cvtFilter(samplerDesc_.filterMin);
  samplerInfo.addressModeU = VK::cvtWrapMode(samplerDesc_.wrapS);
  samplerInfo.addressModeV = VK::cvtWrapMode(samplerDesc_.wrapT);
  samplerInfo.addressModeW = VK::cvtWrapMode(samplerDesc_.wrapR);
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = vkCtx_.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK::cvtBorderColor(samplerDesc_.borderColor);
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
  samplerInfo.mipmapMode = VK::cvtMipmapMode(samplerDesc_.filterMin);
  VK_CHECK(vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_));

  return sampler_;
}

void TextureVulkan::readPixels(const std::function<void(uint8_t *buffer, uint32_t width, uint32_t height, uint32_t rowStride)> &func) {
  createImageHost();
  vkCtx_.createImageMemory(hostMemory_, hostImage_, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // copy
  VkCommandBuffer copyCmd = vkCtx_.beginSingleTimeCommands();
  VKContext::transitionImageLayout(copyCmd, hostImage_, getImageAspect(),
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0, VK_ACCESS_TRANSFER_WRITE_BIT);

  VkImageCopy imageCopyRegion{};
  imageCopyRegion.srcSubresource.aspectMask = getImageAspect();
  imageCopyRegion.srcSubresource.layerCount = 1;
  imageCopyRegion.dstSubresource.aspectMask = getImageAspect();
  imageCopyRegion.dstSubresource.layerCount = 1;
  imageCopyRegion.extent.width = width;
  imageCopyRegion.extent.height = height;
  imageCopyRegion.extent.depth = 1;
  vkCmdCopyImage(copyCmd,
                 needResolve_ ? imageResolve_ : image_,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 hostImage_,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1,
                 &imageCopyRegion);

  VKContext::transitionImageLayout(copyCmd, hostImage_, getImageAspect(),
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);

  vkCtx_.endSingleTimeCommands(copyCmd);

  // map to host memory
  VkImageSubresource subResource{};
  subResource.aspectMask = getImageAspect();
  VkSubresourceLayout subResourceLayout;

  vkGetImageSubresourceLayout(device_, hostImage_, &subResource, &subResourceLayout);

  uint8_t *pixelPtr = nullptr;
  vkMapMemory(device_, hostMemory_, 0, VK_WHOLE_SIZE, 0, (void **) &pixelPtr);
  pixelPtr += subResourceLayout.offset;

  if (func) {
    func(pixelPtr, width, height, subResourceLayout.rowPitch);
  }

  vkUnmapMemory(device_, hostMemory_);
}

void TextureVulkan::createImage() {
  if (image_ != VK_NULL_HANDLE) {
    return;
  }

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK::cvtImageType(type);
  imageInfo.format = VK::cvtImageFormat(format, usage);
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = getLayerCount();
  imageInfo.samples = getSampleCount();
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = needResolve_ ? VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT : 0;

  if (usage & TextureUsage_Sampler) {
    imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  if (usage & TextureUsage_UploadData) {
    imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  if (usage & TextureUsage_AttachmentColor) {
    imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (!needResolve_) {
      imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
  }
  if (usage & TextureUsage_AttachmentDepth) {
    imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }

  VK_CHECK(vkCreateImage(device_, &imageInfo, nullptr, &image_));
}

void TextureVulkan::createImageResolve() {
  if (imageResolve_ != VK_NULL_HANDLE) {
    return;
  }

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK::cvtImageType(type);
  imageInfo.format = VK::cvtImageFormat(format, usage);
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = getLayerCount();
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  if (usage & TextureUsage_Sampler) {
    imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  if (usage & TextureUsage_AttachmentColor) {
    imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }

  VK_CHECK(vkCreateImage(device_, &imageInfo, nullptr, &imageResolve_));
}

void TextureVulkan::createImageHost() {
  if (hostImage_ != VK_NULL_HANDLE) {
    return;
  }

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = VK::cvtImageFormat(format, usage);
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.mipLevels = 1;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  VK_CHECK(vkCreateImage(device_, &imageInfo, nullptr, &hostImage_));
}

void TextureVulkan::createImageView(VkImageView &view, VkImage &image) {
  VkImageViewCreateInfo imageViewCreateInfo{};
  imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCreateInfo.viewType = VK::cvtImageViewType(type);
  imageViewCreateInfo.format = VK::cvtImageFormat(format, usage);
  imageViewCreateInfo.subresourceRange = {};
  imageViewCreateInfo.subresourceRange.aspectMask = getImageAspect();
  imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  imageViewCreateInfo.subresourceRange.levelCount = 1;
  imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  imageViewCreateInfo.subresourceRange.layerCount = getLayerCount();
  imageViewCreateInfo.image = image;
  VK_CHECK(vkCreateImageView(device_, &imageViewCreateInfo, nullptr, &view));
}

void Texture2DVulkan::setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) {
  if (format != TextureFormat_RGBA8) {
    LOGE("setImageData error: format not match");
    return;
  }

  auto &dataBuffer = buffers[0];
  if (dataBuffer->getRawDataSize() != width * height) {
    LOGE("setImageData error: size not match");
    return;
  }

  VkDeviceSize imageSize = dataBuffer->getRawDataSize() * sizeof(RGBA);
  setImageDataInternal(dataBuffer->getRawDataPtr(), imageSize);
}

void Texture2DVulkan::setImageData(const std::vector<std::shared_ptr<Buffer<float>>> &buffers) {
  if (format != TextureFormat_FLOAT32) {
    LOGE("setImageData error: format not match");
    return;
  }

  auto &dataBuffer = buffers[0];
  if (dataBuffer->getRawDataSize() != width * height) {
    LOGE("setImageData error: size not match");
    return;
  }

  VkDeviceSize imageSize = dataBuffer->getRawDataSize() * sizeof(float);
  setImageDataInternal(dataBuffer->getRawDataPtr(), imageSize);
}

void Texture2DVulkan::setImageDataInternal(const void *buffer, VkDeviceSize imageSize) {
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  vkCtx_.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      stagingBuffer, stagingBufferMemory);

  void *dataPtr;
  VK_CHECK(vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &dataPtr));
  memcpy(dataPtr, buffer, static_cast<size_t>(imageSize));
  vkUnmapMemory(device_, stagingBufferMemory);

  VkCommandBuffer copyCmd = vkCtx_.beginSingleTimeCommands();
  VKContext::transitionImageLayout(copyCmd, image_, getImageAspect(),
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0, VK_ACCESS_TRANSFER_WRITE_BIT);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = getImageAspect();
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {(uint32_t) width, (uint32_t) height, 1};

  vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  VKContext::transitionImageLayout(copyCmd, image_, getImageAspect(),
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                   VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
  vkCtx_.endSingleTimeCommands(copyCmd);

  vkDestroyBuffer(device_, stagingBuffer, nullptr);
  vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

}
