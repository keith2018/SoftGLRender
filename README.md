# SoftGLRender

Tiny C++ Software Renderer/Rasterizer, It implements the GPU's main rendering pipeline, including point, line, and polygon drawing, texture mapping, and emulates vertex shaders and fragment shaders, 3D models (GLTF) are loaded
by [assimp](https://github.com/assimp/assimp), and using [GLM](https://github.com/g-truc/glm) as math library. 

The project also adds OpenGL and Vulkan renderers implementation, so you can switch between them in real time while running.

<div align="center">

[![License](https://img.shields.io/badge/license-MIT-green)](./LICENSE)

[![CMake MacOS](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_macos.yml/badge.svg)](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_macos.yml)
[![CMake Windows](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_windows.yml/badge.svg)](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_windows.yml)
[![CMake Linux](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_linux.yml/badge.svg)](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_linux.yml)

</div>

![](screenshot/helmet.png)


#### Source Code Structure

```
src
├── Base/ - Basic utility classes.
├── Render/ - Renderer abstraction.
|   ├── Software/ - Software renderer implementation.
|   ├── OpenGL/ - OpenGL renderer implementation.
|   └── Vulkan/ - Vulkan renderer implementation.
└── Viewer/ -  Code for Viewer, mainly include GLTF loading (based on Assimp), camera & controller, 
    |          setting panel, and render pass management. 
    |          You can switch between software renderer and OpenGL renderer in real time.
    └── Shader/
        ├── GLSL/ - GLSL shader code.
        └── Software/ - Simulate vertex shader & fragment shader using c++, several basic shaders
                        are embed such as blinn-phong lighting, skybox, PBR & IBL, etc.
```

#### Renderer abstraction

```cpp
class Renderer {
 public:
  // framebuffer
  virtual std::shared_ptr<FrameBuffer> createFrameBuffer(bool offscreen) = 0;

  // texture
  virtual std::shared_ptr<Texture> createTexture(const TextureDesc &desc) = 0;

  // vertex
  virtual std::shared_ptr<VertexArrayObject> createVertexArrayObject(const VertexArray &vertexArray) = 0;

  // shader program
  virtual std::shared_ptr<ShaderProgram> createShaderProgram() = 0;

  // pipeline states
  virtual std::shared_ptr<PipelineStates> createPipelineStates(const RenderStates &renderStates) = 0;

  // uniform
  virtual std::shared_ptr<UniformBlock> createUniformBlock(const std::string &name, int size) = 0;
  virtual std::shared_ptr<UniformSampler> createUniformSampler(const std::string &name, const TextureDesc &desc) = 0;

  // pipeline
  virtual void beginRenderPass(std::shared_ptr<FrameBuffer> &frameBuffer, const ClearStates &states) = 0;
  virtual void setViewPort(int x, int y, int width, int height) = 0;
  virtual void setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) = 0;
  virtual void setShaderProgram(std::shared_ptr<ShaderProgram> &program) = 0;
  virtual void setShaderResources(std::shared_ptr<ShaderResources> &uniforms) = 0;
  virtual void setPipelineStates(std::shared_ptr<PipelineStates> &states) = 0;
  virtual void draw() = 0;
  virtual void endRenderPass() = 0;
};
```

#### Software Renderer Features

Pipeline

- [x] Vertex shading
- [x] View Frustum culling (line & triangle)
- [x] Perspective Correct Interpolation
- [x] Back-Front face culling
- [x] Point rasterization
- [x] Line rasterization
- [x] Triangle rasterization
- [x] Fragment Shading
- [x] Shader derivative `dFdx` `dFdy`
- [x] Depth test
- [x] Alpha blending
- [x] Reversed Z
- [x] Early Z
- [x] MSAA

Texture

- [x] Mipmaps
- [x] Sample parameters: `Lod`, `Bias`, `Offset`
- [x] Filtering
    - [x] NEAREST
    - [x] LINEAR
    - [x] NEAREST_MIPMAP_NEAREST
    - [x] LINEAR_MIPMAP_NEAREST
    - [x] NEAREST_MIPMAP_LINEAR
    - [x] LINEAR_MIPMAP_LINEAR
- [x] Wrapping
    - [x] REPEAT
    - [x] MIRRORED_REPEAT
    - [x] CLAMP_TO_EDGE
    - [x] CLAMP_TO_BORDER
    - [x] CLAMP_TO_ZERO
- [x] Image storage tiling and swizzling
  - [x] Linear: pixel values are stored line by line, commonly used as image RGBA buffer
  - [x] Tiled: block base storage, inside the block pixels are stored as `Linear`
  - [x] Morton: block base storage, inside the block pixels are stored as morton pattern (similar to zigzag)


#### Viewer Features

- [x] Settings panel
- [x] Orbit Camera Controller
- [x] Blinn-Phong shading
- [x] PBR & IBL shading
- [x] Skybox CubeMap & Equirectangular
- [x] FXAA
- [x] ShadowMap


#### Optimization

- Multi-Threading: rasterization is block based with multi-threading support, currently the triangle traversal algorithm
  needs to be optimized.
- SIMD: SIMD acceleration is used to optimize performance bottlenecks, such as barycentric coordinate calculation,
  shader's varying interpolation, etc.

## Showcase

### Render Textured

- BoomBox (PBR)
  ![](screenshot/boombox.png)

- Robot
  ![](screenshot/robot.png)

- DamagedHelmet (PBR)
  ![](screenshot/helmet.png)

- GlassTable
  ![](screenshot/glasstable.png)

- AfricanHead
  ![](screenshot/africanhead.png)

- Brickwall
  ![](screenshot/brickwall.png)

- Cube
  ![](screenshot/cube.png)

## Dependencies

* [glm](https://github.com/g-truc/glm)
* [glslang](https://github.com/KhronosGroup/glslang)
* [glfw](https://github.com/glfw/glfw)
* [json11](https://github.com/dropbox/json11)
* [stb_image](https://github.com/nothings/stb)
* [assimp](https://github.com/assimp/assimp)
* [imgui](https://github.com/ocornut/imgui)
* [vma](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

## Clone

```bash
git clone git@github.com:keith2018/SoftGLRender.git
cd SoftGLRender
git submodule update --init --recursive
```

## Build

```bash
mkdir build
cmake -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build --config Release
```

## Run

```bash
cd bin/Release
./SoftGLRender
```

## License

This code is licensed under the MIT License (see [LICENSE](LICENSE)).
