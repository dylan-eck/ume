#pragma once

#include "ume/renderer/renderer.hpp"

#include <sol/sol.hpp>

namespace ume {
void bindRenderer(sol::state &lua_state, Renderer &renderer);
}