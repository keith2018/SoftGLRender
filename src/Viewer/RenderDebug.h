/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "renderdoc_app.h"

namespace SoftGL {
namespace View {

// use RenderDoc In-application API to dump frame
// see: https://renderdoc.org/docs/in_application_api.html

class RenderDebugger {
 public:
  static void startFrameCapture(RENDERDOC_DevicePointer device = nullptr);
  static void endFrameCapture(RENDERDOC_DevicePointer device = nullptr);

 private:
  static RENDERDOC_API_1_0_0* getRenderDocApi();

 private:
  static RENDERDOC_API_1_0_0* rdoc_;
};

}
}
