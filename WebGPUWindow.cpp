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
}

WebGPUWindow::~WebGPUWindow() {}

void WebGPUWindow::init() {
  this->instance = std::make_unique<wgpu::Instance>(wgpu::createInstance({}));

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

  wgpu::SurfaceConfiguration surfaceConfiguration = wgpu::Default;
  surfaceConfiguration.device = *(this->device);
  surfaceConfiguration.format = wgpu::TextureFormat::BGRA8Unorm;
  surfaceConfiguration.usage = wgpu::TextureUsage::RenderAttachment;
  surfaceConfiguration.alphaMode = wgpu::CompositeAlphaMode::Opaque;
  surfaceConfiguration.width = (uint32_t)this->width();
  surfaceConfiguration.height = (uint32_t)this->height();
  surfaceConfiguration.presentMode = wgpu::PresentMode::Fifo;
  this->surface->configure(surfaceConfiguration);
  this->queue = std::make_unique<wgpu::Queue>(this->device->getQueue());

  // Future: initialize with device here to load shaders etc...
  this->renderer = std::make_unique<Renderer>();

  this->initialized = true;
}

void WebGPUWindow::draw() {
  if (!this->initialized) {
    return;
  }

#pragma region Aquirement of Screen Texture
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
#pragma endregion

  this->renderer->draw(*this->device, *this->queue, currentTextureView);

  this->surface->present();

  wgpuTextureViewRelease(currentTextureView);

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