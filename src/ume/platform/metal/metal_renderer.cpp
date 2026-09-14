#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "ume/platform/metal/metal_renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/core/error.hpp"

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

    width_ = static_cast<uint32_t>(layer_->drawableSize().width);
    height_ = static_cast<uint32_t>(layer_->drawableSize().height);

    createRenderTargets(width_, height_);

    auto sampler_desc =
        NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
    sampler_desc->setMinFilter(MTL::SamplerMinMagFilterLinear);
    sampler_desc->setMagFilter(MTL::SamplerMinMagFilterLinear);
    sampler_desc->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
    sampler_desc->setTAddressMode(MTL::SamplerAddressModeClampToEdge);

    linear_sampler_ = samplers_.insert({
        .state = NS::TransferPtr(device_->newSamplerState(sampler_desc.get())),
    });

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
        buildGraphicsPipeline(library.get(), "vertMain", "fragMain", true);
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

void MetalRenderer::resize(uint32_t width, uint32_t height) {
    if (width == width_ && height == height_) return;

    width_ = static_cast<uint32_t>(layer_->drawableSize().width);
    height_ = static_cast<uint32_t>(layer_->drawableSize().height);

    createRenderTargets(width_, height_);
}

void MetalRenderer::beginFrame() {
    frame_pool_ = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
    command_buffer_ = command_queue_->commandBuffer();
}

void MetalRenderer::beginScenePass() {
    auto pass_descriptor =
        NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
    auto *color_attachment = pass_descriptor->colorAttachments()->object(0);
    color_attachment->setTexture(getTexture(color_targets_[0]));
    color_attachment->setLoadAction(MTL::LoadActionClear);
    color_attachment->setStoreAction(MTL::StoreActionStore);
    color_attachment->setClearColor(MTL::ClearColor(0, 0, 0, 1));

    auto *depth_attachment = pass_descriptor->depthAttachment();
    depth_attachment->setTexture(getTexture(depth_texture_));
    depth_attachment->setLoadAction(MTL::LoadActionClear);
    depth_attachment->setStoreAction(MTL::StoreActionStore);
    depth_attachment->setClearDepth(0.0);

    graphics_encoder_ =
        command_buffer_->renderCommandEncoder(pass_descriptor.get());

    graphics_encoder_->setRenderPipelineState(pipeline_state_.get());
    graphics_encoder_->setFrontFacingWinding(MTL::WindingCounterClockwise);
    graphics_encoder_->setCullMode(MTL::CullModeBack);
    graphics_encoder_->setDepthStencilState(depth_state_.get());
}

void MetalRenderer::draw(const DrawCommand &cmd) {
    if (graphics_encoder_ == nullptr) return;

    if (!cmd.push_constants.empty()) {
        graphics_encoder_->setVertexBytes(cmd.push_constants.data(),
                                          cmd.push_constants.size(), 0);
    }

    MTL::Buffer *vertex_buffer = getBuffer(cmd.vertex_buffer);
    if (vertex_buffer == nullptr) {
        UME_LOG_WARN(Renderer,
                     "attempted to draw using invalid vertex buffer {}",
                     cmd.vertex_buffer.id);
        return;
    }

    MTL::Buffer *index_buffer = getBuffer(cmd.index_buffer);
    if (index_buffer == nullptr) {
        UME_LOG_WARN(Renderer,
                     "attempted to draw using invalid index buffer {}",
                     cmd.index_buffer.id);
        return;
    }

    MTL::IndexType index_type = cmd.index_type == IndexType::UInt16
                                    ? MTL::IndexTypeUInt16
                                    : MTL::IndexTypeUInt32;

    graphics_encoder_->setVertexBuffer(vertex_buffer, 0, 1);
    graphics_encoder_->drawIndexedPrimitives(
        MTL::PrimitiveTypeTriangle, NS::UInteger(cmd.index_count), index_type,
        index_buffer, NS::UInteger(0));
}

void MetalRenderer::postProcess(const PostProcessCommand &cmd) {
    if (graphics_encoder_ == nullptr) return;
    graphics_encoder_->endEncoding();
    graphics_encoder_ = nullptr;

    drawable_ = layer_->nextDrawable();
    if (drawable_ == nullptr) {
        graphics_encoder_ = nullptr;
        command_buffer_ = nullptr;
        return;
    }

    if (cmd.passes.empty()) {
        MTL::BlitCommandEncoder *blit = command_buffer_->blitCommandEncoder();
        blit->copyFromTexture(getTexture(color_targets_[0]),
                              drawable_->texture());
        blit->endEncoding();
        return;
    }

    int src = 0;
    for (size_t i = 0; i < cmd.passes.size(); i++) {
        const PostProcessPass &pass = cmd.passes[i];
        MetalGraphicsPipeline *pipeline =
            graphics_pipelines_.get(pass.pipeline);
        if (pipeline == nullptr) {
            UME_LOG_WARN(Renderer, "post pass with invalid pipeline {}",
                         pass.pipeline.id);
            continue;
        }

        const bool last = (i + 1 == cmd.passes.size());
        MTL::Texture *dst =
            last ? drawable_->texture() : getTexture(color_targets_[1 - src]);

        auto pass_desc =
            NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
        auto *color = pass_desc->colorAttachments()->object(0);
        color->setTexture(dst);
        color->setLoadAction(MTL::LoadActionDontCare);
        color->setStoreAction(MTL::StoreActionStore);

        MTL::RenderCommandEncoder *enc =
            command_buffer_->renderCommandEncoder(pass_desc.get());
        enc->setRenderPipelineState(pipeline->state.get());
        enc->setCullMode(MTL::CullModeNone);
        enc->setFragmentTexture(getTexture(color_targets_[src]), 0);
        enc->setFragmentTexture(getTexture(depth_texture_), 1);
        enc->setFragmentSamplerState(getSampler(linear_sampler_), 0);
        enc->setFragmentBytes(cmd.frame_uniforms.data(),
                              cmd.frame_uniforms.size(), 0);
        if (!pass.params.empty()) {
            enc->setFragmentBytes(pass.params.data(), pass.params.size(), 1);
        }
        enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0),
                            NS::UInteger(3));
        enc->endEncoding();

        if (!last) {
            src = 1 - src;
        }
    }
}

void MetalRenderer::endFrame() {
    // if (drawable_ == nullptr) {
    //     frame_pool_->release();
    //     frame_pool_ = nullptr;
    //     return;
    // }

    if (graphics_encoder_ != nullptr) {
        graphics_encoder_->endEncoding();
        graphics_encoder_ = nullptr;
    }

    if (drawable_ != nullptr) {
        command_buffer_->presentDrawable(drawable_);
    }

    command_buffer_->commit();

    graphics_encoder_ = nullptr;
    command_buffer_ = nullptr;
    drawable_ = nullptr;

    frame_pool_.reset();
}

void MetalRenderer::beginComputePass() {
    if (command_buffer_ == nullptr) return;
    compute_encoder_ = command_buffer_->computeCommandEncoder();
}

void MetalRenderer::dispatch(const DispatchCommand &cmd) {
    if (compute_encoder_ == nullptr) {
        UME_LOG_WARN(Renderer, "dispatch() called outside of compute pass");
        return;
    }

    MetalComputePipeline *pipeline = compute_pipelines_.get(cmd.pipeline);
    if (pipeline == nullptr) {
        UME_LOG_WARN(Renderer,
                     "dispatch() called with invalid compute pipeline {}",
                     cmd.pipeline.id);
        return;
    }

    compute_encoder_->setComputePipelineState(pipeline->state.get());

    for (const auto &binding : cmd.bindings.buffers) {
        MTL::Buffer *buffer = getBuffer(binding.buffer);
        if (buffer == nullptr) {
            UME_LOG_WARN(Renderer,
                         "dispatch() called with invalid buffer {} at slot {}",
                         binding.buffer.id, binding.slot);
            return;
        }
        compute_encoder_->setBuffer(buffer, binding.offset, binding.slot);
    }

    for (const auto &binding : cmd.bindings.textures) {
        MTL::Texture *texture = getTexture(binding.texture);
        if (texture == nullptr) {
            UME_LOG_WARN(Renderer,
                         "dispatch() called with invalid texture {} at slot {}",
                         binding.texture.id, binding.slot);
            return;
        }
        compute_encoder_->setTexture(texture, binding.slot);
    }

    for (const auto &binding : cmd.bindings.samplers) {
        MTL::SamplerState *sampler = getSampler(binding.sampler);
        if (sampler == nullptr) {
            UME_LOG_WARN(Renderer,
                         "dispatch() called with invalid sampler {} at slot {}",
                         binding.sampler.id, binding.slot);
            return;
        }
        compute_encoder_->setSamplerState(sampler, binding.slot);
    }

    const auto &wg = pipeline->workgroup_size;
    compute_encoder_->dispatchThreads(
        MTL::Size(cmd.work_size[0], cmd.work_size[1], cmd.work_size[2]),
        MTL::Size(wg[0], wg[1], wg[2]));
}

void MetalRenderer::endComputePass() {
    if (compute_encoder_ == nullptr) return;
    compute_encoder_->endEncoding();
    compute_encoder_ = nullptr;
}

BufferHandle MetalRenderer::createBuffer(const BufferDescription &desc) {
    if (desc.size == 0) {
        UME_LOG_WARN(Renderer, "attempted to create buffer with size zero");
        return {};
    }

    if (desc.initial_data != nullptr && desc.usage == BufferUsage::GpuOnly) {
        UME_LOG_WARN(Renderer, "gpu only buffers cannot have initial data");
        return {};
    }

    NS::SharedPtr<MTL::Buffer> buffer;

    switch (desc.usage) {
    case BufferUsage::CpuToGpu:
        buffer = NS::TransferPtr(
            desc.initial_data != nullptr
                ? device_->newBuffer(desc.initial_data, desc.size,
                                     MTL::ResourceStorageModeShared)
                : device_->newBuffer(desc.size,
                                     MTL::ResourceStorageModeShared));
        break;
    case BufferUsage::GpuOnly:
        buffer = NS::TransferPtr(
            device_->newBuffer(desc.size, MTL::ResourceStorageModePrivate));
        break;
    }

    if (!buffer) {
        UME_LOG_WARN(Renderer, "failed to allocate {} byte buffer", desc.size);
        return {};
    }

    return buffers_.insert(
        MetalBuffer{.buffer = std::move(buffer), .size = desc.size});
}

void MetalRenderer::destroyBuffer(BufferHandle handle) {
    buffers_.remove(handle);
}

void MetalRenderer::readBuffer(BufferHandle handle, size_t offset,
                               std::span<std::byte> out) {
    MetalBuffer *entry = buffers_.get(handle);

    if (entry == nullptr) {
        UME_LOG_WARN(Renderer, "invalid buffer handle passed to readBuffer()");
        return;
    }

    if (entry->buffer->storageMode() == MTL::StorageModePrivate) {
        UME_LOG_WARN(Renderer, "cannot read gpu only buffer {}", handle.id);
        return;
    }

    if (out.size() > entry->size || offset > entry->size - out.size()) {
        UME_LOG_WARN(Renderer,
                     "read of {} bytes at offset {} exceeds buffer size {}",
                     out.size(), offset, entry->size);
        return;
    }

    const auto *src = static_cast<const std::byte *>(entry->buffer->contents());
    std::memcpy(out.data(), src + offset, out.size());
}

TextureHandle MetalRenderer::createTexture(const TextureDescription &desc) {
    return {};
}

void MetalRenderer::destroyTexture(TextureHandle handle) {
    textures_.remove(handle);
}

GraphicsPipelineHandle
MetalRenderer::createGraphicsPipeline(const GraphicsPipelineDescription &desc) {
    auto library = libraryFromSource(desc.shader);
    if (!library) return {};

    auto state = buildGraphicsPipeline(library.get(), desc.vertex_entry,
                                       desc.fragment_entry, false);
    if (!state) return {};

    return graphics_pipelines_.insert(
        MetalGraphicsPipeline{.state = std::move(state)});
}

void MetalRenderer::destroyGraphicsPipeline(GraphicsPipelineHandle handle) {
    graphics_pipelines_.remove(handle);
}

ComputePipelineHandle
MetalRenderer::createComputePipeline(const ComputePipelineDescription &desc) {
    auto library = libraryFromSource(desc.shader);
    if (!library) return {};

    auto state = buildComputePipeline(library.get(), desc.entry);

    if (!state) return {};

    const uint32_t total_threads = desc.workgroup_size[0] *
                                   desc.workgroup_size[1] *
                                   desc.workgroup_size[2];

    if (total_threads > state->maxTotalThreadsPerThreadgroup()) {
        UME_LOG_ERROR(Renderer,
                      "compute workgroup size {} exceeds device maximum {}",
                      total_threads, state->maxTotalThreadsPerThreadgroup());
        return {};
    }

    return compute_pipelines_.insert(MetalComputePipeline{
        .state = std::move(state), .workgroup_size = desc.workgroup_size});
}
void MetalRenderer::destroyComputePipeline(ComputePipelineHandle handle) {
    compute_pipelines_.remove(handle);
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
MetalRenderer::buildGraphicsPipeline(MTL::Library *library, const char *vert,
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

NS::SharedPtr<MTL::ComputePipelineState>
MetalRenderer::buildComputePipeline(MTL::Library *library, const char *entry) {
    auto fn = NS::TransferPtr(library->newFunction(
        NS::String::string(entry, NS::UTF8StringEncoding)));
    if (!fn) {
        UME_LOG_ERROR(Renderer, "compute entrypoint '{}' not found", entry);
        return {};
    }

    NS::Error *error = nullptr;
    auto state =
        NS::TransferPtr(device_->newComputePipelineState(fn.get(), &error));
    if (!state) {
        UME_LOG_ERROR(Renderer, "failed to create compute pipeline: {}",
                      errorString(error));
    }
    return state;
}

void MetalRenderer::createRenderTargets(uint32_t width, uint32_t height) {
    auto make_target = [&](MTL::PixelFormat format, const char *name) {
        MTL::TextureDescriptor *desc =
            MTL::TextureDescriptor::texture2DDescriptor(format, width, height,
                                                        false);
        desc->setStorageMode(MTL::StorageModePrivate);
        desc->setUsage(MTL::TextureUsageRenderTarget |
                       MTL::TextureUsageShaderRead);

        auto tex = NS::TransferPtr(device_->newTexture(desc));
        if (!tex) {
            throw Error(logger::Category::Renderer,
                        "failed to create {} texture", name);
        }
        return tex;
    };

    destroyTexture(color_targets_[0]);
    destroyTexture(color_targets_[1]);
    destroyTexture(depth_texture_);

    color_targets_[0] = textures_.insert({
        .texture = make_target(MTL::PixelFormatBGRA8Unorm, "scene color"),
    });

    color_targets_[1] = textures_.insert({
        .texture = make_target(MTL::PixelFormatBGRA8Unorm, "post color"),
    });

    depth_texture_ = textures_.insert({
        .texture = make_target(MTL::PixelFormatDepth32Float, "depth"),
    });
}

MTL::Buffer *MetalRenderer::getBuffer(BufferHandle handle) {
    MetalBuffer *buffer = buffers_.get(handle);

    if (buffer != nullptr) return buffer->buffer.get();
    return nullptr;
}

MTL::Texture *MetalRenderer::getTexture(TextureHandle handle) {
    MetalTexture *texture = textures_.get(handle);

    if (texture != nullptr) return texture->texture.get();
    return nullptr;
}

MTL::SamplerState *MetalRenderer::getSampler(SamplerHandle handle) {
    MetalSampler *sampler = samplers_.get(handle);

    if (sampler != nullptr) return sampler->state.get();
    return nullptr;
}

std::unique_ptr<RendererBackend> createRendererBackend(const Window &window) {
    return std::make_unique<MetalRenderer>(window.createMetalSurface(),
                                           window.getPixelWidth(),
                                           window.getPixelHeight());
}
} // namespace ume