# SoftGLRender

Tiny C++ Software Renderer/Rasterizer, it implements the main GPU rendering pipeline, 3D models (GLTF) are loaded
by [assimp](https://github.com/assimp/assimp), and using [GLM](https://github.com/g-truc/glm) as math library.

<div align="center">

[![License](https://img.shields.io/badge/license-MIT-green)](./LICENSE)

[![CMake MacOS](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_macos.yml/badge.svg)](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_macos.yml)
[![CMake Windows](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_windows.yml/badge.svg)](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_windows.yml)
[![CMake Linux](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_linux.yml/badge.svg)](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_linux.yml)

</div>

![](screenshot/helmet.png)

- [render](src/render):
    - [soft](src/render/soft): software renderer implementation
    - [opengl](src/render/opengl): OpenGL renderer implementation
- [view](src/view): code for Viewer, mainly include GLTF loading (based on Assimp), camera & controller, setting panel,
  and render pass management. You can switch between software renderer and OpenGL renderer in real time.
    - [shader/soft](src/view/shader/soft): simulate vertex shader & fragment shader using c++, several basic shaders are
      embed such as blinn-phong lighting, skybox, PBR & IBL, etc.
    - [shader/opengl](src/view/shader/opengl): GLSL shader code

#### Renderer abstraction

![](screenshot/pipeline.jpg)

```cpp
class Renderer {
 public:
  // framebuffer
  virtual std::shared_ptr<FrameBuffer> CreateFrameBuffer() = 0;

  // texture
  virtual std::shared_ptr<Texture> CreateTexture(const TextureDesc &desc) = 0;

  // vertex
  virtual std::shared_ptr<VertexArrayObject> CreateVertexArrayObject(const VertexArray &vertex_array) = 0;

  // shader program
  virtual std::shared_ptr<ShaderProgram> CreateShaderProgram() = 0;

  // uniform
  virtual std::shared_ptr<UniformBlock> CreateUniformBlock(const std::string &name, int size) = 0;
  virtual std::shared_ptr<UniformSampler> CreateUniformSampler(const std::string &name, TextureType type,
                                                               TextureFormat format) = 0;

  // pipeline
  virtual void SetFrameBuffer(std::shared_ptr<FrameBuffer> &frame_buffer) = 0;
  virtual void SetViewPort(int x, int y, int width, int height) = 0;
  virtual void Clear(const ClearState &state) = 0;
  virtual void SetRenderState(const RenderState &state) = 0;
  virtual void SetVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) = 0;
  virtual void SetShaderProgram(std::shared_ptr<ShaderProgram> &program) = 0;
  virtual void SetShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) = 0;
  virtual void Draw(PrimitiveType type) = 0;
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

* [GLM](https://github.com/g-truc/glm)
* [json11](https://github.com/dropbox/json11)
* [stb_image](https://github.com/nothings/stb)
* [assimp](https://github.com/assimp/assimp)
* [imgui](https://github.com/ocornut/imgui)

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
