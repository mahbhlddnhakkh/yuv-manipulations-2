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
A library for YUV and BMP images. Note: compression works only with images whose width and height are divisible integer by 16. For YUV (IYUV) conversion image width and height must be a divisible integer by 2.

### `myyuv_cli`
A cli tool to create YUV images from BMP images and compress/decompress them.
<details><summary>myyuv_cli usage</summary>

```
Usage:
`myyuv_cli /path/to/image -info` - prints info about BMP or YUV image `/path/to/image`
`myyuv_cli /path/to/image.bmp -to_yuv format -o /path/to/new_image.myyuv` - creates YUV image from BMP image `/path/to/image.bmp` with `format` format and saves at `/path/to/new_image.myyuv`
`myyuv_cli /path/to/image.myyuv -compress compression [params...] -o /path/to/new_image.myyuv` - compresses YUV image `/path/to/image.myyuv` with `compression` using `params...` and saves at `/path/to/new_image.myyuv`
`myyuv_cli /path/to/image.myyuv -decompress -o /path/to/new_image.myyuv` - decompresses YUV image `/path/to/image.myyuv` and saves at `/path/to/new_image.myyuv`

YUV formats:
IYUV

Compression formats for YUV:
DCT
```
For example:
```
myyuv_cli /path/to/image.bmp -to_yuv IYUV -o /path/to/new_image.myyuv
myyuv_cli /path/to/image.myyuv -compress DCT 50 -o /path/to/new_image.myyuv
```

</details>

### `myyuv_sdl3`
A BMP and YUV image viewer with SDL3 as a backend. Press ESCAPE to exit.
<details><summary>myyuv_sdl3 usage</summary>

```
Usage:
myyuv_sdl3 /path/to/image.myyuv
```

</details>

### `myyuv_opengl_shared`
A shared library for OpenGL programs for shared functionality.

### `myyuv_opengl_viewer`
A BMP and YUV image viewer with OpenGL as a backend. Press ESCAPE to exit.

<details><summary>myyuv_opengl_viewer usage</summary>

```
Usage:
myyuv_opengl_viewer /path/to/image.myyuv
```

</details>

### `myyuv_opengl_spinning_cube`
A spinning cube (or parallelepiped) with BMP and YUV image as a texture. Move around with WASD, look around with arrow keys, fly up with SPACE and fly down with SHIFT. The camera is an airplane-like. Press ESCAPE to exit.
<details><summary>myyuv_opengl_spinning_cube usage</summary>

```
Usage:
myyuv_opengl_spinning_cube /path/to/image.myyuv [params]
Params:
`-shapes n` - creates `n` shapes, where `n` is a number between 1 and 1000
`-force_cube` - forces shape with texture into a cube even if the image width and height are not equal
`-flip_width_height` - flips width and height of a texture. This will affect only the shape. Does nothing if the shape is cube
```
For example:
```
myyuv_opengl_spinning_cube /path/to/image.myyuv -force_cube -shapes 10
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
