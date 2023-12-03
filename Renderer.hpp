#pragma once

#include <array>
#include <memory>


#include <webgpu/webgpu.h>

class Renderer {

public:
  Renderer();
  virtual ~Renderer();

#ifdef __EMSCRIPTEN__
  void drawEmscripten(uintptr_t device, uintptr_t queue, uintptr_t textureView);
#endif
  void draw(WGPUDevice device, WGPUQueue queue, WGPUTextureView textureView);

  void setBackgroundColor(double r, double g, double b);

private:
  std::array<double, 3> backgroundColor;
};

void draw(WGPUDevice device, WGPUQueue queue, WGPUTextureView textureView);