#ifndef UME_PLUGIN_API_H
#define UME_PLUGIN_API_H

/* NOTE: this is a C ABI. No function pointer in this header may allow an
 * exception to escape, in either direction. The host guarantees this for every
 * function in UmePluginApi and UmeParams; the plugin must guarantee it for its
 * register function and for every callback it installs in UmeObjectType and
 * UmePluginDescription. C++ plugins should mark these noexcept and catch
 * internally.
 *
 * Threading
 *
 * The plugin API is not thread safe and has no internal synchronisation. Every
 * function in UmePluginApi must be called from the same thread the host used
 * to call into the plugin. The host calls every plugin callback from a single
 * thread. A plugin that uses worker threads must marshal all API calls back to
 * the thread it was called on.
 *
 * Reentrancy
 *
 * No function in UmePluginApi invokes a plugin callback, so plugin callbacks
 * are never entered reentrantly by the host and a plugin need not guard
 * against it.
 *
 * Lifecycle
 *
 * A plugin is driven through the following sequence:
 *
 *   register  -- fill in the description; no API call is permitted here
 *   init      -- register object types; other API functions become available
 *   create / update / destroy   -- once per object, any number of objects
 *   shutdown  -- last call the plugin receives
 *
 * registerObjectType is valid only during init and is rejected at any other
 * time. All other API functions are valid from init until shutdown returns.
 *
 * Every live object's destroy callback is invoked before shutdown, so
 * user_data and the plugin's state are still valid inside destroy. This holds
 * during host teardown as well: a plugin cannot assume shutdown is the first
 * notice it gets that the host is going away.
 *
 * shutdown must only release resources. Objects have already been destroyed
 * and meshes already reclaimed by the time it runs; creating anything new
 * there will leak.
 *
 * Pointer lifetimes
 *
 * Unless stated otherwise, every pointer the host passes to a plugin is valid
 * only for the duration of the call it appears in, and every pointer the
 * plugin passes to the host is read before that call returns and never
 * retained. A plugin that needs any of this data later must copy it.
 *
 * Resource ownership
 *
 * Meshes are owned by the plugin that created them and must be destroyed with
 * destroyMesh before shutdown returns. A plugin may only destroy or submit
 * meshes it created. The host reclaims meshes still outstanding at unload and
 * logs a warning; that is leak detection, not a substitute for cleanup.
 */

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

/* NOTE: struct_size and abi_version are the first two fields of UmePluginApi
 * and UmePluginDescription and must remain at those offsets permanently. They
 * are the only fields readable across an ABI version mismatch, and are what
 * makes an ABI mismatch detectable.
 *
 * Host and plugin ABI versions must match exactly. There is no compatibility
 * range.
 *
 * ABI version is bumped for any change that is not backward compatible. Such as
 * removing, reordering, or retyping a struct field, changing a function
 * pointer signature, changing the semantics of an existing function or changing
 * the value or meaning of an enum constant.
 *
 * ABI version is not bumped for purely additive changes. such as appending a
 * field to the end of a struct that carries struct_size. Those are detected by
 * comparing struct_size, and both sides must tolerate a peer whose struct is
 * smaller than their own.
 *
 * NOTE: ABI version is not being incremented right now since there are no
 * official engine releases yet.
 */

#define UME_PLUGIN_ABI_VERSION 0

#ifdef UME_PLUGIN_STATIC
#define UME_PLUGIN_EXPORT
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

typedef struct UmeMeshDescription {
    uint32_t struct_size;
    uint32_t vertex_count;
    const float *positions;
    const float *normals;
    uint32_t index_count;
    const uint32_t *indices;

} UmeMeshDescription;

// Valid only for the duration of the update call.
typedef struct UmeFrameContext {
    uint32_t struct_size;
    uint64_t frame_index;
    double camera_position[3];
    float camera_orientation[9];
    float fov_y, z_near, aspect, delta_time;
} UmeFrameContext;

// Valid only for the duration of the create call.
typedef struct UmeParams {
    uint32_t struct_size;
    const void *impl;
    double (*number)(const void *impl, const char *key, double fallback);
} UmeParams;

/* The host copies everything it needs from this struct, including name, before
 * registerObjectType returns. The struct and the string need not outlive the
 * call. user_data is retained and passed back to create, destroy, and update.
 */
typedef struct UmeObjectType {
    uint32_t struct_size;
    const char *name;
    void *user_data;
    void *(*create)(void *user_data, const UmeParams *params);
    void (*destroy)(void *user_data, void *object);
    void (*update)(void *user_data, void *object, const UmeFrameContext *frame);
} UmeObjectType;

/* the API pointer passed to a plugin's register function is valid only for the
 * duration of that call. Any plugin that needs the API later must copy the
 * struct.
 *
 * If registration is successful, the copied contents remain valid until the
 * plugin's shutdown function returns. If registration fails, the copied
 * contents are invalid as soon as the register function returns and must not
 * be used.
 */
typedef struct UmePluginApi {
    uint32_t struct_size;
    uint32_t abi_version;
    void *context;
    UME_PLUGIN_BOOL (*registerObjectType)(void *context,
                                          const UmeObjectType *type);
    UmeMeshHandle (*createMesh)(void *context,
                                const UmeMeshDescription *description);
    void (*destroyMesh)(void *context, UmeMeshHandle handle);
    void (*submit)(void *context, UmeMeshHandle mesh,
                   const double world_position[3],
                   const float local_transform[16]);
    void (*log)(void *context, UmeLogLevel log_level, const char *message);
} UmePluginApi;

/* The plugin description is owned by the host and is valid only during the
 * register call. The host copies name, so the plugin's buffer need not outlive
 * the call. init, state, and shutdown are retained by the host. init is called
 * after the plugin successfully registers and shutdown is called with when the
 * plugin is unloaded.
 */
typedef struct UmePluginDescription {
    uint32_t struct_size;
    uint32_t abi_version;
    const char *name;
    void *state;
    UME_PLUGIN_BOOL (*init)(void *state);
    void (*shutdown)(void *state);
} UmePluginDescription;

/* Returns UME_TRUE on success.
 * If UME_FALSE is returned, the plugin must have freed any resources that
 * it allocated.
 *
 * The plugin may not call any API function from its register function; the API
 * becomes usable once init is called.
 */
typedef UME_PLUGIN_BOOL (*UmePluginRegisterFunction)(
    const UmePluginApi *api, UmePluginDescription *description);

#ifdef __cplusplus
}
#endif

#endif // UME_PLUGIN_API_H