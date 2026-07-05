#include "crimson/gpu/gpu_picking.hpp"

#include "crimson/gpu/gpu_handles.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

#include <bx/math.h>

namespace crimson::gpu {
namespace {

using quader::math::Vec3;

constexpr std::uint16_t kPickingReadbackDim = 8;
constexpr bgfx::ViewId kPickingViewId = 32;
constexpr bgfx::ViewId kPickingBlitViewId = 33;

[[nodiscard]] bgfx::TextureFormat::Enum to_bgfx_texture_format(PickingIdTargetFormat format) noexcept
{
    return format == PickingIdTargetFormat::R32Uint
        ? bgfx::TextureFormat::R32U
        : bgfx::TextureFormat::RGBA8;
}

[[nodiscard]] bx::Vec3 to_bx(Vec3 value) noexcept
{
    return {value.x, value.y, value.z};
}

[[nodiscard]] bool homogeneous_depth() noexcept
{
    const bgfx::Caps* caps = bgfx::getCaps();
    return caps != nullptr && caps->homogeneousDepth;
}

void build_camera_matrices(const RenderCamera& camera, float aspect, float* view_matrix, float* projection)
{
    bgfx::setViewMode(kPickingViewId, bgfx::ViewMode::Sequential);
    bx::mtxLookAt(view_matrix, to_bx(camera.eye), to_bx(camera.target), to_bx(camera.up));
    if (camera.projection == CameraProjection::Orthographic) {
        const float extent = std::max(0.01F, camera.orthographic_height_m) * 0.5F;
        bx::mtxOrtho(
            projection,
            -extent * aspect,
            extent * aspect,
            -extent,
            extent,
            camera.near_plane_m,
            camera.far_plane_m,
            0.0F,
            homogeneous_depth());
        return;
    }

    bx::mtxProj(
        projection,
        camera.vertical_fov_degrees,
        aspect,
        camera.near_plane_m,
        camera.far_plane_m,
        homogeneous_depth());
}

[[nodiscard]] float clamped_request_axis(std::uint16_t value, std::uint16_t start, std::uint16_t extent) noexcept
{
    if (extent == 0) {
        return 0.0F;
    }
    const std::uint16_t end = static_cast<std::uint16_t>(std::min<int>(65535, start + extent - 1U));
    const std::uint16_t clamped = static_cast<std::uint16_t>(std::clamp<int>(value, start, end));
    return static_cast<float>(clamped - start) + 0.5F;
}

void configure_picking_view(const RenderView& view, const PickingRequest& request)
{
    const float width = static_cast<float>(std::max<std::uint16_t>(1, view.rect.width));
    const float height = static_cast<float>(std::max<std::uint16_t>(1, view.rect.height));
    const float aspect = width / height;

    float view_matrix[16];
    float projection[16];
    build_camera_matrices(view.camera, aspect, view_matrix, projection);

    float view_projection[16];
    bx::mtxMul(view_projection, view_matrix, projection);

    float inverse_view_projection[16];
    bx::mtxInverse(inverse_view_projection, view_projection);

    const float request_x = clamped_request_axis(request.x_px, view.rect.x, view.rect.width);
    const float request_y = clamped_request_axis(request.y_px, view.rect.y, view.rect.height);
    const float mouse_x_ndc = (request_x / width) * 2.0F - 1.0F;
    const float mouse_y_ndc = ((height - request_y) / height) * 2.0F - 1.0F;

    const bx::Vec3 pick_eye = bx::mulH({mouse_x_ndc, mouse_y_ndc, 0.0F}, inverse_view_projection);
    const bx::Vec3 pick_at = bx::mulH({mouse_x_ndc, mouse_y_ndc, 1.0F}, inverse_view_projection);

    float pick_view[16];
    float pick_projection[16];
    bx::mtxLookAt(pick_view, pick_eye, pick_at, to_bx(view.camera.up));
    bx::mtxProj(pick_projection, 3.0F, 1.0F, view.camera.near_plane_m, view.camera.far_plane_m, homogeneous_depth());

    bgfx::setViewName(kPickingViewId, "PickingPass");
    bgfx::setViewMode(kPickingViewId, bgfx::ViewMode::Sequential);
    bgfx::setViewRect(kPickingViewId, 0, 0, kPickingReadbackDim, kPickingReadbackDim);
    bgfx::setViewClear(
        kPickingViewId,
        BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
        0x00000000,
        1.0F,
        0);
    bgfx::setViewTransform(kPickingViewId, pick_view, pick_projection);
}

[[nodiscard]] const RenderView* find_request_view(
    std::span<const RenderView> views,
    std::uint32_t view_index) noexcept
{
    for (const RenderView& view : views) {
        if (view.view_index == view_index) {
            return &view;
        }
    }
    return nullptr;
}

[[nodiscard]] std::uint32_t dominant_raw_id(std::span<const std::uint8_t> bytes)
{
    std::unordered_map<std::uint32_t, std::uint32_t> counts;
    std::uint32_t best_id = 0;
    std::uint32_t best_count = 0;
    for (std::size_t offset = 0; offset + 3U < bytes.size(); offset += 4U) {
        const std::uint32_t raw_id = decode_picking_id_rgba8(PackedPickingIdRgba8{{
            bytes[offset],
            bytes[offset + 1U],
            bytes[offset + 2U],
            bytes[offset + 3U],
        }});
        if (raw_id == 0) {
            continue;
        }

        const std::uint32_t count = ++counts[raw_id];
        if (count > best_count) {
            best_count = count;
            best_id = raw_id;
        }
    }
    return best_id;
}

void push_resource_error(RendererStatus& status, std::string detail)
{
    status.diagnostics.push_back(RendererDiagnostic{
        .severity = RendererDiagnosticSeverity::Error,
        .code = RendererDiagnosticCode::ResourceCreationFailed,
        .message = "Crimson failed to create picking GPU resources.",
        .detail = std::move(detail),
        .subsystem = "gpu.picking",
        .resource_name = "PickingId",
    });
}

} // namespace

struct GpuPicking::Impl {
    struct PendingReadback {
        PickingRequest request;
        PickingIdAllocator allocator;
        std::array<std::uint8_t, kPickingReadbackDim * kPickingReadbackDim * 4U> bytes{};
        std::uint32_t ready_bgfx_frame = 0;
    };

    PickingIdTargetFormat target_format = PickingIdTargetFormat::Rgba8Data;
    UniqueTextureHandle id_target;
    UniqueTextureHandle depth_target;
    UniqueTextureHandle readback_target;
    UniqueFrameBufferHandle framebuffer;
    UniqueUniformHandle id_uniform;
    std::optional<PendingReadback> pending;

    [[nodiscard]] bool resources_ready() const noexcept
    {
        return id_target.valid()
            && depth_target.valid()
            && readback_target.valid()
            && framebuffer.valid()
            && id_uniform.valid();
    }
};

GpuPicking::GpuPicking()
    : impl_(std::make_unique<Impl>())
{
}

GpuPicking::~GpuPicking()
{
    shutdown();
}

bool GpuPicking::initialize(PickingIdTargetFormat target_format, RendererStatus& status)
{
    shutdown();
    impl_->target_format = target_format;

    const bgfx::TextureFormat::Enum color_format = to_bgfx_texture_format(target_format);
    constexpr std::uint64_t kPointClampSampler = BGFX_SAMPLER_MIN_POINT
        | BGFX_SAMPLER_MAG_POINT
        | BGFX_SAMPLER_MIP_POINT
        | BGFX_SAMPLER_U_CLAMP
        | BGFX_SAMPLER_V_CLAMP;

    impl_->id_target.reset(bgfx::createTexture2D(
        kPickingReadbackDim,
        kPickingReadbackDim,
        false,
        1,
        color_format,
        BGFX_TEXTURE_RT | kPointClampSampler));
    impl_->depth_target.reset(bgfx::createTexture2D(
        kPickingReadbackDim,
        kPickingReadbackDim,
        false,
        1,
        bgfx::TextureFormat::D32F,
        BGFX_TEXTURE_RT | kPointClampSampler));
    impl_->readback_target.reset(bgfx::createTexture2D(
        kPickingReadbackDim,
        kPickingReadbackDim,
        false,
        1,
        color_format,
        BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK | kPointClampSampler));

    const std::array<bgfx::TextureHandle, 2> targets = {
        impl_->id_target.get(),
        impl_->depth_target.get(),
    };
    impl_->framebuffer.reset(bgfx::createFrameBuffer(
        static_cast<std::uint8_t>(targets.size()),
        targets.data(),
        false));
    impl_->id_uniform.reset(bgfx::createUniform("u_pickingId", bgfx::UniformType::Vec4));

    if (!impl_->resources_ready()) {
        push_resource_error(status, "Picking color/depth/readback texture, framebuffer, or ID uniform is invalid.");
        shutdown();
        return false;
    }

    bgfx::setName(impl_->id_target.get(), "PickingIdTarget");
    bgfx::setName(impl_->depth_target.get(), "PickingDepthTarget");
    bgfx::setName(impl_->readback_target.get(), "PickingReadbackTarget");
    return true;
}

void GpuPicking::shutdown() noexcept
{
    impl_->pending = std::nullopt;
    impl_->framebuffer.reset();
    impl_->readback_target.reset();
    impl_->depth_target.reset();
    impl_->id_target.reset();
    impl_->id_uniform.reset();
}

bool GpuPicking::ready() const noexcept
{
    return impl_->resources_ready();
}

GpuPickingFrameResult GpuPicking::submit_frame_requests(
    const FrameSnapshot& snapshot,
    const GpuMeshCache& mesh_cache,
    const GpuProgramCache& program_cache,
    RenderMeshHandle fallback_mesh,
    RenderProgramHandle picking_program,
    std::uint32_t completed_bgfx_frame)
{
    GpuPickingFrameResult result;
    if (impl_->pending && impl_->pending->ready_bgfx_frame != 0
        && impl_->pending->ready_bgfx_frame <= completed_bgfx_frame) {
        const std::uint32_t raw_id = dominant_raw_id(impl_->pending->bytes);
        const std::optional<PickingPayload> payload = impl_->pending->allocator.resolve(raw_id);
        result.completed_results.push_back(PickingResult{
            .request_id = impl_->pending->request.request_id,
            .hit = payload.has_value(),
            .payload = payload.value_or(PickingPayload{}),
        });
        impl_->pending = std::nullopt;
    }

    if (!picking_readback_requested(snapshot.picking_requests()) || impl_->pending || !ready()) {
        return result;
    }

    const bgfx::ProgramHandle program = program_cache.program(picking_program);
    if (!bgfx::isValid(program)) {
        return result;
    }

    const PickingRequest request = snapshot.picking_requests()[0];
    const RenderView* view = find_request_view(snapshot.views(), request.view_index);
    if (view == nullptr) {
        result.completed_results.push_back(PickingResult{.request_id = request.request_id});
        return result;
    }

    impl_->pending.emplace();
    impl_->pending->request = request;
    impl_->pending->allocator.begin_request(request.request_id);

    configure_picking_view(*view, request);
    bgfx::setViewFrameBuffer(kPickingViewId, impl_->framebuffer.get());
    bgfx::touch(kPickingViewId);

    for (const RenderObject& object : snapshot.objects()) {
        if (!object.visible || !object.pickable || object.object_id == 0) {
            continue;
        }

        const RenderMeshHandle mesh_handle = is_valid_handle(object.mesh) ? object.mesh : fallback_mesh;
        const GpuMeshResource* mesh = mesh_cache.get(mesh_handle);
        if (mesh == nullptr || !mesh->valid()) {
            continue;
        }

        const std::uint32_t raw_id = impl_->pending->allocator.allocate(picking_payload_for_object(object));
        if (raw_id == 0) {
            continue;
        }
        const std::array<float, 4> encoded = picking_id_rgba8_to_unorm(encode_picking_id_rgba8(raw_id));

        bgfx::setTransform(object.world_from_object.data());
        bgfx::setUniform(impl_->id_uniform.get(), encoded.data());
        bgfx::setVertexBuffer(0, mesh->vertex_buffer.get());
        bgfx::setIndexBuffer(mesh->index_buffer.get());
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(kPickingViewId, program);
    }

    bgfx::setViewName(kPickingBlitViewId, "PickingReadbackBlit");
    bgfx::blit(kPickingBlitViewId, impl_->readback_target.get(), 0, 0, impl_->id_target.get());
    impl_->pending->ready_bgfx_frame = bgfx::readTexture(
        impl_->readback_target.get(),
        impl_->pending->bytes.data());
    result.scheduled_readbacks = impl_->pending->ready_bgfx_frame == 0 ? 0U : 1U;
    if (result.scheduled_readbacks == 0) {
        impl_->pending = std::nullopt;
    }

    return result;
}

} // namespace crimson::gpu
