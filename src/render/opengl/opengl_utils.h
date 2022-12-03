/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include <glad/glad.h>
#include "base/logger.h"

namespace SoftGL {

class OpenGLUtils {
 public:
  static void CheckGLError_(const char *file, int line) {
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
      default:
        str = "(ERROR: Unknown Error Enum)";
        break;
    }

    if (err != GL_NO_ERROR) {
      LOGE("CheckGLError: %s, (%s:%d)", str, file, line);
    }
  }
};

#if DEBUG
#define CheckGLError() OpenGLUtils::CheckGLError_(__FILE__, __LINE__)
#else
#define CheckGLError()
#endif

}