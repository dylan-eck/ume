#ifndef UME_PLUGIN_API_H
#define UME_PLUGIN_API_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
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

typedef uint32_t UmeMeshHandle;
#define UME_MESH_HANDLE_INVALID ((UmeMeshHandle)0)

typedef enum UmeLogLevel {
    UME_LOG_INFO = 0,
    UME_LOG_WARN = 1,
    UME_LOG_ERROR = 2,
    UME_LOG_LEVEL_FORCE_U32 =
        0x7fffffff // ensure that underlying type is uint32_t
} UmeLogLevel;

typedef struct UmeVertex {
    float position[4];
    float normal[4];
} UmeVertex;

typedef struct UmeMeshDescription {
    const UmeVertex *vertices;
    uint32_t vertex_count;
    const uint32_t *indices;
    uint32_t index_count;
} UmeMeshDescription;

typedef struct UmeFrameContext {
    uint64_t frame_index;
    double camera_position[3];
    float camera_orientation[9];
    float fov_y, z_near, aspect, delta_time;
} UmeFrameContext;

typedef struct UmeParams {
    void *impl;
    double (*number)(void *impl, const char *key, double fallback);
} UmeParams;

typedef struct UmeObjectType {
    const char *name;
    void *user_data;
    void *(*create)(void *user_data, const UmeParams *params);
    void (*destroy)(void *user_data, void *object);
    void (*update)(void *user_data, void *object, const UmeFrameContext *frame);
} UmeObjectType;

typedef struct UmePluginApi {
    uint32_t abi_version;
    uint32_t struct_size;
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
    uint32_t abi_version;
    const char *name;
    void *state;
    void (*shutdown)(void *state);
} UmePluginDescription;

typedef bool (*UmePluginRegisterFunction)(const UmePluginApi *api,
                                          UmePluginDescription *description);

#ifdef __cplusplus
}
#endif

#endif // UME_PLUGIN_API_H