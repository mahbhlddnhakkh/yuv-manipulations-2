# yuv-manipulations-2

### Requirements:
- Compiler with C++11 support, cmake
- OpenGL, GLFW3, GLEW: optional, required for everything inside `myyuv_opengl`
- SDL3: optional, required for `myyuv_sdl3`

### Build:
```
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build .
cd ..
```

### Run:
TODO

### YUV formats:
- `IYUV`: YUV 4:2:0 with planar storage type.

### TODOs:
- Change (most) asserts to C++ exceptions
- Add compression
- Add tests?

### Resources
- Chef with trumpet: https://heic.digital/samples/
