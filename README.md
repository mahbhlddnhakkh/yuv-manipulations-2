# yuv-manipulations-2

### Requirements:
- Compiler with C++17 support, cmake
- OpenGL, GLFW3, GLEW: optional, required for everything inside `myyuv_opengl`
- SDL3: optional, required for `myyuv_sdl3`

### Build:
```
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build . --target all
cd ..
```

### Run:
- myyuv_lib - library for YUV images (+BMP)
- myyuv_cli - cli tool to create YUV images from BMP images and compress/decompress them
- myyuv_sdl3 - BMP and YUV image viewer with SDL3 as a backend
- myyuv_opengl_viewer - BMP and YUV image viewer with OpenGL as a backend

### YUV formats:
- `IYUV`: YUV 4:2:0 with planar storage type.

### BMP formats:
- `XRGB8888` on little-endian tested

### TODOs:
- Change (most) asserts to C++ exceptions
- Add tests?
- Account for endian
- Add more OpenGL examples
- Test 24 bit BMP

### Resources
- Chef with trumpet: https://heic.digital/samples/
