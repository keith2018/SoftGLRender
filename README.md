# SoftGLRender
Tiny C++ Software Renderer/Rasterizer, it implements the main GPU rendering pipeline, 3D models (GLTF) are loaded by [assimp](https://github.com/assimp/assimp), and using [GLM](https://github.com/g-truc/glm) as math library.

[![CMake MacOS](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_macos.yml/badge.svg)](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_macos.yml)
[![CMake Windows](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_windows.yml/badge.svg)](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_windows.yml)
[![CMake Linux](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_linux.yml/badge.svg)](https://github.com/keith2018/SoftGLRender/actions/workflows/cmake_linux.yml)


![](screenshot/helmet.png)

## Features
- Wireframe
- View Frustum culling
- Back-Front culling
- Orbit Camera Controller
- Perspective Correct Interpolation
- Reversed Z
- Early Z
- Tangent Space Normal Mapping
- Basic Lighting
- Blinn-Phong shading
- PBR & IBL shading
- Skybox CubeMap & Equirectangular
- Texture mipmaps
- Texture tiling and swizzling (linear, tiled, morton)
- Texture filtering and wrapping
- Shader varying partial derivative `dFdx` `dFdy`
- Alpha mask & blend

Texture Filtering
  - NEAREST
  - LINEAR
  - NEAREST_MIPMAP_NEAREST
  - LINEAR_MIPMAP_NEAREST
  - NEAREST_MIPMAP_LINEAR
  - LINEAR_MIPMAP_LINEAR

Texture Wrapping
  - REPEAT
  - MIRRORED_REPEAT
  - CLAMP_TO_EDGE
  - CLAMP_TO_BORDER
  - CLAMP_TO_ZERO

Texture Fetch
  - Lod
  - Bias
  - Offset

Anti Aliasing
  - SSAA
  - FXAA


## TODO
- [ ] MSAA\TAA
- [ ] Shadow Map


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


### Render Wireframe

Check "show clip" to show the triangles created by frustum clip

![](screenshot/clip.png)


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
