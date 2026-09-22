# Ume

## About

Ume is a work-in-progress graphics engine written in C++.

## Features

- **Metal renderer** built on a backend-agnostic interface, with an offscreen
  scene pass, a chain of post-processing passes, and compute dispatch. A
  Vulkan backend is in progress.
- **Runtime shader compilation** with [Slang](https://shader-slang.com/).
  Shaders are compiled to MSL or SPIR-V at load time, with reflection used to
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
  declare post-processing effects in a `plugin.toml` manifest. The host
  validates every handle and ownership claim that crosses the boundary.
- **Procedural planets**, provided by the `proc_planet` plugin: a noise
  displaced cube-sphere with ray-marched ocean, Rayleigh-scattered atmosphere,
  and sun disc post effects, plus a quadtree level-of-detail terrain plugin.
- **SDL3 platform layer** for windowing, high-DPI and resize handling, and
  keyboard input.
- **TOML project files** describing the application name, main script, and
  window.

## Building

## License

Ume is licensed under the MIT License; see [`LICENSE`](LICENSE).

Ume statically links a number of third-party libraries. Their licenses and
required notices are collected in
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md). Any binary built with
Ume, including applications built on top of it, must be distributed with a copy of
that file.
