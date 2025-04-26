# yuv-manipulations-2

## Requirements:
- Compiler with C++17 support, cmake
- OpenMP: optional, required if myyuv_lib is built with `-D MYYUV_USE_OPENMP=ON`
- OpenGL, GLFW3, GLEW, GLM: optional, required for everything inside `myyuv_opengl`
- SDL3: optional, required for `myyuv_sdl3`

## Build:
```bash
mkdir -p build
cd build
cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build . --target all
cd ..
```
or
```bash
mkdir -p build
cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
cmake --build . --target all
cd ..
```
You can also use `-D MYYUV_USE_OPENMP=ON` to build with OpenMP support for parallel DCT compression and decompression. OpenMP is disabled by default.

## Run:
### `libmyyuv_lib`
A library for YUV and BMP images.

### `myyuv_cli`
A cli tool to create YUV images from BMP images and compress/decompress them.
<details><summary>myyuv_cli usage</summary>

```
Usage:
/path/to/image -info
/path/to/image.bmp -to_yuv format -o /path/to/new_image.myyuv
/path/to/image.myyuv -compress compression [params...] -o /path/to/new_image.myyuv
/path/to/image.myyuv -decompress -o /path/to/new_image.myyuv

YUV formats:
IYUV

Compression formats for YUV:
DCT
```

</details>

### `myyuv_sdl3`
A BMP and YUV image viewer with SDL3 as a backend.
<details><summary>myyuv_sdl3 usage</summary>

```
Usage: /path/to/image.myyuv
```

</details>

### `myyuv_opengl_shared`
A shared library for OpenGL programs for shared functionality.

### `myyuv_opengl_viewer`
A BMP and YUV image viewer with OpenGL as a backend.

<details><summary>myyuv_opengl_viewer usage</summary>

```
Usage: /path/to/image.myyuv
```

</details>

### `myyuv_opengl_spinning_cube`
A spinning cube (or parallelepiped) with BMP and YUV image as a texture.
<details><summary>myyuv_opengl_spinning_cube usage</summary>

```
Usage:
/path/to/image.myyuv
-force_cube /path/to/image.myyuv
-flip_width_height /path/to/image.myyuv
```

</details>

## YUV formats:
- `IYUV`: YUV 4:2:0 with planar storage type.

## BMP formats:
- `XRGB8888` on little-endian tested

## TODOs:
- Change (most) asserts to C++ exceptions
- Add tests?
- Account for endian
- Add more OpenGL examples
- Test 24 bit BMP

## Resources
- Chef with trumpet: https://heic.digital/samples/
- Learn OpenGL: https://learnopengl.com/
