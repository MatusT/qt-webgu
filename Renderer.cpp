#include "Renderer.hpp"
// #include <webgpu/webgpu.hpp>
#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/val.h>
using namespace emscripten;
#endif

Renderer::Renderer() : backgroundColor({0.0, 0.25, 0.8}) {}
Renderer::~Renderer() {}

#ifdef __EMSCRIPTEN__
void Renderer::drawEmscripten(uintptr_t devicePtr, uintptr_t queuePtr, uintptr_t textureViewPtr) {
  WGPUDevice device = reinterpret_cast<WGPUDevice>(devicePtr);
  WGPUQueue queue = reinterpret_cast<WGPUQueue>(queuePtr);
  WGPUTextureView textureView = reinterpret_cast<WGPUTextureView>(textureViewPtr);

  this->draw(device, queue, textureView);
}
#endif

void Renderer::draw(WGPUDevice device, WGPUQueue queue, WGPUTextureView textureView) {
  WGPUCommandEncoderDescriptor commandEncoderDescriptor = {.nextInChain = nullptr};
  WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDescriptor);

  WGPURenderPassColorAttachment colorAttachments[1] = {{
      .nextInChain = nullptr,
      .view = textureView,
      .resolveTarget = nullptr,
      .loadOp = WGPULoadOp_Clear,
      .storeOp = WGPUStoreOp_Store,
      .clearValue = {.r = this->backgroundColor[0], .g = this->backgroundColor[1], .b = this->backgroundColor[2], .a = 1.0},
  }};
  WGPURenderPassDescriptor renderPassDescriptor = {
      .nextInChain = nullptr,
      .label = nullptr,
      .colorAttachmentCount = 1,
      .colorAttachments = colorAttachments,
      .depthStencilAttachment = nullptr,
  };
  WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
  wgpuRenderPassEncoderEnd(renderPass);

  WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(commandEncoder, nullptr);

  wgpuQueueSubmit(queue, 1, &commandBuffer);
}

void Renderer::setBackgroundColor(double r, double g, double b) { this->backgroundColor = {r, g, b}; }

#ifdef __EMSCRIPTEN__
// Binding code
EMSCRIPTEN_BINDINGS(renderer) { class_<Renderer>("Renderer").constructor<>().function("drawEmscripten", &Renderer::drawEmscripten).function("setBackgroundColor", &Renderer::setBackgroundColor); }
#endif

// TODO: Figure out why webgpu.HPP doesn't work in Esmcripten?
// #pragma region Command Encoding
//   wgpu::CommandEncoder encoder = device.createCommandEncoder({});

// #pragma region Clear Render Pass
//   wgpu::RenderPassColorAttachment screenColorAttachment = wgpu::Default;
//   screenColorAttachment.view = textureView;
//   screenColorAttachment.loadOp = wgpu::LoadOp::Clear;
//   screenColorAttachment.storeOp = wgpu::StoreOp::Store;
//   screenColorAttachment.clearValue = {.r = 0.2, .g = 0.3, .b = 0.6, .a = 1.0};

//   std::vector<wgpu::RenderPassColorAttachment> colorAttachments{screenColorAttachment};

//   wgpu::RenderPassDescriptor renderPassDescriptor = wgpu::Default;
//   renderPassDescriptor.colorAttachmentCount = 1;
//   renderPassDescriptor.colorAttachments = colorAttachments.data();

//   wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDescriptor);
//   renderPass.end();
// #pragma endregion

//   wgpu::CommandBuffer command = encoder.finish();
// #pragma endregion

//   queue.submit(command);

//   command.release();
//   renderPass.release();
//   encoder.release();
