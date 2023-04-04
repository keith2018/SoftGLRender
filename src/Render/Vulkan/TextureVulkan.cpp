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

  // image format
  vkFormat_ = VK::cvtImageFormat(format, usage);

  // only multi sample color attachment need to be resolved
  needResolve_ = multiSample && (usage & TextureUsage_AttachmentColor);
  needMipmaps_ = useMipmaps && vkCtx_.linearBlitAvailable(vkFormat_);
  layerCount_ = getLayerCount();
  if (needMipmaps_) {
    levelCount_ = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
  } else {
    levelCount_ = 1;
  }
  imageAspect_ = getImageAspect();

  createImage();
  bool memoryReady = false;
  if (needResolve_) {
    // check if lazy allocate available
    memoryReady = vkCtx_.createImageMemory(memory_, image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
  }
  if (!memoryReady) {
    vkCtx_.createImageMemory(memory_, image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  // resolve
  if (needResolve_) {
    createImageResolve();
    vkCtx_.createImageMemory(memoryResolve_, imageResolve_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    createImageView(viewResolve_, imageResolve_);
  }
}

TextureVulkan::~TextureVulkan() {
  vkDestroySampler(device_, sampler_, nullptr);
  vkDestroyImageView(device_, sampleView_, nullptr);

  vkDestroyImage(device_, image_, nullptr);
  vkFreeMemory(device_, memory_, nullptr);

  vkDestroyImageView(device_, viewResolve_, nullptr);
  vkDestroyImage(device_, imageResolve_, nullptr);
  vkFreeMemory(device_, memoryResolve_, nullptr);

  vkDestroyImage(device_, hostImage_, nullptr);
  vkFreeMemory(device_, hostMemory_, nullptr);
}

void TextureVulkan::dumpImage(const char *path, uint32_t layer, uint32_t level) {
  if (multiSample) {
    return;
  }

  readPixels(layer, level, [&](uint8_t *buffer, uint32_t w, uint32_t h, uint32_t rowStride) -> void {
    auto *pixels = new uint8_t[w * h * 4];
    for (uint32_t i = 0; i < h; i++) {
      memcpy(pixels + i * rowStride, buffer + i * rowStride, w * getPixelByteSize());
    }

    // convert float to rgba
    if (format == TextureFormat_FLOAT32) {
      ImageUtils::convertFloatImage(reinterpret_cast<RGBA *>(pixels), reinterpret_cast<float *>(pixels), w, h);
    }
    ImageUtils::writeImage(path, (int) w, (int) h, 4, pixels, (int) w * 4, true);
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
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK::cvtMipmapMode(samplerDesc_.filterMin);
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = static_cast<float>(levelCount_);
  samplerInfo.mipLodBias = 0.0f;
  VK_CHECK(vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_));

  return sampler_;
}

void TextureVulkan::readPixels(uint32_t layer, uint32_t level,
                               const std::function<void(uint8_t *buffer, uint32_t width, uint32_t height, uint32_t rowStride)> &func) {
  bool needResetMemory = createImageHost(level);
  if (needResetMemory) {
    if (hostMemory_ != VK_NULL_HANDLE) {
      vkFreeMemory(device_, hostMemory_, nullptr);
      hostMemory_ = VK_NULL_HANDLE;
    }
    vkCtx_.createImageMemory(hostMemory_, hostImage_, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }

  // copy
  VkCommandBuffer copyCmd = vkCtx_.beginSingleTimeCommands();
  VKContext::transitionImageLayout(copyCmd, {hostImage_, imageAspect_, 1, 1},
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0, VK_ACCESS_TRANSFER_WRITE_BIT);

  VkImageCopy imageCopyRegion{};
  imageCopyRegion.srcSubresource.aspectMask = imageAspect_;
  imageCopyRegion.srcSubresource.baseArrayLayer = layer;
  imageCopyRegion.srcSubresource.layerCount = 1;
  imageCopyRegion.srcSubresource.mipLevel = level;
  imageCopyRegion.dstSubresource.aspectMask = imageAspect_;
  imageCopyRegion.dstSubresource.baseArrayLayer = 0;
  imageCopyRegion.dstSubresource.layerCount = 1;
  imageCopyRegion.dstSubresource.mipLevel = 0;
  imageCopyRegion.extent.width = getLevelWidth(level);
  imageCopyRegion.extent.height = getLevelHeight(level);
  imageCopyRegion.extent.depth = 1;
  vkCmdCopyImage(copyCmd,
                 needResolve_ ? imageResolve_ : image_,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 hostImage_,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1,
                 &imageCopyRegion);

  VKContext::transitionImageLayout(copyCmd, {hostImage_, imageAspect_, 1, 1},
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);

  vkCtx_.endSingleTimeCommands(copyCmd);

  // map to host memory
  VkImageSubresource subResource{};
  subResource.aspectMask = imageAspect_;
  VkSubresourceLayout subResourceLayout;

  vkGetImageSubresourceLayout(device_, hostImage_, &subResource, &subResourceLayout);

  uint8_t *pixelPtr = nullptr;
  vkMapMemory(device_, hostMemory_, 0, VK_WHOLE_SIZE, 0, (void **) &pixelPtr);
  pixelPtr += subResourceLayout.offset;

  if (func) {
    func(pixelPtr, getLevelWidth(level), getLevelHeight(level), subResourceLayout.rowPitch);
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
  imageInfo.format = vkFormat_;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = levelCount_;
  imageInfo.arrayLayers = layerCount_;
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
  if (needMipmaps_) {
    imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
  imageInfo.format = vkFormat_;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
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

bool TextureVulkan::createImageHost(uint32_t level) {
  if (hostImage_ != VK_NULL_HANDLE && level == hostImageLevel_) {
    return false;
  }
  hostImageLevel_ = level;
  if (hostImage_ != VK_NULL_HANDLE) {
    vkDestroyImage(device_, hostImage_, nullptr);
  }

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = vkFormat_;
  imageInfo.extent.width = getLevelWidth(level);
  imageInfo.extent.height = getLevelHeight(level);
  imageInfo.extent.depth = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.mipLevels = 1;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  VK_CHECK(vkCreateImage(device_, &imageInfo, nullptr, &hostImage_));

  return true;
}

void TextureVulkan::createImageView(VkImageView &view, VkImage &image) {
  VkImageViewCreateInfo imageViewCreateInfo{};
  imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCreateInfo.viewType = VK::cvtImageViewType(type);
  imageViewCreateInfo.format = vkFormat_;
  imageViewCreateInfo.subresourceRange = {};
  imageViewCreateInfo.subresourceRange.aspectMask = imageAspect_;
  imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  imageViewCreateInfo.subresourceRange.levelCount = levelCount_;
  imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  imageViewCreateInfo.subresourceRange.layerCount = layerCount_;
  imageViewCreateInfo.image = image;
  VK_CHECK(vkCreateImageView(device_, &imageViewCreateInfo, nullptr, &view));
}

VkImageView TextureVulkan::createAttachmentView(uint32_t layer, uint32_t level) {
  VkImageView view{};
  VkImageViewCreateInfo imageViewCreateInfo{};
  imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // force view type 2D
  imageViewCreateInfo.format = vkFormat_;
  imageViewCreateInfo.subresourceRange = {};
  imageViewCreateInfo.subresourceRange.aspectMask = imageAspect_;
  imageViewCreateInfo.subresourceRange.baseMipLevel = level;
  imageViewCreateInfo.subresourceRange.levelCount = 1;
  imageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
  imageViewCreateInfo.subresourceRange.layerCount = 1;
  imageViewCreateInfo.image = image_;
  VK_CHECK(vkCreateImageView(device_, &imageViewCreateInfo, nullptr, &view));
  return view;
}

void TextureVulkan::generateMipmaps() {
  VkCommandBuffer commandBuffer = vkCtx_.beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image_;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = imageAspect_;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  for (uint32_t layer = 0; layer < layerCount_; layer++) {
    barrier.subresourceRange.baseArrayLayer = layer;
    for (uint32_t level = 1; level < levelCount_; level++) {
      barrier.subresourceRange.baseMipLevel = level - 1;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(commandBuffer,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                           0, nullptr,
                           0, nullptr,
                           1, &barrier);

      VkImageBlit blit{};
      blit.srcOffsets[0] = {0, 0, 0};
      blit.srcOffsets[1] = {(int32_t) getLevelWidth(level - 1), (int32_t) getLevelHeight(level - 1), 1};
      blit.srcSubresource.aspectMask = imageAspect_;
      blit.srcSubresource.mipLevel = level - 1;
      blit.srcSubresource.baseArrayLayer = layer;
      blit.srcSubresource.layerCount = 1;
      blit.dstOffsets[0] = {0, 0, 0};
      blit.dstOffsets[1] = {(int32_t) getLevelWidth(level), (int32_t) getLevelHeight(level), 1};
      blit.dstSubresource.aspectMask = imageAspect_;
      blit.dstSubresource.mipLevel = level;
      blit.dstSubresource.baseArrayLayer = layer;
      blit.dstSubresource.layerCount = 1;

      vkCmdBlitImage(commandBuffer,
                     image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     1, &blit,
                     VK_FILTER_LINEAR);

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(commandBuffer,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                           0, nullptr,
                           0, nullptr,
                           1, &barrier);
    }

    barrier.subresourceRange.baseMipLevel = levelCount_ - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
  }

  vkCtx_.endSingleTimeCommands(commandBuffer);
}

void TextureVulkan::setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) {
  if (format != TextureFormat_RGBA8) {
    LOGE("setImageData error: format not match");
    return;
  }

  if (buffers.size() != layerCount_) {
    LOGE("setImageData error: layer count not match");
    return;
  }

  auto &dataBuffer = buffers[0];
  if (dataBuffer->getRawDataSize() != width * height) {
    LOGE("setImageData error: size not match");
    return;
  }

  VkDeviceSize imageSize = dataBuffer->getRawDataSize() * sizeof(RGBA);
  std::vector<const void *> buffersPtr;
  buffersPtr.reserve(buffers.size());
  for (auto &buff : buffers) {
    buffersPtr.push_back(buff->getRawDataPtr());
  }

  setImageDataInternal(buffersPtr, imageSize);

  if (needMipmaps_) {
    generateMipmaps();
  }
}

void TextureVulkan::setImageData(const std::vector<std::shared_ptr<Buffer<float>>> &buffers) {
  if (format != TextureFormat_FLOAT32) {
    LOGE("setImageData error: format not match");
    return;
  }

  if (buffers.size() != layerCount_) {
    LOGE("setImageData error: layer count not match");
    return;
  }

  auto &dataBuffer = buffers[0];
  if (dataBuffer->getRawDataSize() != width * height) {
    LOGE("setImageData error: size not match");
    return;
  }

  VkDeviceSize imageSize = dataBuffer->getRawDataSize() * sizeof(float);
  std::vector<const void *> buffersPtr;
  buffersPtr.reserve(buffers.size());
  for (auto &buff : buffers) {
    buffersPtr.push_back(buff->getRawDataPtr());
  }

  setImageDataInternal(buffersPtr, imageSize);

  if (needMipmaps_) {
    generateMipmaps();
  }
}

void TextureVulkan::setImageDataInternal(const std::vector<const void *> &buffers, VkDeviceSize imageSize) {
  VkDeviceSize bufferSize = imageSize * layerCount_;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  vkCtx_.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      stagingBuffer, stagingBufferMemory);

  uint8_t *dataPtr;
  VK_CHECK(vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, reinterpret_cast<void **>(&dataPtr)));
  for (auto &ptr : buffers) {
    memcpy(dataPtr, ptr, static_cast<size_t>(imageSize));
    dataPtr += imageSize;
  }
  vkUnmapMemory(device_, stagingBufferMemory);

  VkCommandBuffer copyCmd = vkCtx_.beginSingleTimeCommands();
  VKContext::transitionImageLayout(copyCmd, {image_, imageAspect_, layerCount_, levelCount_},
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0, VK_ACCESS_TRANSFER_WRITE_BIT);

  std::vector<VkBufferImageCopy> copyRegions;
  for (uint32_t layer = 0; layer < layerCount_; layer++) {
    VkBufferImageCopy region{};
    region.bufferOffset = layer * imageSize;
    region.imageSubresource.aspectMask = imageAspect_;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {(uint32_t) width, (uint32_t) height, 1};
    copyRegions.push_back(region);
  }

  vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyRegions.size(), copyRegions.data());

  // transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
  if (!needMipmaps_) {
    VKContext::transitionImageLayout(copyCmd, {image_, imageAspect_, layerCount_, levelCount_},
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
  }
  vkCtx_.endSingleTimeCommands(copyCmd);

  vkDestroyBuffer(device_, stagingBuffer, nullptr);
  vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

}
