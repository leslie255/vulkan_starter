# Vulkan Starter Template

**Starter template for C++ Vulkan gamedev projects**

## What it contains

This starter template contains the bare-minimum code that creates a GLFW window, and draws a quad in said window using Vulkan. It also correctly handles swapchain updating when window is resized.

The code purposefully contains almost zero abstractions, almost all code is contained inside a very long main function. This is so that users of the template can decide from grounds-up the kinds of abstractions they want to implement.

## Opinionated parts

- build system: Cmake
- programming language: C++20 with Clang and `-fno-exceptions`
- Vulkan: LunarG's Vulkan SDK
- windowing: GLFW
- maths library: GLM.

## Building and running

```bash
$ cmake -S . -B build # also generates a clangd config
$ make -C build
$ ./build/vulkan_starter # run
```
