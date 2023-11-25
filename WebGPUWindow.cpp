#include "WebGPUWindow.hpp"

#include <QDebug>
#include <QEvent>
#include <QPlatformSurfaceEvent>

#include <cassert>
#include <memory>
#include <vector>

#include <Windows.h>

#include <webgpu/webgpu.hpp>

WebGPUWindow::WebGPUWindow(QWindow *parent) : QWindow(parent) {
  setSurfaceType(QWindow::Direct3DSurface);

  qDebug() << "Surface type " << this->surfaceType();
  qDebug() << "Surface format " << this->surfaceClass();
}

WebGPUWindow::~WebGPUWindow() {}

void WebGPUWindow::init() {
  this->instance = std::make_unique<wgpu::Instance>(wgpu::createInstance({}));

  if (!instance) {
    qDebug() << "Could not initialize WebGPU!";
  }

  HWND hwnd = reinterpret_cast<HWND>(this->winId());
  HINSTANCE hinstance = GetModuleHandle(NULL);

  wgpu::SurfaceDescriptorFromWindowsHWND surfaceDescriptorFromHWND = wgpu::Default;
  surfaceDescriptorFromHWND.hinstance = hinstance;
  surfaceDescriptorFromHWND.hwnd = hwnd;
  wgpu::SurfaceDescriptor surfaceDescriptor = wgpu::Default;
  surfaceDescriptor.nextInChain = reinterpret_cast<const WGPUChainedStruct *>(&surfaceDescriptorFromHWND);
  this->surface = std::make_unique<wgpu::Surface>(this->instance->createSurface(surfaceDescriptor));

  wgpu::RequestAdapterOptions adapterOptions = wgpu::Default;
  adapterOptions.compatibleSurface = *(this->surface);
  adapterOptions.backendType = wgpu::BackendType::D3D12;

  this->adapter = std::make_unique<wgpu::Adapter>(this->instance->requestAdapter(adapterOptions));
  this->device = std::make_unique<wgpu::Device>(this->adapter->requestDevice({}));

  qDebug() << this->surface->getPreferredFormat(*this->adapter);

  wgpu::SurfaceConfiguration surfaceConfiguration = wgpu::Default;
  surfaceConfiguration.device = *(this->device);
  surfaceConfiguration.format = wgpu::TextureFormat::BGRA8Unorm;
  surfaceConfiguration.usage = wgpu::TextureUsage::RenderAttachment;
  surfaceConfiguration.alphaMode = wgpu::CompositeAlphaMode::Opaque;
  surfaceConfiguration.width = (uint32_t)this->width();
  surfaceConfiguration.height = (uint32_t)this->height();
  surfaceConfiguration.presentMode = wgpu::PresentMode::Fifo;
  this->surface->configure(surfaceConfiguration);

  // auto onDeviceError = [](WGPUErrorType type, char const *message,
  //                         void * /* pUserData */) {
  //   qDebug() << "Uncaptured device error: type " << type;
  //   if (message)
  //     qDebug() << " (" << message << ")";
  // };
  // wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError,
  //                                      nullptr /* pUserData */);

  // this->queue = wgpuDeviceGetQueue(this->device);
  this->queue = std::make_unique<wgpu::Queue>(this->device->getQueue());

  this->initialized = true;
}

void WebGPUWindow::draw() {
  if (!this->initialized) {
    return;
  }

  wgpu::SurfaceTexture currentTexture;
  this->surface->getCurrentTexture(&currentTexture);

  if (currentTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
    if (currentTexture.texture != NULL) {
      wgpuTextureRelease(currentTexture.texture);
    }

    wgpu::SurfaceConfiguration surfaceConfiguration = wgpu::Default;
    surfaceConfiguration.device = *(this->device);
    surfaceConfiguration.format = wgpu::TextureFormat::BGRA8Unorm;
    surfaceConfiguration.usage = wgpu::TextureUsage::RenderAttachment;
    surfaceConfiguration.alphaMode = wgpu::CompositeAlphaMode::Opaque;
    surfaceConfiguration.width = (uint32_t)this->width();
    surfaceConfiguration.height = (uint32_t)this->height();
    surfaceConfiguration.presentMode = wgpu::PresentMode::Fifo;
    this->surface->configure(surfaceConfiguration);

    this->requestUpdate();

    return;
  }

  WGPUTextureView currentTextureView = wgpuTextureCreateView(currentTexture.texture, NULL);

  // We create a command encoder to be able to create command buffers
  WGPUCommandEncoderDescriptor encoderDesc = {};
  encoderDesc.nextInChain = nullptr;
  encoderDesc.label = "My command encoder";
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(*this->device, &encoderDesc);

  // Encode commands into a command buffer
  WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
  cmdBufferDescriptor.nextInChain = nullptr;
  cmdBufferDescriptor.label = "Command buffer";

  // Rende Pass
  WGPURenderPassColorAttachment colorAttachments[1] = {{.nextInChain = nullptr,
                                                        .view = currentTextureView,
                                                        .resolveTarget = NULL,
                                                        .loadOp = WGPULoadOp_Clear,
                                                        .storeOp = WGPUStoreOp_Store,
                                                        .clearValue{
                                                            .r = 0.2,
                                                            .g = 0.3,
                                                            .b = 0.6,
                                                            .a = 1.0,
                                                        }}};
  WGPURenderPassDescriptor renderPassDesc = {
      .nextInChain = nullptr,
      .label = nullptr,
      .colorAttachmentCount = 1,
      .colorAttachments = colorAttachments,
      .depthStencilAttachment = nullptr,
  };
  WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
  wgpuRenderPassEncoderEnd(renderPass);

  WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);

  // Finally submit the command queue
  wgpuQueueSubmit(*this->queue, 1, &command);
  wgpuSurfacePresent(*this->surface);

  wgpuCommandBufferRelease(command);
  wgpuRenderPassEncoderRelease(renderPass);
  wgpuCommandEncoderRelease(encoder);
  wgpuTextureViewRelease(currentTextureView);
  wgpuTextureRelease(currentTexture.texture);

  this->requestUpdate();
}

void WebGPUWindow::exposeEvent(QExposeEvent *) {
  if (isExposed()) {
    this->init();
  }
}

bool WebGPUWindow::event(QEvent *event) {
  switch (event->type()) {
  case QEvent::UpdateRequest:
    this->draw();
    break;
  case QEvent::PlatformSurface:
    if (static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
      // destroy surface
    }
    break;
  }

  return QWindow::event(event);
}

void WebGPUWindow::resizeEvent(QResizeEvent *resizeEvent) { Q_UNUSED(resizeEvent); }

void WebGPUWindow::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  this->draw();
}