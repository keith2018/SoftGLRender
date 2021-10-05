/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Common.h"

namespace SoftGL {

enum BufferLayout {
  Layout_Linear,
  Layout_Tiled,
  Layout_Morton,
};

template<typename T>
class Buffer {
 public:

  virtual void InitLayout() {
    inner_width_ = width_;
    inner_height_ = height_;
  }

  virtual inline size_t ConvertIndex(size_t x, size_t y) const {
    return x + y * inner_width_;
  }

  virtual BufferLayout GetLayout() const {
    return Layout_Linear;
  }

  void Create(size_t w, size_t h) {
    if (w > 0 && h > 0) {
      if (width_ == w && height_ == h) {
        return;
      }
      width_ = w;
      height_ = h;

      InitLayout();
      data_size_ = inner_width_ * inner_height_;
      data_ = std::shared_ptr<T>(new T[data_size_], [](const T *ptr) { delete[] ptr; });
    }
  }

  virtual void Destroy() {
    width_ = 0;
    height_ = 0;
    inner_width_ = 0;
    inner_height_ = 0;
    data_size_ = 0;
    data_ = nullptr;
  }

  inline T *GetRawDataPtr() const {
    return data_.get();
  }

  inline bool Empty() const {
    return data_ == nullptr;
  }

  inline size_t GetWidth() const {
    return width_;
  }

  inline size_t GetHeight() const {
    return height_;
  }

  inline T *Get(size_t x, size_t y) {
    T *ptr = data_.get();
    if (ptr != nullptr && x < width_ && y < height_) {
      return &ptr[ConvertIndex(x, y)];
    }
    return nullptr;
  }

  inline void Set(size_t x, size_t y, const T &pixel) {
    T *ptr = data_.get();
    if (ptr != nullptr && x < width_ && y < height_) {
      ptr[ConvertIndex(x, y)] = pixel;
    }
  }

  void CopyRawDataTo(T *out, bool flip_y = false) const {
    T *ptr = data_.get();
    if (ptr != nullptr) {
      if (!flip_y) {
        memcpy(out, ptr, data_size_ * sizeof(T));
      } else {
        for (int i = 0; i < inner_height_; i++) {
          memcpy(out + inner_width_ * i, ptr + inner_width_ * (inner_height_ - 1 - i), inner_width_ * sizeof(T));
        }
      }
    }
  }

  inline void Clear() const {
    T *ptr = data_.get();
    if (ptr != nullptr) {
      memset(ptr, 0, data_size_ * sizeof(T));
    }
  }

  inline void SetAll(T val) const {
    T *ptr = data_.get();
    if (ptr != nullptr) {
      for (int i = 0; i < data_size_; i++) {
        ptr[i] = val;
      }
    }
  }

 protected:
  size_t width_ = 0;
  size_t height_ = 0;
  size_t inner_width_ = 0;
  size_t inner_height_ = 0;
  std::shared_ptr<T> data_ = nullptr;
  size_t data_size_ = 0;
};

template<typename T>
class TiledBuffer : public Buffer<T> {
 public:

  void InitLayout() override {
    tile_width_ = (this->width_ + tile_size_ - 1) / tile_size_;
    tile_height_ = (this->height_ + tile_size_ - 1) / tile_size_;
    this->inner_width_ = tile_width_ * tile_size_;
    this->inner_height_ = tile_height_ * tile_size_;
  }

  inline size_t ConvertIndex(size_t x, size_t y) const override {
    uint16_t tileX = x >> bits_;              // x / tile_size_
    uint16_t tileY = y >> bits_;              // y / tile_size_
    uint16_t inTileX = x & (tile_size_ - 1);  // x % tile_size_
    uint16_t inTileY = y & (tile_size_ - 1);  // y % tile_size_

    return ((tileY * tile_width_ + tileX) << bits_ << bits_)
        + (inTileY << bits_)
        + inTileX;
  }

  BufferLayout GetLayout() const override {
    return Layout_Tiled;
  }

 private:
  const static int tile_size_ = 4;   // 4 x 4
  const static int bits_ = 2;        // tile_size_ = 2^bits_
  size_t tile_width_ = 0;
  size_t tile_height_ = 0;
};

template<typename T>
class MortonBuffer : public Buffer<T> {
 public:

  void InitLayout() override {
    tile_width_ = (this->width_ + tile_size_ - 1) / tile_size_;
    tile_height_ = (this->height_ + tile_size_ - 1) / tile_size_;
    this->inner_width_ = tile_width_ * tile_size_;
    this->inner_height_ = tile_height_ * tile_size_;
  }

  /**
   * Ref: https://gist.github.com/JarkkoPFC/0e4e599320b0cc7ea92df45fb416d79a
   */
  static inline uint16_t encode16_morton2(uint8_t x_, uint8_t y_) {
    uint32_t res = x_ | (uint32_t(y_) << 16);
    res = (res | (res << 4)) & 0x0f0f0f0f;
    res = (res | (res << 2)) & 0x33333333;
    res = (res | (res << 1)) & 0x55555555;
    return uint16_t(res | (res >> 15));
  }

  inline size_t ConvertIndex(size_t x, size_t y) const override {
    uint16_t tileX = x >> bits_;              // x / tile_size_
    uint16_t tileY = y >> bits_;              // y / tile_size_
    uint16_t inTileX = x & (tile_size_ - 1);  // x % tile_size_
    uint16_t inTileY = y & (tile_size_ - 1);  // y % tile_size_

    uint16_t mortonIndex = encode16_morton2(inTileX, inTileY);

    return ((tileY * tile_width_ + tileX) << bits_ << bits_)
        + mortonIndex;
  }

  BufferLayout GetLayout() const override {
    return Layout_Morton;
  }

 private:
  const static int tile_size_ = 32; // 32 x 32
  const static int bits_ = 5;       // tile_size_ = 2^bits_
  size_t tile_width_ = 0;
  size_t tile_height_ = 0;
};


struct FrameBuffer {
  size_t width = 0;
  size_t height = 0;
  std::shared_ptr<Buffer<glm::vec4>> color;
  std::shared_ptr<Buffer<float>> depth;

  void Create(size_t w, size_t h) {
    if (!color) {
      color = std::make_shared<Buffer<glm::vec4>>();
    }
    if (!depth) {
      depth = std::make_shared<Buffer<float>>();
    }

    color->Create(w, h);
    depth->Create(w, h);
  }
};

}
