# qt-webgu

![Showcase](/screenshots/main.png)

Example of creating project that shares code between Qt QWidgets and Web.
In this example, there is Renderer class responsible for rendering 3D scene.
Emscripten compiles this class down to WebAssembly. In order to pass WebGPU's
Device, Queue, etc... objects they are turned into a 32-bit pointer using
shim layer provided by emscripten.

Qt side embeds renderer inside QWindow that retrieves its native window handle
in order to create native surface. This QWindow is then integrade into QWidgets
project thanks to createWindowContainer function of Qt 5.1+.

Tested on Windows 11, DX12.

# Dependencies
- C++20 compiler (Visual Studio 2019+)
- Qt 6 + (5 is enough but this was tested with 6) with cmake and ninja installed
- Emscripten

# Building

## Qt Version

Open Qt Creator and build it!. The only dependency is Qt.

## Web version

[ There is already a prebuilt version in build-wasm. ]
You need  to install emscripten sdk and then run the build using enmake cmake

```
mkdir build-wasm
cd build-wasm
enmake cmake ..
cmake --build .
```

# Running

## Qt Version

Hit run

## Web version

Open index.html in any live server (like the one VS Code has)

# TODO
- Figure out why only DX12 works in Qt
