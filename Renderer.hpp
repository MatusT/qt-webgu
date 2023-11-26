#pragma once

#include <memory>

#include <webgpu/webgpu.h>

class Renderer {

public:
  Renderer();
  virtual ~Renderer();
  void draw(WGPUDevice device, WGPUQueue queue, WGPUTextureView textureView);
};