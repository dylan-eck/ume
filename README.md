# Ume

## About

Ume is a work-in-progress graphics engine written in C++.

## Features

- **Metal renderer** Metal renderer behind a backend-agnostic
  interface: explicit render passes and attachments, pipelines with baked
  raster and depth state, reflection-driven resource binding, a ping-pong
  post-processing chain, and compute dispatch. A Vulkan backend
  is in progress on the `vulkan-backend` branch.

- **Runtime shader compilation** with [Slang](https://shader-slang.com/).
  Shaders are compiled to MSL at load time, with reflection used to
  bind resources automatically. Post-processing shaders can be hot reloaded
  while the engine is running.

- **Planetary-scale rendering.** World positions are double precision and
  rebased relative to the camera each frame, and the depth buffer uses a
  reverse-Z, infinite-far projection.

- **Wren scripting.** Applications are written in
  [Wren](https://wren.io/) against an embedded `ume` module that exposes the
  renderer, camera, input, and window.

- **Native plugins** loaded through a versioned C ABI. Plugins register object
  types that scripts can instantiate, submit meshes to the renderer, and
  declare post-processing effects in a `plugin.toml` manifest.

- **Procedural planets**, provided by the `proc_planet` plugin: a noise
  displaced cube-sphere with ray-marched ocean, Rayleigh-scattered atmosphere,
  and sun disc post effects. A quadtree level-of-detail terrain plugin (`terrain`) is in
  progress.

- **SDL3 platform layer** for windowing, high-DPI and resize handling, and
  keyboard input.

- **TOML project files** describing the application name, main script, and
  window.

## Building

- **Requirements:**
  - macOS with Xcode or the Command Line Tools;
  - CMake 3.25+ and Ninja;
  - Slang, with `libslang` and `slang.h` either in a default search path or
    under `$VULKAN_SDK` (the LunarG Vulkan SDK ships them)
  - optionally ccache or sccache.

To build the project, first clone the repo and all submodules by running `git clone --recursive https://github.com/dylan-eck/ume.git`.
Then run `./build.sh -r debug -- examples/planet` to build and run the project.

## License

Ume is licensed under the MIT License; see [`LICENSE`](LICENSE).

Ume statically links a number of third-party libraries. Their licenses and
required notices are collected in
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md). Any binary built with
Ume, including applications built on top of it, must be distributed with a copy of
that file.
