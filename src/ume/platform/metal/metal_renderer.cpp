#include "metal_renderer.hpp"
#include "../../core/logger.hpp"

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include "battery/embed.hpp"

namespace ume {

MetalRenderer::MetalRenderer(void *native_window_handle) {
    auto *sdl_window = static_cast<SDL_Window *>(native_window_handle);

    metal_view_ = SDL_Metal_CreateView(sdl_window);

    if (metal_view_ == nullptr) {
        throw std::runtime_error("failed to create metal view");
    }

    layer_ = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metal_view_));

    device_ = MTL::CreateSystemDefaultDevice();

    layer_->setDevice(device_);
    layer_->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    layer_->setFramebufferOnly(true);

    command_queue_ = device_->newCommandQueue();

    int pixel_width;
    int pixel_height;
    SDL_GetWindowSizeInPixels(sdl_window, &pixel_width, &pixel_height);
    layer_->setDrawableSize(CGSizeMake(pixel_width, pixel_height));

    auto shader = b::embed<
        "generated/src/ume/renderer/shaders/triangle.slang.metallib">();

    dispatch_data_t data = dispatch_data_create(
        shader.data(), shader.size(),
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        ^{
        });

    NS::Error *error = nullptr;
    MTL::Library *library = device_->newLibrary(data, &error);
    dispatch_release(data);

    if (library == nullptr) {
        throw std::runtime_error(
            std::string("failed to create metal library: ") +
            error->localizedDescription()->utf8String());
    }

    MTL::Function *vertex_func = library->newFunction(
        NS::String::string("vertMain", NS::UTF8StringEncoding));

    MTL::Function *fragment_func = library->newFunction(
        NS::String::string("fragMain", NS::UTF8StringEncoding));

    if (vertex_func == nullptr || fragment_func == nullptr) {
        library->release();
        throw std::runtime_error(
            "vertMain/fragMain entrypoints not found in shader library");
    }

    auto *descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    descriptor->setVertexFunction(vertex_func);
    descriptor->setFragmentFunction(fragment_func);
    descriptor->colorAttachments()->object(0)->setPixelFormat(
        MTL::PixelFormatBGRA8Unorm);

    error = nullptr;
    pipeline_state_ = device_->newRenderPipelineState(descriptor, &error);

    vertex_func->release();
    fragment_func->release();
    descriptor->release();
    library->release();

    if (pipeline_state_ == nullptr) {
        throw std::runtime_error(
            std::string("failed to create render pipeline state: ") +
            error->localizedDescription()->utf8String());
    }

    UME_LOG_INFO(Renderer, "initialized metal renderer backend");
}

MetalRenderer::~MetalRenderer() {
    pipeline_state_->release();
    command_queue_->release();
    device_->release();
    SDL_Metal_DestroyView(metal_view_);
}

void MetalRenderer::beginFrame() {
    frame_pool_ = NS::AutoreleasePool::alloc()->init();

    drawable_ = layer_->nextDrawable();
    if (drawable_ == nullptr) {
        return;
    }

    command_buffer_ = command_queue_->commandBuffer();

    auto *pass_descriptor = MTL::RenderPassDescriptor::alloc()->init();
    auto *color_attachment = pass_descriptor->colorAttachments()->object(0);
    color_attachment->setTexture(drawable_->texture());
    color_attachment->setLoadAction(MTL::LoadActionClear);
    color_attachment->setStoreAction(MTL::StoreActionStore);
    color_attachment->setClearColor(MTL::ClearColor(0, 0, 0, 1));

    encoder_ = command_buffer_->renderCommandEncoder(pass_descriptor);
    pass_descriptor->release();

    encoder_->setRenderPipelineState(pipeline_state_);
    encoder_->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0),
                             NS::UInteger(3));
}

void MetalRenderer::endFrame() {
    if (drawable_ == nullptr) {
        frame_pool_->release();
        frame_pool_ = nullptr;
        return;
    }

    encoder_->endEncoding();
    command_buffer_->presentDrawable(drawable_);
    command_buffer_->commit();

    encoder_ = nullptr;
    command_buffer_ = nullptr;
    drawable_ = nullptr;

    frame_pool_->release();
    frame_pool_ = nullptr;
}

std::unique_ptr<RendererBackend>
createRendererBackend(void *native_window_handle) {
    return std::make_unique<MetalRenderer>(native_window_handle);
}
} // namespace ume