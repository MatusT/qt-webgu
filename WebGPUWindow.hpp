#pragma once

#ifndef WEBGPUWINDOW
#define WEBGPUWINDOW

#include <QWindow>

#include <memory>

#include <webgpu/webgpu.hpp>

class WebGPUWindow : public QWindow {
  Q_OBJECT

public:
  WebGPUWindow(QWindow *parent = nullptr);
  virtual ~WebGPUWindow();

//   QPaintEngine* paintEngine() const override;

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent *event) override;
  bool event(QEvent* event) override;
  void exposeEvent(QExposeEvent *) override;

private:
  std::unique_ptr<wgpu::Instance> instance;
  std::unique_ptr<wgpu::Adapter> adapter;
  std::unique_ptr<wgpu::Surface> surface;
  std::unique_ptr<wgpu::Device> device;
  std::unique_ptr<wgpu::Queue> queue;

  void init();
  void draw();

  bool initialized = false;
};

#endif