/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include "md5.h"

namespace SoftGL {

class HashUtils {
 public:
  template<class T>
  inline static void hashCombine(size_t &seed, T const &v) {
    seed ^= std::hash<T>()(v) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
  }

  inline static uint32_t murmur3(const uint32_t *key, size_t wordCount, uint32_t seed) noexcept {
    uint32_t h = seed;
    size_t i = wordCount;
    do {
      uint32_t k = *key++;
      k *= 0xcc9e2d51u;
      k = (k << 15u) | (k >> 17u);
      k *= 0x1b873593u;
      h ^= k;
      h = (h << 13u) | (h >> 19u);
      h = (h * 5u) + 0xe6546b64u;
    } while (--i);
    h ^= wordCount;
    h ^= h >> 16u;
    h *= 0x85ebca6bu;
    h ^= h >> 13u;
    h *= 0xc2b2ae35u;
    h ^= h >> 16u;
    return h;
  }

  template<typename T>
  inline static void hashCombineMurmur(size_t &seed, const T &key) {
    static_assert(0 == (sizeof(key) & 3u), "Hashing requires a size that is a multiple of 4.");
    uint32_t keyHash = HashUtils::murmur3((const uint32_t *) &key, sizeof(key) / 4, 0);
    seed ^= keyHash + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
  }

  inline static std::string getHashMD5(const void *data, size_t length) {
    MD5 md5;
    return md5(data, length);
  }

  inline static std::string getHashMD5(const std::string &text) {
    MD5 md5;
    return md5(text);
  }
};

}
