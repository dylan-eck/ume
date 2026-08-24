#include "ume/platform/metal/metal_renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/core/error.hpp"

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <battery/embed.hpp>

namespace ume {

namespace {
const char *errorString(NS::Error *error) {
    if (error == nullptr || error->localizedDescription() == nullptr) {
        return "unknown error";
    }
    return error->localizedDescription()->utf8String();
}
} // namespace

MetalRenderer::MetalRenderer(MetalSurface surface, uint32_t pixel_width,
                             uint32_t pixel_height)
    : surface_(std::move(surface)), layer_(surface_.getLayer()) {

    auto pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

    device_ = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    if (!device_) {
        throw Error(logger::Category::Renderer,
                    "no metal rendering device found");
    }

    layer_->setDevice(device_.get());
    layer_->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    layer_->setFramebufferOnly(true);
    layer_->setDrawableSize(CGSizeMake(pixel_width, pixel_height));

    command_queue_ = NS::TransferPtr(device_->newCommandQueue());
    if (!command_queue_) {
        throw Error(logger::Category::Renderer,
                    "failed to create metal command queue");
    }

    auto shader = b::embed<
        "generated/src/ume/renderer/shaders/triangle.slang.metallib">();

    dispatch_data_t data = dispatch_data_create(
        shader.data(), shader.size(),
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        ^{
        });

    NS::Error *error = nullptr;
    auto library = NS::TransferPtr(device_->newLibrary(data, &error));
    dispatch_release(data);

    if (!library) {
        throw Error(logger::Category::Renderer,
                    "failed to create metal library: {}", errorString(error));
    }

    auto vertex_func = NS::TransferPtr(library->newFunction(
        NS::String::string("vertMain", NS::UTF8StringEncoding)));

    auto fragment_func = NS::TransferPtr(library->newFunction(
        NS::String::string("fragMain", NS::UTF8StringEncoding)));

    if (!vertex_func || !fragment_func) {
        throw Error(
            logger::Category::Renderer,
            "vertMain/fragMain entrypoints not found in shader library");
    }

    auto descriptor =
        NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
    descriptor->setVertexFunction(vertex_func.get());
    descriptor->setFragmentFunction(fragment_func.get());
    descriptor->colorAttachments()->object(0)->setPixelFormat(
        MTL::PixelFormatBGRA8Unorm);
    descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    error = nullptr;
    pipeline_state_ = NS::TransferPtr(
        device_->newRenderPipelineState(descriptor.get(), &error));

    if (!pipeline_state_) {
        throw Error(logger::Category::Renderer,
                    "failed to create render pipeline state: {}",
                    errorString(error));
    }

    MTL::TextureDescriptor *tex_desc =
        MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatDepth32Float, pixel_width, pixel_height, false);

    tex_desc->setStorageMode(MTL::StorageModePrivate);
    tex_desc->setUsage(MTL::TextureUsageRenderTarget);

    depth_texture_ = NS::TransferPtr(device_->newTexture(tex_desc));
    if (!depth_texture_) {
        throw Error(logger::Category::Renderer,
                    "failed to create depth texture");
    }

    auto depth_desc =
        NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
    depth_desc->setDepthCompareFunction(MTL::CompareFunctionGreater);
    depth_desc->setDepthWriteEnabled(true);

    depth_state_ =
        NS::TransferPtr(device_->newDepthStencilState(depth_desc.get()));
    if (!depth_state_) {
        throw Error(logger::Category::Renderer,
                    "failed to create depth stencil state");
    }

    UME_LOG_INFO(Renderer, "initialized metal renderer backend");
}

void MetalRenderer::beginFrame() {
    frame_pool_ = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

    drawable_ = layer_->nextDrawable();
    if (drawable_ == nullptr) {
        encoder_ = nullptr;
        command_buffer_ = nullptr;
        return;
    }

    command_buffer_ = command_queue_->commandBuffer();

    auto pass_descriptor =
        NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
    auto *color_attachment = pass_descriptor->colorAttachments()->object(0);
    color_attachment->setTexture(drawable_->texture());
    color_attachment->setLoadAction(MTL::LoadActionClear);
    color_attachment->setStoreAction(MTL::StoreActionStore);
    color_attachment->setClearColor(MTL::ClearColor(0.1, 0.1, 0.1, 1));

    auto *depth_attachment = pass_descriptor->depthAttachment();
    depth_attachment->setTexture(depth_texture_.get());
    depth_attachment->setLoadAction(MTL::LoadActionClear);
    depth_attachment->setStoreAction(MTL::StoreActionDontCare);
    depth_attachment->setClearDepth(0.0);

    encoder_ = command_buffer_->renderCommandEncoder(pass_descriptor.get());

    encoder_->setRenderPipelineState(pipeline_state_.get());
    encoder_->setFrontFacingWinding(MTL::WindingCounterClockwise);
    // encoder_->setCullMode(MTL::CullModeBack);
    encoder_->setTriangleFillMode(MTL::TriangleFillModeLines);
    encoder_->setDepthStencilState(depth_state_.get());
}

void MetalRenderer::draw(const DrawCommand &cmd) {
    if (encoder_ == nullptr) {
        return;
    }

    if (!cmd.push_constants.empty()) {
        encoder_->setVertexBytes(cmd.push_constants.data(),
                                 cmd.push_constants.size(), 0);
    }

    MetalBuffer *vertex_buffer = buffers_.get(cmd.vertex_buffer);
    if (vertex_buffer == nullptr) {
        UME_LOG_WARN(Renderer,
                     "attempted to draw using invalid vertex buffer {}",
                     cmd.vertex_buffer.id);
        return;
    }

    MetalBuffer *index_buffer = buffers_.get(cmd.index_buffer);
    if (index_buffer == nullptr) {
        UME_LOG_WARN(Renderer,
                     "attempted to draw using invalid index buffer {}",
                     cmd.index_buffer.id);
        return;
    }

    MTL::IndexType index_type = cmd.index_type == IndexType::UInt16
                                    ? MTL::IndexTypeUInt16
                                    : MTL::IndexTypeUInt32;

    encoder_->setVertexBuffer(vertex_buffer->buffer.get(), 0, 1);
    encoder_->drawIndexedPrimitives(
        MTL::PrimitiveTypeTriangle, NS::UInteger(cmd.index_count), index_type,
        index_buffer->buffer.get(), NS::UInteger(0));
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

    frame_pool_.reset();
}

BufferHandle MetalRenderer::createBuffer(const BufferDescription &desc) {
    if (desc.size == 0) {
        UME_LOG_WARN(Renderer, "attempted to create buffer with size zero");
        return {};
    }

    auto buffer = NS::TransferPtr(
        desc.initial_data != nullptr
            ? device_->newBuffer(desc.initial_data, desc.size,
                                 MTL::ResourceStorageModeShared)
            : device_->newBuffer(desc.size, MTL::ResourceStorageModeShared));

    if (!buffer) {
        UME_LOG_WARN(Renderer, "failed to allocate {} byte buffer", desc.size);
        return {};
    }

    return buffers_.insert(
        MetalBuffer{.buffer = std::move(buffer), .size = desc.size});
}

void MetalRenderer::destroyBuffer(BufferHandle handle) {
    std::optional<MetalBuffer> entry = buffers_.remove(handle);

    if (!entry) {
        UME_LOG_WARN(Renderer, "attempted to destroy stale handle: {}",
                     handle.id);
        return;
    }
}

std::unique_ptr<RendererBackend> createRendererBackend(const Window &window) {
    return std::make_unique<MetalRenderer>(window.createMetalSurface(),
                                           window.getPixelWidth(),
                                           window.getPixelHeight());
}
} // namespace ume