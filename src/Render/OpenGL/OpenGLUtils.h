/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include <glad/glad.h>
#include "Base/Logger.h"

namespace SoftGL {

class OpenGLUtils {
 public:
  static void checkGLError_(const char *stmt, const char *file, int line) {
    const char *str;
    GLenum err = glGetError();
    switch (err) {
      case GL_NO_ERROR:
        str = "GL_NO_ERROR";
        break;
      case GL_INVALID_ENUM:
        str = "GL_INVALID_ENUM";
        break;
      case GL_INVALID_VALUE:
        str = "GL_INVALID_VALUE";
        break;
      case GL_INVALID_OPERATION:
        str = "GL_INVALID_OPERATION";
        break;
      case GL_OUT_OF_MEMORY:
        str = "GL_OUT_OF_MEMORY";
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        str = "GL_INVALID_FRAMEBUFFER_OPERATION";
        break;
      default:
        str = "(ERROR: Unknown Error Enum)";
        break;
    }

    if (err != GL_NO_ERROR) {
      LOGE("CheckGLError: %s, %s:%d, %s", str, file, line, stmt);
      abort();
    }
  }
};

#ifdef DEBUG
#define GL_CHECK(stmt) do { \
            stmt; \
            OpenGLUtils::checkGLError_(#stmt, __FILE__, __LINE__); \
        } while (0)
#else
#define GL_CHECK(stmt) stmt
#endif

}