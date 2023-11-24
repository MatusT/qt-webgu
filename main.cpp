#include <QApplication>

#include "WebGPUWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    WebGPUWindow window;
    window.show();

    return application.exec();
}