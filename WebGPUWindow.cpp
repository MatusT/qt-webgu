#include "WebGPUWindow.hpp"

#include <QDebug>
#include <QEvent>
#include <QPlatformSurfaceEvent>

#include <webgpu.h>

#include <cassert>
#include <vector>

#include <Windows.h>

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapter(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter requestAdapter(WGPUInstance instance,
                           WGPURequestAdapterOptions const *options) {
  // A simple structure holding the local information shared with the
  // onAdapterRequestEnded callback.
  struct UserData {
    WGPUAdapter adapter = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  // Callback called by wgpuInstanceRequestAdapter when the request returns
  // This is a C++ lambda function, but could be any function defined in the
  // global scope. It must be non-capturing (the brackets [] are empty) so
  // that it behaves like a regular C function pointer, which is what
  // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
  // is to convey what we want to capture through the pUserData pointer,
  // provided as the last argument of wgpuInstanceRequestAdapter and received
  // by the callback as its last argument.
  auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status,
                                  WGPUAdapter adapter, char const *message,
                                  void *pUserData) {
    UserData &userData = *reinterpret_cast<UserData *>(pUserData);
    if (status == WGPURequestAdapterStatus_Success) {
      userData.adapter = adapter;
    } else {
      qDebug() << "Could not get WebGPU adapter: " << message;
    }
    userData.requestEnded = true;
  };

  // Call to the WebGPU request adapter procedure
  wgpuInstanceRequestAdapter(instance /* equivalent of navigator.gpu */,
                             options, onAdapterRequestEnded, (void *)&userData);

  // In theory we should wait until onAdapterReady has been called, which
  // could take some time (what the 'await' keyword does in the JavaScript
  // code). In practice, we know that when the wgpuInstanceRequestAdapter()
  // function returns its callback has been called.
  assert(userData.requestEnded);

  return userData.adapter;
}

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDevice(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice requestDevice(WGPUAdapter adapter,
                         WGPUDeviceDescriptor const *descriptor) {
  struct UserData {
    WGPUDevice device = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status,
                                 WGPUDevice device, char const *message,
                                 void *pUserData) {
    UserData &userData = *reinterpret_cast<UserData *>(pUserData);
    if (status == WGPURequestDeviceStatus_Success) {
      userData.device = device;
    } else {
      qDebug() << "Could not get WebGPU device: " << message;
    }
    userData.requestEnded = true;
  };

  wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded,
                           (void *)&userData);

  assert(userData.requestEnded);

  return userData.device;
}

WebGPUWindow::WebGPUWindow(QWindow *parent) : QWindow(parent) {
  setSurfaceType(QWindow::RasterSurface);

  qDebug() << "Surface type " << this->surfaceType();
  qDebug() << "Surface format " << this->surfaceClass();
}

WebGPUWindow::~WebGPUWindow() {}

void WebGPUWindow::init() {
  // 1. We create a descriptor
  WGPUInstanceDescriptor desc = {};
  desc.nextInChain = nullptr;

  // 2. We create the instance using this descriptor
  this->instance = wgpuCreateInstance(&desc);

  // 3. We can check whether there is actually an instance created
  if (!instance) {
    qDebug() << "Could not initialize WebGPU!";
  }

  // 4. Display the object (WGPUInstance is a simple pointer, it may be
  // copied around without worrying about its size).
  qDebug() << "WGPU instance: " << instance;

  HWND hwnd = reinterpret_cast<HWND>(this->winId());
  HINSTANCE hinstance = GetModuleHandle(NULL);

  const WGPUSurfaceDescriptorFromWindowsHWND hwnd_descriptor = {
      .chain =
          {
              .next = NULL,
              .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
          },
      .hinstance = hinstance,
      .hwnd = hwnd,
  };

  const WGPUSurfaceDescriptor surface_descriptor = {
      .nextInChain =
          reinterpret_cast<const WGPUChainedStruct *>(&hwnd_descriptor),
      .label = NULL,
  };

  this->surface =
      wgpuInstanceCreateSurface(this->instance, &surface_descriptor);

  qDebug() << "Got surface " << this->surface;

  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.nextInChain = nullptr;
  adapterOpts.compatibleSurface = this->surface;
  adapterOpts.backendType = WGPUBackendType_Vulkan;

  this->adapter = requestAdapter(instance, &adapterOpts);

  qDebug() << "Got adapter: " << this->adapter;

  std::vector<WGPUFeatureName> features;

  // // Call the function a first time with a null return address, just to get
  // // the entry count.
  // size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);

  // // Allocate memory (could be a new, or a malloc() if this were a C program)
  // features.resize(featureCount);

  // // Call the function a second time, with a non-null return address
  // wgpuAdapterEnumerateFeatures(adapter, features.data());

  // qDebug() << "Adapter features:";
  // for (auto f : features) {
  //   qDebug() << " - " << f;
  // }

  WGPUDeviceDescriptor deviceDesc = {};
  deviceDesc.nextInChain = nullptr;
  deviceDesc.label = "My Device";      // anything works here, that's your call
  deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
  deviceDesc.defaultQueue.nextInChain = nullptr;
  deviceDesc.defaultQueue.label = "The default queue";
  this->device = requestDevice(this->adapter, &deviceDesc);

  qDebug() << "Got device: " << device;

  const WGPUSurfaceConfiguration surface_configuration = {
      .nextInChain = nullptr,
      .device = this->device,
      .format = WGPUTextureFormat_BGRA8Unorm,
      .usage = WGPUTextureUsage_RenderAttachment,
      .viewFormatCount = 0,
      .viewFormats = nullptr,
      .alphaMode = WGPUCompositeAlphaMode_Opaque,
      .width = (uint32_t)this->width(),
      .height = (uint32_t)this->height(),
      .presentMode = WGPUPresentMode_Fifo};

  wgpuSurfaceConfigure(this->surface, &surface_configuration);

  auto onDeviceError = [](WGPUErrorType type, char const *message,
                          void * /* pUserData */) {
    qDebug() << "Uncaptured device error: type " << type;
    if (message)
      qDebug() << " (" << message << ")";
  };
  wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError,
                                       nullptr /* pUserData */);

  this->queue = wgpuDeviceGetQueue(this->device);

  this->initialized = true;
}

void WebGPUWindow::draw() {
  if (!this->initialized) {
    return;
  }

  qDebug() << "Draw " << this->winId();
  WGPUSurfaceTexture currentTexture;
  wgpuSurfaceGetCurrentTexture(this->surface, &currentTexture);

  if (currentTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
    if (currentTexture.texture != NULL) {
      wgpuTextureRelease(currentTexture.texture);
    }

    const WGPUSurfaceConfiguration surface_configuration = {
        .nextInChain = nullptr,
        .device = this->device,
        .format = WGPUTextureFormat_BGRA8Unorm,
        .usage = WGPUTextureUsage_RenderAttachment,
        .viewFormatCount = 0,
        .viewFormats = nullptr,
        .alphaMode = WGPUCompositeAlphaMode_Opaque,
        .width = (uint32_t)this->width(),
        .height = (uint32_t)this->height(),
        .presentMode = WGPUPresentMode_Fifo};

    wgpuSurfaceConfigure(this->surface, &surface_configuration);

    qDebug() << "Had to recreate surface";

    this->requestUpdate();

    return;
  }

  WGPUTextureView currentTextureView =
      wgpuTextureCreateView(currentTexture.texture, NULL);

  // We create a command encoder to be able to create command buffers
  WGPUCommandEncoderDescriptor encoderDesc = {};
  encoderDesc.nextInChain = nullptr;
  encoderDesc.label = "My command encoder";
  WGPUCommandEncoder encoder =
      wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

  // Encode commands into a command buffer
  WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
  cmdBufferDescriptor.nextInChain = nullptr;
  cmdBufferDescriptor.label = "Command buffer";

  // Rende Pass
  WGPURenderPassColorAttachment colorAttachments[1] = {
      {.nextInChain = nullptr,
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
  WGPURenderPassEncoder renderPass =
      wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
  wgpuRenderPassEncoderEnd(renderPass);

  WGPUCommandBuffer command =
      wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);

  // Finally submit the command queue
  wgpuQueueSubmit(queue, 1, &command);
  wgpuSurfacePresent(this->surface);

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

void WebGPUWindow::paintEvent(QPaintEvent *event) {
  qDebug() << "Paint event" << event << " is visible " << this->isVisible()
           << " is exposed " << this->isExposed();

  if (!this->initialized || !this->adapter || !this->device || !this->surface) {
    return;
  }

  this->draw();
}

bool WebGPUWindow::event(QEvent *event) {
  switch (event->type()) {
  case QEvent::UpdateRequest:
    this->draw();
    break;
  case QEvent::PlatformSurface:
    if (static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() ==
        QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
      qDebug() << "platform surface";
    }
    qDebug() << "surface event" << static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType();
    break;
  }

  return QWindow::event(event);
}

void WebGPUWindow::resizeEvent(QResizeEvent *resizeEvent) {
  Q_UNUSED(resizeEvent);
}
