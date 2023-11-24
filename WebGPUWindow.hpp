#pragma once

#include <QWindow>

#include <webgpu.h>

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
  WGPUInstance instance = nullptr;
  WGPUAdapter adapter = nullptr;
  WGPUSurface surface = nullptr;
  WGPUDevice device = nullptr;
  WGPUQueue queue = nullptr;

  void init();
  void draw();

  bool initialized = false;
};