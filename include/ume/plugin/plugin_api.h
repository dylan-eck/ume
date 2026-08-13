#ifndef UME_PLUGIN_API_H
#define UME_PLUGIN_API_H

#include <stdint.h>

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#endif

#define UME_PLUGIN_ABI_VERSION 0

#ifdef UME_PLUGIN_STATIC
#define UME_PLUGIN_EXPORT
#elif defined(_WIN32)
#define UME_PLUGIN_EXPORT __declspec(dllexport)
#else
#define UME_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#ifdef UME_PLUGIN_STATIC
#define UME_PLUGIN_ENTRY(name) name##PluginRegister
#else
#define UME_PLUGIN_ENTRY(name) umePluginRegister
#endif

typedef uint8_t UME_PLUGIN_BOOL;
#define UME_TRUE ((UME_PLUGIN_BOOL)1)
#define UME_FALSE ((UME_PLUGIN_BOOL)0)

typedef uint32_t UmeMeshHandle;
#define UME_MESH_HANDLE_INVALID ((UmeMeshHandle)0)

typedef enum UmeLogLevel {
    UME_LOG_LEVEL_INFO = 0,
    UME_LOG_LEVEL_WARN = 1,
    UME_LOG_LEVEL_ERROR = 2,
    UME_LOG_LEVEL_FORCE_U32 = 0x7fffffff
} UmeLogLevel;

typedef struct UmeVertex {
    float position[4];
    float normal[4];
} UmeVertex;

typedef struct UmeMeshDescription {
    uint32_t struct_size;
    const UmeVertex *vertices;
    uint32_t vertex_count;
    const uint32_t *indices;
    uint32_t index_count;
} UmeMeshDescription;

typedef struct UmeFrameContext {
    uint32_t struct_size;
    uint64_t frame_index;
    double camera_position[3];
    float camera_orientation[9];
    float fov_y, z_near, aspect, delta_time;
} UmeFrameContext;

typedef struct UmeParams {
    uint32_t struct_size;
    void *impl;
    double (*number)(void *impl, const char *key, double fallback);
} UmeParams;

typedef struct UmeObjectType {
    uint32_t struct_size;
    const char *name;
    void *user_data;
    void *(*create)(void *user_data, const UmeParams *params);
    void (*destroy)(void *user_data, void *object);
    void (*update)(void *user_data, void *object, const UmeFrameContext *frame);
} UmeObjectType;

typedef struct UmePluginApi {
    uint32_t struct_size;
    uint32_t abi_version;
    void *context;
    void (*registerObjectType)(void *context, const UmeObjectType *type);
    UmeMeshHandle (*createMesh)(void *context,
                                const UmeMeshDescription *description);
    void (*destroyMesh)(void *context, UmeMeshHandle handle);
    void (*submit)(void *context, UmeMeshHandle mesh,
                   const double world_position[3]);
    void (*log)(void *context, UmeLogLevel log_level, const char *message);
} UmePluginApi;

typedef struct UmePluginDescription {
    uint32_t struct_size;
    uint32_t abi_version;
    const char *name;
    void *state;
    void (*shutdown)(void *state);
} UmePluginDescription;

typedef UME_PLUGIN_BOOL (*UmePluginRegisterFunction)(
    const UmePluginApi *api, UmePluginDescription *description);

#ifdef __cplusplus
}
#endif

#endif // UME_PLUGIN_API_H