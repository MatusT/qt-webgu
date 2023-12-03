#include <QApplication>
#include <QDebug>
#include <QGridLayout>
#include <QMainWindow>
#include <QObject>
#include <QSlider>
#include <QWidget>


#include "WebGPUWindow.hpp"

int main(int argc, char *argv[]) {
  QApplication application(argc, argv);

  QMainWindow window;
  window.setMinimumWidth(1280);
  window.setMinimumHeight(720);
  QWidget *w = new QWidget(&window);

  WebGPUWindow webgpuWindow;
  QWidget *windowContainer = QWidget::createWindowContainer(&webgpuWindow);

  QGridLayout grid;
  QSlider colorSlider;
  colorSlider.setMinimum(0);
  colorSlider.setMaximum(255);

  grid.addWidget(&colorSlider, 0, 0, 1, 1);
  grid.addWidget(windowContainer, 0, 1, 1, 23);

  w->setLayout(&grid);

  QObject::connect(&colorSlider, &QSlider::valueChanged, [&](int value) { webgpuWindow.setBackgroundColor(value / 255.0, webgpuWindow.g, webgpuWindow.b); });

  window.setCentralWidget(w);
  window.show();

  return application.exec();
}