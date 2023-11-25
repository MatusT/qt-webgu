#include <QApplication>

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include "WebGPUWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    WebGPUWindow window;
    window.show();

    return application.exec();
}