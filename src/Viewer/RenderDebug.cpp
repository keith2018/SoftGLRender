/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "RenderDebug.h"
#include "Base/Platform.h"
#include "Base/Logger.h"

#ifdef PLATFORM_WINDOWS
#include "windows.h"
#endif

namespace SoftGL {
namespace View {

#define RENDER_DOC_LIBRARY "renderdoc.dll"
#define RENDER_DOC_FUNC_GetAPI "RENDERDOC_GetAPI"

RENDERDOC_API_1_0_0 *RenderDebugger::rdoc_ = nullptr;

void RenderDebugger::startFrameCapture(RENDERDOC_DevicePointer device) {
  if (!rdoc_) {
    rdoc_ = getRenderDocApi();
  }

  if (rdoc_) {
    rdoc_->StartFrameCapture(device, nullptr);
  }
}

void RenderDebugger::endFrameCapture(RENDERDOC_DevicePointer device) {
  if (!rdoc_) {
    rdoc_ = getRenderDocApi();
  }

  if (rdoc_) {
    rdoc_->EndFrameCapture(device, nullptr);
  }
}

RENDERDOC_API_1_0_0 *RenderDebugger::getRenderDocApi() {
  RENDERDOC_API_1_0_0 *rdoc = nullptr;
#ifdef PLATFORM_WINDOWS
  HMODULE module = LoadLibrary(RENDER_DOC_LIBRARY);

  if (module == nullptr) {
    LOGE("load %s failed!", RENDER_DOC_LIBRARY);
    return nullptr;
  }

  pRENDERDOC_GetAPI getApi = nullptr;
  getApi = (pRENDERDOC_GetAPI) GetProcAddress(module, RENDER_DOC_FUNC_GetAPI);

  if (getApi == nullptr) {
    LOGE("GetProcAddress '%s' failed!", RENDER_DOC_FUNC_GetAPI);
    return nullptr;
  }

  if (getApi(eRENDERDOC_API_Version_1_0_0, (void **) &rdoc) != 1) {
    LOGE("RenderDoc call getApi failed!");
    return nullptr;
  }
#endif
  return rdoc;
}

}
}
