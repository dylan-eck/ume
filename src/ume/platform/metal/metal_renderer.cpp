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
    layer_->setFramebufferOnly(false);
    layer_->setDrawableSize(CGSizeMake(pixel_width, pixel_height));

    auto make_target = [&](MTL::PixelFormat format, const char *what) {
        MTL::TextureDescriptor *desc =
            MTL::TextureDescriptor::texture2DDescriptor(format, pixel_width,
                                                        pixel_height, false);
        desc->setStorageMode(MTL::StorageModePrivate);
        desc->setUsage(MTL::TextureUsageRenderTarget |
                       MTL::TextureUsageShaderRead);

        auto tex = NS::TransferPtr(device_->newTexture(desc));
        if (!tex) {
            throw Error(logger::Category::Renderer,
                        "failed to create {} texture", what);
        }
        return tex;
    };

    color_targets_[0] = make_target(MTL::PixelFormatBGRA8Unorm, "scene color");
    color_targets_[1] = make_target(MTL::PixelFormatBGRA8Unorm, "post color");
    depth_texture_ = make_target(MTL::PixelFormatDepth32Float, "depth");

    auto sampler_desc =
        NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
    sampler_desc->setMinFilter(MTL::SamplerMinMagFilterLinear);
    sampler_desc->setMagFilter(MTL::SamplerMinMagFilterLinear);
    sampler_desc->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
    sampler_desc->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
    linear_sampler_ =
        NS::TransferPtr(device_->newSamplerState(sampler_desc.get()));

    command_queue_ = NS::TransferPtr(device_->newCommandQueue());
    if (!command_queue_) {
        throw Error(logger::Category::Renderer,
                    "failed to create metal command queue");
    }

    auto shader =
        b::embed<"generated/src/ume/renderer/shaders/default.slang.metallib">();
    auto library = libraryFromMetallib(
        std::as_bytes(std::span(shader.data(), shader.size())));
    if (!library) {
        throw Error(logger::Category::Renderer,
                    "failed to load default shader library");
    }

    pipeline_state_ =
        buildPipeline(library.get(), "vertMain", "fragMain", true);
    if (!pipeline_state_) {
        throw Error(logger::Category::Renderer,
                    "failed to create default pipeline state");
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
    color_attachment->setTexture(color_targets_[0].get());
    color_attachment->setLoadAction(MTL::LoadActionClear);
    color_attachment->setStoreAction(MTL::StoreActionStore);
    color_attachment->setClearColor(MTL::ClearColor(0, 0, 0, 1));

    auto *depth_attachment = pass_descriptor->depthAttachment();
    depth_attachment->setTexture(depth_texture_.get());
    depth_attachment->setLoadAction(MTL::LoadActionClear);
    depth_attachment->setStoreAction(MTL::StoreActionStore);
    depth_attachment->setClearDepth(0.0);

    encoder_ = command_buffer_->renderCommandEncoder(pass_descriptor.get());

    encoder_->setRenderPipelineState(pipeline_state_.get());
    encoder_->setFrontFacingWinding(MTL::WindingCounterClockwise);
    encoder_->setCullMode(MTL::CullModeBack);
    // encoder_->setTriangleFillMode(MTL::TriangleFillModeLines);
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

void MetalRenderer::postProcess(const PostProcessCommand &cmd) {}

void MetalRenderer::endFrame() {
    if (drawable_ == nullptr) {
        frame_pool_->release();
        frame_pool_ = nullptr;
        return;
    }

    encoder_->endEncoding();

    MTL::BlitCommandEncoder *blit = command_buffer_->blitCommandEncoder();
    blit->copyFromTexture(color_targets_[0].get(), drawable_->texture());
    blit->endEncoding();

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

PipelineHandle MetalRenderer::createPipeline(const PipelineDescription &desc) {
    auto library = libraryFromSource(desc.shader);
    if (!library) {
        return {};
    }

    auto state = buildPipeline(library.get(), desc.vertex_entry,
                               desc.fragment_entry, false);
    if (!state) {
        return {};
    }

    return pipelines_.insert(MetalPipeline{.state = std::move(state)});
}

void MetalRenderer::destroyPipeline(PipelineHandle handle) {
    if (!pipelines_.remove(handle)) {
        UME_LOG_WARN(Renderer, "attempted to destroy stale pipeline handle: {}",
                     handle.id);
    }
}

NS::SharedPtr<MTL::Library>
MetalRenderer::libraryFromMetallib(std::span<const std::byte> bytes) {
    dispatch_data_t data = dispatch_data_create(
        bytes.data(), bytes.size(),
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        ^{
        });

    NS::Error *error = nullptr;
    auto library = NS::TransferPtr(device_->newLibrary(data, &error));
    dispatch_release(data);
    if (!library) {
        UME_LOG_ERROR(Renderer, "failed to load metallib: {}",
                      errorString(error));
    }
    return library;
}

NS::SharedPtr<MTL::Library>
MetalRenderer::libraryFromSource(std::span<const std::byte> bytes) {
    auto source = NS::TransferPtr(NS::String::alloc()->init(
        const_cast<void *>(static_cast<const void *>(bytes.data())),
        bytes.size(), NS::UTF8StringEncoding, false));

    auto options = NS::TransferPtr(MTL::CompileOptions::alloc()->init());

    NS::Error *error = nullptr;
    auto library = NS::TransferPtr(
        device_->newLibrary(source.get(), options.get(), &error));
    if (!library) {
        UME_LOG_ERROR(Renderer, "failed to compile msl shader: {}",
                      errorString(error));
    }
    return library;
}

NS::SharedPtr<MTL::RenderPipelineState>
MetalRenderer::buildPipeline(MTL::Library *library, const char *vert,
                             const char *frag, bool with_depth) {
    auto vfn = NS::TransferPtr(
        library->newFunction(NS::String::string(vert, NS::UTF8StringEncoding)));
    auto ffn = NS::TransferPtr(
        library->newFunction(NS::String::string(frag, NS::UTF8StringEncoding)));
    if (!vfn || !ffn) {
        UME_LOG_ERROR(Renderer, "entrypoints '{}'/'{}' not found", vert, frag);
        return {};
    }

    auto desc = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
    desc->setVertexFunction(vfn.get());
    desc->setFragmentFunction(ffn.get());
    desc->colorAttachments()->object(0)->setPixelFormat(
        MTL::PixelFormatBGRA8Unorm);
    if (with_depth) {
        desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    }

    NS::Error *error = nullptr;
    auto state =
        NS::TransferPtr(device_->newRenderPipelineState(desc.get(), &error));
    if (!state) {
        UME_LOG_ERROR(Renderer, "failed to create pipeline state: {}",
                      errorString(error));
    }
    return state;
}

std::unique_ptr<RendererBackend> createRendererBackend(const Window &window) {
    return std::make_unique<MetalRenderer>(window.createMetalSurface(),
                                           window.getPixelWidth(),
                                           window.getPixelHeight());
}
} // namespace ume