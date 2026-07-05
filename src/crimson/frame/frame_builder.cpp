#include "crimson/frame/frame_builder.hpp"

#include "crimson/overlays/grid_overlay.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace crimson {
namespace {

[[nodiscard]] RendererDiagnostic invalid_snapshot_diagnostic(std::string detail) {
	return RendererDiagnostic{
		.severity = RendererDiagnosticSeverity::Error,
		.code = RendererDiagnosticCode::InvalidFrameSnapshot,
		.message = "Crimson received an invalid prototype viewport frame.",
		.detail = std::move(detail),
		.subsystem = "frame",
		.resource_name = "PrototypeViewportFrame",
	};
}

[[nodiscard]] CameraProjection projection_from(PrototypeCameraProjection projection) noexcept {
	return projection == PrototypeCameraProjection::Orthographic
			? CameraProjection::Orthographic
			: CameraProjection::Perspective;
}

[[nodiscard]] RenderCamera render_camera_from(const PrototypeCamera &camera) noexcept {
	return RenderCamera{
		.eye = camera.eye,
		.target = camera.target,
		.up = camera.up,
		.forward = camera.forward,
		.projection = projection_from(camera.projection),
		.vertical_fov_degrees = camera.fov_degrees,
		.orthographic_height_m = camera.orthographic_size,
	};
}

[[nodiscard]] std::array<float, 16> rotation_xy(float x_radians, float y_radians) noexcept {
	const float kSx = std::sin(x_radians);
	const float kCx = std::cos(x_radians);
	const float kSy = std::sin(y_radians);
	const float kCy = std::cos(y_radians);

	return {
		kCy,
		kSx * kSy,
		-kCx * kSy,
		0.0F,
		0.0F,
		kCx,
		kSx,
		0.0F,
		kSy,
		-kSx * kCy,
		kCx * kCy,
		0.0F,
		0.0F,
		0.0F,
		0.0F,
		1.0F,
	};
}

[[nodiscard]] RenderObject make_prototype_cube(double elapsed_seconds, bool animation_enabled) {
	const float kAnimationTime = animation_enabled ? static_cast<float>(elapsed_seconds) : 0.0F;
	RenderObject object;
	object.object_id = 1;
	object.base_shader = BaseShaderId::OpaquePbr;
	object.queue = RenderQueue::Opaque;
	object.world_from_object = rotation_xy(kAnimationTime * 0.8F, kAnimationTime * 1.1F);
	object.world_bounds = quader::math::Aabb{
		.min = { -1.0F, -1.0F, -1.0F },
		.max = { 1.0F, 1.0F, 1.0F },
	};
	object.visible = true;
	return object;
}

} // namespace

quader::foundation::Result<FrameSnapshot, RendererDiagnostic> FrameBuilder::build_prototype_snapshot(
		const PrototypeViewportFrame &frame) const {
	if (!is_valid_extent(frame.target_extent)) {
		return quader::foundation::Result<FrameSnapshot, RendererDiagnostic>::failure(
				invalid_snapshot_diagnostic("Prototype frames require a valid target extent."));
	}

	if (frame.views.empty()) {
		return quader::foundation::Result<FrameSnapshot, RendererDiagnostic>::failure(
				invalid_snapshot_diagnostic("Prototype frames require at least one view."));
	}

	if (frame.cameras.empty()) {
		return quader::foundation::Result<FrameSnapshot, RendererDiagnostic>::failure(
				invalid_snapshot_diagnostic("Prototype frames require at least one camera."));
	}

	std::vector<RenderView> views;
	views.reserve(frame.views.size());
	for (std::size_t index = 0; index < frame.views.size(); ++index) {
		const PrototypeViewportView &source = frame.views[index];
		if (!is_valid_prototype_view(source) || source.camera_index >= frame.cameras.size()) {
			return quader::foundation::Result<FrameSnapshot, RendererDiagnostic>::failure(
					invalid_snapshot_diagnostic("A prototype view has an invalid rectangle or camera index."));
		}

		const char *debug_name = source.debug_name == nullptr ? "Viewport" : source.debug_name;
		views.push_back(RenderView{
				.view_index = static_cast<std::uint32_t>(index),
				.rect = RenderViewportRect{
						.x = source.rect.x,
						.y = source.rect.y,
						.width = source.rect.width,
						.height = source.rect.height,
				},
				.camera = render_camera_from(frame.cameras[source.camera_index]),
				.debug_name = debug_name,
		});
	}

	std::vector<OverlayCommand> overlays;
	overlays.reserve(views.size());
	std::vector<GridOverlayCommand> grid_overlay_payloads;
	grid_overlay_payloads.reserve(views.size());
	for (const RenderView &view : views) {
		GridOverlayCommand grid = make_grid_overlay_for_view(view);
		overlays.push_back(make_grid_overlay_command(
				grid,
				static_cast<std::uint32_t>(grid_overlay_payloads.size())));
		grid_overlay_payloads.push_back(grid);
	}

	std::vector<RenderObject> objects;
	objects.push_back(make_prototype_cube(frame.elapsed_seconds, frame.animation_enabled));

	std::vector<RenderLight> lights;
	lights.push_back(RenderLight{});

	std::vector<PickingRequest> picking_requests(frame.picking_requests.begin(), frame.picking_requests.end());

	// Preserve the non-physical prototype cube lighting until a later task gives the fixture physical luminance.
	ViewportSettings viewport_settings;
	viewport_settings.exposure_ev100 = 0.0F;

	return quader::foundation::Result<FrameSnapshot, RendererDiagnostic>::success(FrameSnapshot{
			0,
			frame.elapsed_seconds,
			frame.target_extent,
			std::move(views),
			std::move(objects),
			std::move(lights),
			RenderEnvironment{},
			std::move(overlays),
			std::move(grid_overlay_payloads),
			std::move(picking_requests),
			viewport_settings,
			DebugViewMode::FinalColor,
	});
}

} // namespace crimson
