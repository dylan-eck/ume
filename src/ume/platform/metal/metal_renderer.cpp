#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "ume/platform/metal/metal_renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/core/error.hpp"
#include "ume/core/resource_handle.hpp"
#include "ume/platform/window.hpp"
#include "ume/renderer/renderer_backend.hpp"
#include "ume/renderer/backend_types.hpp"
#include "ume/platform/metal/metal_conversions.hpp"

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <span>
#include <utility>

namespace ume {

namespace {
const char *errorString(NS::Error *error) {
    if (error == nullptr || error->localizedDescription() == nullptr) {
        return "unknown error";
    }
    return error->localizedDescription()->utf8String();
}

struct VertexBinder {
    MTL::RenderCommandEncoder *encoder;
    static constexpr const char *kName = "vertex";

    void buffer(MTL::Buffer *buffer, NS::UInteger offset,
                NS::UInteger slot) const {
        encoder->setVertexBuffer(buffer, offset, slot);
    }

    void texture(MTL::Texture *texture, NS::UInteger slot) const {
        encoder->setVertexTexture(texture, slot);
    }

    void sampler(MTL::SamplerState *sampler, NS::UInteger slot) const {
        encoder->setVertexSamplerState(sampler, slot);
    }

    void inlineData(const void *data, NS::UInteger size,
                    NS::UInteger slot) const {
        encoder->setVertexBytes(data, size, slot);
    }
};

struct FragmentBinder {
    MTL::RenderCommandEncoder *encoder;
    static constexpr const char *kName = "fragment";

    void buffer(MTL::Buffer *buffer, NS::UInteger offset,
                NS::UInteger slot) const {
        encoder->setFragmentBuffer(buffer, offset, slot);
    }

    void texture(MTL::Texture *texture, NS::UInteger slot) const {
        encoder->setFragmentTexture(texture, slot);
    }

    void sampler(MTL::SamplerState *sampler, NS::UInteger slot) const {
        encoder->setFragmentSamplerState(sampler, slot);
    }

    void inlineData(const void *data, NS::UInteger size,
                    NS::UInteger slot) const {
        encoder->setFragmentBytes(data, size, slot);
    }
};

struct ComputeBinder {
    MTL::ComputeCommandEncoder *encoder;
    static constexpr const char *kName = "compute";

    void buffer(MTL::Buffer *buffer, NS::UInteger offset,
                NS::UInteger slot) const {
        encoder->setBuffer(buffer, offset, slot);
    }

    void texture(MTL::Texture *texture, NS::UInteger slot) const {
        encoder->setTexture(texture, slot);
    }

    void sampler(MTL::SamplerState *sampler, NS::UInteger slot) const {
        encoder->setSamplerState(sampler, slot);
    }

    void inlineData(const void *data, NS::UInteger size,
                    NS::UInteger slot) const {
        encoder->setBytes(data, size, slot);
    }
};
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
    layer_->setDisplaySyncEnabled(true);
    layer_->setDrawableSize({
        .width = static_cast<CGFloat>(pixel_width),
        .height = static_cast<CGFloat>(pixel_height),
    });

    width_ = pixel_width;
    height_ = pixel_height;

    backbuffer_ = textures_.insert({
        .description =
            {
                .width = width_,
                .height = height_,
                .format = Format::BGRA8Unorm,
                .usage = TextureUsage::RenderTarget,
            },
        .texture = {},
    });

    command_queue_ = NS::TransferPtr(device_->newCommandQueue());
    if (!command_queue_) {
        throw Error(logger::Category::Renderer,
                    "failed to create metal command queue");
    }

    UME_LOG_INFO(Renderer, "initialized metal renderer backend");
}

void MetalRenderer::resize(uint32_t width, uint32_t height) {
    if (width == width_ && height == height_) return;

    width_ = width;
    height_ = height;

    layer_->setDrawableSize({
        .width = static_cast<CGFloat>(width),
        .height = static_cast<CGFloat>(height),
    });

    MetalTexture *bb_texture = textures_.get(backbuffer_);
    bb_texture->description.width = width;
    bb_texture->description.height = height;
}

TextureHandle MetalRenderer::backbuffer() const { return backbuffer_; }

void MetalRenderer::beginFrame() {
    frame_pool_ = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
    command_buffer_ = command_queue_->commandBuffer();
    curr_frame_failed_ = false;
}

std::expected<void, std::string>
MetalRenderer::validateRenderPass(const RenderPassDescription &desc) {
    if (desc.color_attachments.empty() && !desc.depth_attachment.has_value()) {
        return std::unexpected("render pass must have at least one attachment");
    }

    std::optional<std::pair<uint32_t, uint32_t>> size;

    auto check = [&](TextureHandle handle, std::string_view kind, size_t index,
                     bool is_depth) -> std::expected<void, std::string> {
        const MetalTexture *texture = textures_.get(handle);

        if (texture == nullptr) {
            return std::unexpected(
                std::format("{} {} texture handle is invalid", kind, index));
        }

        // TODO: may need to support multiple depth formats in future
        if (is_depth && texture->description.format != Format::Depth32Float) {
            return std::unexpected(
                std::format("{} format is not a depth format", kind));
        }

        if (is_depth && handle == backbuffer_) {
            return std::unexpected(
                "backbuffer cannot be bound as a depth texture");
        }

        if (!is_depth && texture->description.format == Format::Depth32Float) {
            return std::unexpected(
                std::format("{} {} format is not a color format", kind, index));
        }

        if (!size.has_value()) {
            size = std::pair(texture->description.width,
                             texture->description.height);
            return {};
        }

        if (texture->description.width != size->first ||
            texture->description.height != size->second) {
            return std::unexpected(
                std::format("{} {} is a different size than other attachments",
                            kind, index));
        }

        return {};
    };

    for (size_t i = 0; i < desc.color_attachments.size(); i++) {
        if (!desc.color_attachments[i].texture) continue;

        if (auto result = check(desc.color_attachments[i].texture,
                                "color attachment", i, false);
            !result) {
            return result;
        }
    }

    if (desc.depth_attachment.has_value()) {
        if (auto result = check(desc.depth_attachment->texture,
                                "depth attachment", 0, true);
            !result) {
            return result;
        }
    }

    return {};
}

void MetalRenderer::beginRenderPass(const RenderPassDescription &desc) {
    if (curr_frame_failed_) return;

    if (curr_pass_desc_) {
        failFrame("cannot start new render pass before previous has finished");
        return;
    }

    if (auto result = validateRenderPass(desc); !result) {
        failFrame(std::format("invalid render pass: {}", result.error()));
        return;
    }

    if (std::ranges::contains(desc.color_attachments, backbuffer_,
                              &ColorAttachment::texture) &&
        !acquireBackbuffer()) {
        failFrame("failed to acquire drawable");
        return;
    }

    curr_pass_desc_ = desc;

    std::ranges::transform(desc.color_attachments,
                           curr_pass_color_formats_.begin(),
                           [&](const ColorAttachment &color_attachment) {
                               const MetalTexture *texture =
                                   textures_.get(color_attachment.texture);
                               return texture != nullptr
                                          ? texture->description.format
                                          : Format::Undefined;
                           });

    if (desc.depth_attachment) {
        const MetalTexture *texture =
            textures_.get(desc.depth_attachment->texture);
        // texture shouldn't ever be null here
        curr_pass_depth_format_ = texture->description.format;
    } else {
        curr_pass_depth_format_ = Format::Undefined;
    }

    auto pass_descriptor =
        NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());

    for (size_t i = 0; i < desc.color_attachments.size(); i++) {
        const ColorAttachment &color = desc.color_attachments[i];

        if (!color.texture) continue;

        auto *mtl_color = pass_descriptor->colorAttachments()->object(i);

        mtl_color->setTexture(getTexture(color.texture));
        mtl_color->setLoadAction(metal::toMTLLoadAction(color.load_action));
        mtl_color->setStoreAction(metal::toMTLStoreAction(color.store_action));
        mtl_color->setClearColor(metal::toMTLClearColor(color.clear_value));
    }

    if (desc.depth_attachment.has_value()) {
        const DepthAttachment &depth = desc.depth_attachment.value();
        auto *mtl_depth = pass_descriptor->depthAttachment();

        mtl_depth->setTexture(getTexture(depth.texture));
        mtl_depth->setLoadAction(metal::toMTLLoadAction(depth.load_action));
        mtl_depth->setStoreAction(metal::toMTLStoreAction(depth.store_action));
        mtl_depth->setClearDepth(depth.clear_value);
    }

    graphics_encoder_ =
        command_buffer_->renderCommandEncoder(pass_descriptor.get());

    curr_graphics_pipeline_ = {};
}

void MetalRenderer::endRenderPass() {
    curr_pass_desc_.reset();
    curr_pass_color_formats_ = {};
    curr_pass_depth_format_ = Format::Undefined;

    if (graphics_encoder_ != nullptr) {
        graphics_encoder_->endEncoding();
        graphics_encoder_ = nullptr;
    }
}

void MetalRenderer::draw(const DrawCommand &cmd) {
    if (curr_frame_failed_) return;

    if (!curr_pass_desc_) {
        UME_LOG_WARN(Renderer, "draw() called outside of a render pass");
        return;
    }

    MetalGraphicsPipeline *pipeline = graphics_pipelines_.get(cmd.pipeline);
    if (pipeline == nullptr) {
        UME_LOG_WARN(Renderer,
                     "draw() called with invalid graphics pipeline {}",
                     cmd.pipeline.id);
        return;
    }

    if (pipeline->description.color_formats != curr_pass_color_formats_ ||
        pipeline->description.depth_format != curr_pass_depth_format_) {
        UME_LOG_WARN(
            Renderer,
            "pipeline formats and render pass attachments do not match");
        return;
    }

    if (cmd.pipeline != curr_graphics_pipeline_) {
        graphics_encoder_->setRenderPipelineState(pipeline->state.get());
        curr_graphics_pipeline_ = cmd.pipeline;

        graphics_encoder_->setCullMode(
            metal::toMTLCullMode(pipeline->description.raster_state.cull_mode));

        graphics_encoder_->setFrontFacingWinding(metal::toMTLWinding(
            pipeline->description.raster_state.front_face_winding));

        if (pipeline->depth_state) {
            graphics_encoder_->setDepthStencilState(
                pipeline->depth_state.get());
        }
    }

    if (!bindResources(VertexBinder{graphics_encoder_}, cmd.vertex_bindings) ||
        !bindResources(FragmentBinder{graphics_encoder_},
                       cmd.fragment_bindings)) {
        return;
    }

    if (cmd.index_buffer) {
        if (cmd.index_count == 0 || cmd.index_count % 3 != 0) {
            UME_LOG_WARN(Renderer, "attempted indexed draw with index count {}",
                         cmd.index_count);
            return;
        }

        MTL::Buffer *index_buffer = getBuffer(cmd.index_buffer);
        if (index_buffer == nullptr) {
            UME_LOG_WARN(Renderer,
                         "attempted indexed draw using stale index buffer {}",
                         cmd.index_buffer.id);
            return;
        }

        MTL::IndexType index_type = cmd.index_type == IndexType::UInt16
                                        ? MTL::IndexTypeUInt16
                                        : MTL::IndexTypeUInt32;

        graphics_encoder_->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
                                                 cmd.index_count, index_type,
                                                 index_buffer, 0);
    } else {
        if (cmd.vertex_count < 3 || cmd.vertex_count % 3 != 0) {
            UME_LOG_WARN(
                Renderer,
                "attempted non-indexed draw with invalid vertex count {}",
                cmd.vertex_count);
            return;
        }

        graphics_encoder_->drawPrimitives(MTL::PrimitiveTypeTriangle,
                                          NS::UInteger{0}, cmd.vertex_count);
    }
}

void MetalRenderer::endFrame() {
    if (curr_pass_desc_) {
        endRenderPass();
        UME_LOG_WARN(Renderer, "render pass not closed before end of frame");
    }

    if (!curr_frame_failed_) {
        last_error_ = "";

        if (drawable_ != nullptr) {
            command_buffer_->presentDrawable(drawable_);
        }
    }

    command_buffer_->commit();

    graphics_encoder_ = nullptr;
    command_buffer_ = nullptr;
    drawable_ = nullptr;

    frame_pool_.reset();

    MetalTexture *bb_texture = textures_.get(backbuffer_);
    bb_texture->texture = {};
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

    if (!bindResources(ComputeBinder{compute_encoder_}, cmd.bindings)) {
        return;
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
    if (desc.width == 0 || desc.height == 0) {
        UME_LOG_WARN(Renderer,
                     "createTexture() called with zero width or height");
        return {};
    }

    if (desc.format == Format::Undefined) {
        UME_LOG_WARN(Renderer, "createTexture() called with undefined format");
        return {};
    }

    auto descriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
    // TODO: support other texture types
    descriptor->setTextureType(MTL::TextureType2D);
    descriptor->setPixelFormat(metal::toMTLPixelFormat(desc.format));
    descriptor->setWidth(desc.width);
    descriptor->setHeight(desc.height);
    descriptor->setStorageMode(MTL::StorageModePrivate);
    descriptor->setUsage(metal::toMTLTextureUsage(desc.usage));

    auto texture = NS::TransferPtr(device_->newTexture(descriptor.get()));
    if (!texture) {
        // TODO: log format as well
        UME_LOG_WARN(Renderer, "failed to create {}x{} texture", desc.width,
                     desc.height);
        return {};
    }

    return textures_.insert({
        .description = desc,
        .texture = texture,
    });
}

void MetalRenderer::destroyTexture(TextureHandle handle) {
    if (handle == backbuffer_) {
        UME_LOG_WARN(Renderer, "attempted to destroy backbuffer");
        return;
    }
    textures_.remove(handle);
}

void MetalRenderer::copyTexture(TextureHandle src, TextureHandle dst) {
    if (curr_frame_failed_) return;

    if (curr_pass_desc_ || compute_encoder_ != nullptr) {
        UME_LOG_WARN(Renderer,
                     "cannot copy texture while render pass is in progress");
        return;
    }

    if (src == backbuffer_) {
        UME_LOG_WARN(Renderer, "cannot copy from the backbuffer");
        return;
    }

    MetalTexture *src_texture = textures_.get(src);
    MetalTexture *dst_texture = textures_.get(dst);

    if (src_texture == nullptr) {
        UME_LOG_WARN(Renderer, "source texture is invalid");
        return;
    }

    if (dst_texture == nullptr) {
        UME_LOG_WARN(Renderer, "destination texture is invalid");
        return;
    }

    if (src_texture->description.format != dst_texture->description.format) {
        UME_LOG_WARN(Renderer,
                     "source and destination textures have different format");
        return;
    }

    if (src_texture->description.width != dst_texture->description.width ||
        src_texture->description.height != dst_texture->description.height) {
        UME_LOG_WARN(Renderer,
                     "source and destination textures are different sizes");
        return;
    }

    if (dst == backbuffer_ && !acquireBackbuffer()) {
        failFrame("failed to acquire drawable");
        return;
    }

    MTL::BlitCommandEncoder *encoder = command_buffer_->blitCommandEncoder();
    encoder->copyFromTexture(getTexture(src), getTexture(dst));
    encoder->endEncoding();
}

SamplerHandle MetalRenderer::createSampler(const SamplerDescription &desc) {
    auto descriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
    descriptor->setMinFilter(metal::toMTLFilterType(desc.min_filter));
    descriptor->setMagFilter(metal::toMTLFilterType(desc.mag_filter));
    descriptor->setSAddressMode(metal::toMTLAddressMode(desc.address_mode));
    descriptor->setTAddressMode(metal::toMTLAddressMode(desc.address_mode));

    auto sampler = NS::TransferPtr(device_->newSamplerState(descriptor.get()));
    if (!sampler) {
        UME_LOG_WARN(Renderer, "failed to create sampler");
        return {};
    }

    return samplers_.insert({
        .state = sampler,
    });
}

void MetalRenderer::destroySampler(SamplerHandle handle) {
    samplers_.remove(handle);
}

ShaderHandle MetalRenderer::createShader(const ShaderDescription &desc) {
    auto library = libraryFromSource(desc.code);
    if (!library) return {};

    return shaders_.insert({
        .library = library,
        .entry_points = {desc.entry_points.begin(), desc.entry_points.end()},
    });
}

void MetalRenderer::destroyShader(ShaderHandle handle) {
    shaders_.remove(handle);
}

GraphicsPipelineHandle
MetalRenderer::createGraphicsPipeline(const GraphicsPipelineDescription &desc) {
    if (desc.depth_format == Format::Undefined &&
        (desc.depth_state.compare != CompareFunction::Always ||
         desc.depth_state.write)) {
        UME_LOG_WARN(Renderer,
                     "pipeline with depth enabled must have a depth format");
        return {};
    }

    MetalShader *shader = shaders_.get(desc.shader);
    if (shader == nullptr) return {};

    auto check_entry = [&](std::string_view name, ShaderStage stage) -> bool {
        auto itr =
            std::ranges::find(shader->entry_points, name, &EntryPoint::name);

        if (itr == shader->entry_points.end()) {
            UME_LOG_WARN(Renderer, "no entry point named '{}' in shader", name);
            return false;
        }

        if ((*itr).stage != stage) {
            UME_LOG_WARN(Renderer,
                         "entry point '{}' exists, but has incorrect stage",
                         name);
            return false;
        }
        return true;
    };

    if (!check_entry(desc.vertex_entry, ShaderStage::Vertex) ||
        !check_entry(desc.fragment_entry, ShaderStage::Fragment)) {
        return {};
    }

    auto library = shader->library;
    auto state = buildGraphicsPipeline(desc, library.get());
    if (!state) return {};

    MetalGraphicsPipeline metal_pipeline{
        .description = desc,
        .state = std::move(state),
    };

    if (desc.depth_format != Format::Undefined) {
        auto depth_desc =
            NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
        depth_desc->setDepthCompareFunction(
            metal::toMTLCompareFunction(desc.depth_state.compare));
        depth_desc->setDepthWriteEnabled(desc.depth_state.write);

        auto depth_state =
            NS::TransferPtr(device_->newDepthStencilState(depth_desc.get()));

        if (!depth_state) {
            UME_LOG_ERROR(Renderer,
                          "failed to create metal depth stencil state");
            return {};
        }

        metal_pipeline.depth_state = std::move(depth_state);
    }

    return graphics_pipelines_.insert(metal_pipeline);
}

void MetalRenderer::destroyGraphicsPipeline(GraphicsPipelineHandle handle) {
    graphics_pipelines_.remove(handle);
}

ComputePipelineHandle
MetalRenderer::createComputePipeline(const ComputePipelineDescription &desc) {
    MetalShader *shader = shaders_.get(desc.shader);
    if (shader == nullptr) return {};

    auto itr =
        std::ranges::find(shader->entry_points, desc.entry, &EntryPoint::name);
    if (itr == shader->entry_points.end()) {
        UME_LOG_WARN(Renderer, "no entry point named '{}' in shader",
                     desc.entry);
        return {};
    }
    if (itr->stage != ShaderStage::Compute) {
        UME_LOG_WARN(Renderer, "entry point '{}' is not a compute stage",
                     desc.entry);
        return {};
    }

    auto library = shader->library;
    auto state = buildComputePipeline(library.get(), desc.entry);
    if (!state) return {};

    std::array<uint32_t, 3> wg = (*itr).workgroup_size;

    const uint32_t total_threads = wg[0] * wg[1] * wg[2];

    if (total_threads > state->maxTotalThreadsPerThreadgroup()) {
        UME_LOG_ERROR(Renderer,
                      "compute workgroup size {} exceeds device maximum {}",
                      total_threads, state->maxTotalThreadsPerThreadgroup());
        return {};
    }

    return compute_pipelines_.insert(
        MetalComputePipeline{.state = std::move(state), .workgroup_size = wg});
}

void MetalRenderer::destroyComputePipeline(ComputePipelineHandle handle) {
    compute_pipelines_.remove(handle);
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
MetalRenderer::buildGraphicsPipeline(const GraphicsPipelineDescription &desc,
                                     MTL::Library *library) {
    auto vfn = NS::TransferPtr(library->newFunction(
        NS::String::string(desc.vertex_entry.c_str(), NS::UTF8StringEncoding)));
    auto ffn = NS::TransferPtr(library->newFunction(NS::String::string(
        desc.fragment_entry.c_str(), NS::UTF8StringEncoding)));
    if (!vfn || !ffn) {
        UME_LOG_ERROR(Renderer, "entrypoints '{}'/'{}' not found",
                      desc.vertex_entry.c_str(), desc.fragment_entry.c_str());
        return {};
    }

    auto mtl_pipeline_desc =
        NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
    mtl_pipeline_desc->setVertexFunction(vfn.get());
    mtl_pipeline_desc->setFragmentFunction(ffn.get());

    for (size_t i = 0; i < desc.color_formats.size(); i++) {
        if (desc.color_formats[i] == Format::Undefined) continue;

        mtl_pipeline_desc->colorAttachments()->object(i)->setPixelFormat(
            metal::toMTLPixelFormat(desc.color_formats[i]));
    }

    if (desc.depth_format != Format::Undefined) {
        mtl_pipeline_desc->setDepthAttachmentPixelFormat(
            metal::toMTLPixelFormat(desc.depth_format));
    }

    NS::Error *error = nullptr;
    auto state = NS::TransferPtr(
        device_->newRenderPipelineState(mtl_pipeline_desc.get(), &error));
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

bool MetalRenderer::acquireBackbuffer() {
    if (drawable_ != nullptr) return true;

    drawable_ = layer_->nextDrawable();
    if (drawable_ == nullptr) return false;

    MetalTexture *bb_texture = textures_.get(backbuffer_);
    bb_texture->texture = NS::RetainPtr(drawable_->texture());
    return true;
}

void MetalRenderer::failFrame(std::string message) {
    curr_frame_failed_ = true;

    if (message != last_error_) {
        UME_LOG_WARN(Renderer, "{}", message);
    }

    last_error_ = std::move(message);
}

template <typename Binder>
bool MetalRenderer::bindResources(const Binder &binder,
                                  const ResourceBindings &bindings) {

    for (const auto &binding : bindings.buffers) {
        MTL::Buffer *buffer = getBuffer(binding.buffer);
        if (buffer == nullptr) {
            UME_LOG_WARN(Renderer, "invalid {} buffer binding {} at slot {}",
                         Binder::kName, binding.buffer.id, binding.slot);
            return false;
        }

        binder.buffer(buffer, binding.offset, binding.slot);
    }

    for (const auto &binding : bindings.textures) {
        MTL::Texture *texture = getTexture(binding.texture);

        if (texture == nullptr) {
            UME_LOG_WARN(Renderer, "invalid {} texture binding {} at slot {}",
                         Binder::kName, binding.texture.id, binding.slot);
            return false;
        }

        if (binding.texture == backbuffer_) {
            UME_LOG_WARN(Renderer, "backbuffer cannot be bound as a texture");
            return false;
        }

        // TODO: maybe detect this earlier (before binding a bunch of stuff?)
        if (isCurrentPassAttachment(binding.texture)) {
            UME_LOG_WARN(Renderer,
                         "texture binding {} slot {} is an attachment of the "
                         "current pass",
                         binding.texture.id, binding.slot);
            return false;
        }

        binder.texture(texture, binding.slot);
    }

    // NOLINTNEXTLINE(readability-use-anyofallof)
    for (const auto &binding : bindings.samplers) {
        MTL::SamplerState *sampler = getSampler(binding.sampler);
        if (sampler == nullptr) {
            UME_LOG_WARN(Renderer, "invalid {} sampler binding {} at slot {}",
                         Binder::kName, binding.sampler.id, binding.slot);
            return false;
        }

        binder.sampler(sampler, binding.slot);
    }

    for (const InlineDataBinding binding : bindings.inline_data) {
        binder.inlineData(binding.data.data(), binding.data.size(),
                          binding.slot);
    }

    return true;
}

bool MetalRenderer::isCurrentPassAttachment(TextureHandle handle) const {
    if (!handle || !curr_pass_desc_) return false;
    const RenderPassDescription &pass = *curr_pass_desc_;
    return std::ranges::contains(pass.color_attachments, handle,
                                 &ColorAttachment::texture) ||
           (pass.depth_attachment && pass.depth_attachment->texture == handle);
}

std::unique_ptr<RendererBackend> createRendererBackend(const Window &window) {
    return std::make_unique<MetalRenderer>(window.createMetalSurface(),
                                           window.getPixelWidth(),
                                           window.getPixelHeight());
}
} // namespace ume
