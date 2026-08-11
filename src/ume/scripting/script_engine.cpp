#include "script_engine.hpp"
#include "ume/core/logger.hpp"
#include "ume/renderer/renderer.hpp"

#include <wren.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

namespace ume {

namespace {
void writeFn(WrenVM *vm, const char *text) {
    static std::string buffer;
    buffer += text;

    size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos) {
        UME_LOG_INFO(Script, "{}", buffer.substr(0, pos));
        buffer.erase(0, pos + 1);
    }
}

void errorFn(WrenVM *vm, WrenErrorType error_type, const char *module,
             const int line, const char *msg) {
    switch (error_type) {
    case WREN_ERROR_COMPILE:
        UME_LOG_ERROR(Script, "wren compile error [{} line {}] [error] {}",
                      module, line, msg);
        break;
    case WREN_ERROR_STACK_TRACE:
        UME_LOG_ERROR(Script, "ren stack trace error [{} line {}] in {}",
                      module, line, msg);
        break;
    case WREN_ERROR_RUNTIME:
        UME_LOG_ERROR(Script, "wren runtime error [runtime error] {}", msg);
        break;
    }
}

void abortWithError(WrenVM *vm, const char *message) {
    wrenSetSlotString(vm, 0, message);
    wrenAbortFiber(vm, 0);
}

Renderer &rendererFromVM(WrenVM *vm) {
    return *static_cast<Renderer *>(wrenGetUserData(vm));
}

bool readNumberList(WrenVM *vm, int list_slot, int scratch_slot,
                    std::vector<float> &out) {
    const int count = wrenGetListCount(vm, list_slot);
    out.resize(static_cast<size_t>(count));

    for (int i = 0; i < count; i++) {
        wrenGetListElement(vm, list_slot, i, scratch_slot);
        if (wrenGetSlotType(vm, scratch_slot) != WREN_TYPE_NUM) {
            return false;
        }

        out[static_cast<size_t>(i)] =
            static_cast<float>(wrenGetSlotDouble(vm, scratch_slot));
    }

    return true;
}

bool readIndexList(WrenVM *vm, int list_slot, int scratch_slot,
                   std::vector<uint32_t> &out) {
    const int count = wrenGetListCount(vm, list_slot);
    out.resize(static_cast<size_t>(count));

    for (int i = 0; i < count; i++) {
        wrenGetListElement(vm, list_slot, i, scratch_slot);
        if (wrenGetSlotType(vm, scratch_slot) != WREN_TYPE_NUM) {
            return false;
        }

        out[static_cast<size_t>(i)] =
            static_cast<uint32_t>(wrenGetSlotDouble(vm, scratch_slot));
    }

    return true;
}

void scriptCreateMesh(WrenVM *vm) {
    if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST ||
        wrenGetSlotType(vm, 2) != WREN_TYPE_LIST ||
        wrenGetSlotType(vm, 3) != WREN_TYPE_LIST) {
        abortWithError(vm, "createMesh expects (positions, normals, indices) "
                           "as three lists");
        return;
    }

    const int position_count = wrenGetListCount(vm, 1);
    const int normal_count = wrenGetListCount(vm, 2);
    const int index_count = wrenGetListCount(vm, 3);

    if (position_count % 3 != 0) {
        abortWithError(vm, "createMesh: positions length must be a multiple "
                           "of 3");
        return;
    }

    if (normal_count != position_count) {
        abortWithError(vm, "createMesh: normals and positions must have the "
                           "same length");
        return;
    }

    if (index_count % 3 != 0) {
        abortWithError(vm, "createMesh: indices length must be a multiple "
                           "of 3");
        return;
    }

    wrenEnsureSlots(vm, 5);
    constexpr int scratch_slot = 4;

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<uint32_t> indices;

    if (!readNumberList(vm, 1, scratch_slot, positions) ||
        !readNumberList(vm, 2, scratch_slot, normals)) {
        abortWithError(vm, "createMesh: positions and normals must contain "
                           "only numbers");
        return;
    }

    if (!readIndexList(vm, 3, scratch_slot, indices)) {
        abortWithError(vm, "createMesh: indices must contain only numbers");
        return;
    }

    const size_t vertex_count = positions.size() / 3;

    std::vector<Vertex> vertices(vertex_count);
    for (size_t i = 0; i < vertex_count; ++i) {
        vertices[i].position = {positions[i * 3], positions[(i * 3) + 1],
                                positions[(i * 3) + 2], 1.0f};
        vertices[i].normal = {normals[i * 3], normals[(i * 3) + 1],
                              normals[(i * 3) + 2], 0.0f};
    }

    for (uint32_t index : indices) {
        if (index >= vertex_count) {
            abortWithError(vm, "createMesh: index out of range");
            return;
        }
    }

    MeshHandle handle = rendererFromVM(vm).createMesh(
        MeshDescription{.vertices = vertices, .indices = indices});

    if (!handle) {
        abortWithError(vm, "createMesh: renderer failed to create mesh");
        return;
    }

    wrenSetSlotDouble(vm, 0, static_cast<double>(handle.id));
}

void scriptSubmit(WrenVM *vm) {
    if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) {
        abortWithError(vm, "submit expects a mesh handle");
        return;
    }

    const glm::dvec3 world_position{
        wrenGetSlotDouble(vm, 2),
        wrenGetSlotDouble(vm, 3),
        wrenGetSlotDouble(vm, 4),
    };

    MeshHandle handle{static_cast<uint32_t>(wrenGetSlotDouble(vm, 1))};
    rendererFromVM(vm).submit(handle, world_position, glm::mat4(1.0f));
}

void scriptSetCamera(WrenVM *vm) {
    for (int slot = 1; slot <= 7; ++slot) {
        if (wrenGetSlotType(vm, slot) != WREN_TYPE_NUM) {
            abortWithError(vm, "setCamera expects seven numbers");
            return;
        }
    }

    const glm::dvec3 camera_world_position =
        glm::dvec3(wrenGetSlotDouble(vm, 1), wrenGetSlotDouble(vm, 2),
                   wrenGetSlotDouble(vm, 3));

    const glm::dvec3 target_world_position =
        glm::dvec3(wrenGetSlotDouble(vm, 4), wrenGetSlotDouble(vm, 5),
                   wrenGetSlotDouble(vm, 6));

    const auto fov_y_degrees =
        glm::radians(static_cast<float>(wrenGetSlotDouble(vm, 7)));

    const CameraState camera_state =
        lookAtCamera(camera_world_position, target_world_position,
                     glm::dvec3(0.0f, 1.0f, 0.0f), fov_y_degrees, 1.0f);

    rendererFromVM(vm).setCamera(camera_state);
}

WrenForeignMethodFn bindForeignMethodFn(WrenVM *vm, const char *module,
                                        const char *class_name, bool is_static,
                                        const char *signature) {
    if (strcmp(module, "main") != 0 || strcmp(class_name, "Renderer") != 0 ||
        !is_static) {
        return nullptr;
    }

    if (strcmp(signature, "createMesh(_,_,_)") == 0) {
        return &scriptCreateMesh;
    }

    if (strcmp(signature, "submit(_,_,_,_)") == 0) {
        return &scriptSubmit;
    }

    if (strcmp(signature, "setCamera(_,_,_,_,_,_,_)") == 0) {
        return &scriptSetCamera;
    }

    return nullptr;
}
} // namespace

ScriptEngine::ScriptEngine(Renderer &renderer,
                           const std::string &main_script_path) {
    std::ifstream main_script(main_script_path);

    if (!main_script.is_open()) {
        throw std::runtime_error("failed to open main script");
    }

    std::stringstream buffer;
    buffer << main_script.rdbuf();
    std::string main_script_src = buffer.str();
    main_script.close();

    WrenConfiguration config;
    wrenInitConfiguration(&config);
    config.writeFn = &writeFn;
    config.errorFn = &errorFn;
    config.bindForeignMethodFn = &bindForeignMethodFn;
    config.userData = &renderer;

    wren_vm_ = wrenNewVM(&config);

    WrenInterpretResult result =
        wrenInterpret(wren_vm_, "main", main_script_src.c_str());

    switch (result) {
    case WREN_RESULT_COMPILE_ERROR:
        throw std::runtime_error("wren compile error");
    case WREN_RESULT_RUNTIME_ERROR:
        throw std::runtime_error("wren runtime error");
    case WREN_RESULT_SUCCESS:
        break;
    }

    main_init_ = wrenMakeCallHandle(wren_vm_, "init()");
    main_update_ = wrenMakeCallHandle(wren_vm_, "update(_)");
    wrenEnsureSlots(wren_vm_, 1);
    wrenGetVariable(wren_vm_, "main", "Sandbox", 0);
    main_class_ = wrenGetSlotHandle(wren_vm_, 0);
}

ScriptEngine::~ScriptEngine() { wrenFreeVM(wren_vm_); }

void ScriptEngine::init() {
    wrenEnsureSlots(wren_vm_, 1);
    wrenSetSlotHandle(wren_vm_, 0, main_class_);
    WrenInterpretResult result = wrenCall(wren_vm_, main_init_);

    switch (result) {
    case WREN_RESULT_COMPILE_ERROR:
    case WREN_RESULT_RUNTIME_ERROR:
        main_script_failed_ = true;
    case WREN_RESULT_SUCCESS:
        break;
    }
}

void ScriptEngine::update(float delta) {
    if (main_script_failed_) {
        return;
    }

    wrenEnsureSlots(wren_vm_, 2);
    wrenSetSlotHandle(wren_vm_, 0, main_class_);
    wrenSetSlotDouble(wren_vm_, 1, delta);

    WrenInterpretResult result = wrenCall(wren_vm_, main_update_);

    switch (result) {
    case WREN_RESULT_COMPILE_ERROR:
    case WREN_RESULT_RUNTIME_ERROR:
        main_script_failed_ = true;
    case WREN_RESULT_SUCCESS:
        break;
    }
}
} // namespace ume