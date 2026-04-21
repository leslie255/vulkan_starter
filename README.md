# Vulkan Starter Template

**Starter template for C++ Vulkan gamedev projects**

## What it contains

This starter template contains the bare-minimum code that creates a GLFW window, and draws a quad in said window using Vulkan. It also correctly handles swapchain updating when window is resized.

The code purposefully contains almost zero abstractions, almost all code is contained inside a very long main function. This is so that users of the template can decide from grounds-up the kinds of abstractions they want to implement.

## Opinionated parts

- build system: Cmake
- windowing: GLFW
- programming language: C++20 with Clang/GCC and `-fno-exceptions`
- meta loader for Vulkan: Volk
- Vulkan: LunarG's Vulkan SDK

## Building and running

At project's root directory:

```bash
git submodule update --init --recursive
cmake -S . -B build
make -C build
```

Run:

```bash
./build/vulkan_starter
```

Result:

<img width="50%" alt="Screenshot 2026-04-21 at 20 14 09" src="https://github.com/user-attachments/assets/617fb4f7-8691-4353-ab5b-cd66215cd551" />
