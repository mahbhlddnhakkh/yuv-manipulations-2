#include <myyuv.hpp>
#include <SDL3/SDL.h>
#include <iostream>
#include <cassert>
#include <fstream>

SDL_Surface* create_surface_from_path(const std::string& path, uint8_t*& surface_data) {
  surface_data = nullptr;
  myyuv::BMPHeader bmp_header;
  myyuv::YUVHeader yuv_header;
  const uint32_t magic_size = std::max(sizeof(bmp_header.type), sizeof(yuv_header.type));
  uint8_t magic[magic_size];
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Error opening file to read " + path);
  }
  f.read(reinterpret_cast<char*>(magic), sizeof(magic));
  f.close();
  SDL_Surface* surf = nullptr;
  if (std::equal(bmp_header.type, bmp_header.type + sizeof(bmp_header.type), magic)) {
    static myyuv::BMP bmp(path);
    assert(bmp.header.width > 0 && bmp.header.height > 0);
    if (!bmp.isValid()) {
      throw std::runtime_error("Invalid bmp");
    }
    uint8_t* data = bmp.colorData();
    surface_data = data;
    if (bmp.header.bit_count == 24) {
      surf = SDL_CreateSurfaceFrom(bmp.trueWidth(), bmp.trueHeight(), SDL_PIXELFORMAT_RGB24, data, 3*bmp.trueWidth());
    } else if (bmp.header.bit_count == 32) {
      SDL_PixelFormat format;
      if (bmp.color_header.alpha_mask == 0) {
        format = SDL_PIXELFORMAT_XRGB8888;
      } else {
        format = SDL_PIXELFORMAT_ARGB8888;
      }
      surf = SDL_CreateSurfaceFrom(bmp.trueWidth(), bmp.trueHeight(), format, data, 4*bmp.trueWidth());
    }
  } else if (std::equal(yuv_header.type, yuv_header.type + sizeof(yuv_header.type), magic)) {
    static myyuv::YUV yuv(path);
    if (yuv.isCompressed()) {
      yuv = yuv.decompress();
    }
    if (!yuv.isValid()) {
      throw std::runtime_error("Invalid yuv");
    }
    uint32_t yuv_data_size = yuv.getDataSize();
    surface_data = new uint8_t[yuv_data_size];
    std::copy(yuv.data, yuv.data + yuv_data_size, surface_data);
    surf = SDL_CreateSurfaceFrom(yuv.header.width, yuv.header.height, static_cast<SDL_PixelFormat>(yuv.header.fourcc_format), surface_data, yuv.header.width);
  } else {
    throw std::runtime_error("Unknown image format (magic) " + path);
  }
  return surf;
}

int main(int argc, char* argv[]) {
  if (argc <= 1 || argc > 2) {
    std::cout << "A BMP and YUV image viewer with SDL3 as a backend. Press ESCAPE to exit.\n"
    << "Usage:\n"
    << "myyuv_sdl3 /path/to/image.myyuv\n";
    return 0;
  }
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::cout << "Error initializing SDL: " << SDL_GetError() << '\n';
    return 1;
  }
  uint8_t* surface_data;
  SDL_Surface* surf = create_surface_from_path(argv[1], surface_data);
  if (surf == nullptr) {
    std::cout << "Error creating surface" << '\n';
    return 1;
  }
  SDL_Window* win = SDL_CreateWindow("YUV SDL3 Viewer", surf->w, surf->h, 0);
  SDL_Renderer* renderer = SDL_CreateRenderer(win, nullptr);
  assert(renderer);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
  SDL_DestroySurface(surf);
  delete[] surface_data;
  assert(texture);
  SDL_RenderClear(renderer);
  SDL_RenderTexture(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
  SDL_DestroyTexture(texture);
  bool close = false;
  while (!close) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch(event.type) {
        case SDL_EVENT_QUIT:
          close = true;
          break;
        case SDL_EVENT_KEY_UP:
          if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
            close = true;
          }
          break;
      }
    }
    SDL_Delay(40);
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}
