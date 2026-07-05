#pragma once

#include "crimson/scene/render_object.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace crimson {

using PickingRequestId = std::uint64_t;

enum class PickingElementKind : std::uint8_t {
	None,
	Object,
	Submesh,
	Face,
	Edge,
	Vertex,
};

struct PickingPayload {
	RenderObjectId object_id = 0;
	std::uint32_t submesh_index = 0;
	PickingElementKind element_kind = PickingElementKind::None;
	std::uint32_t element_index = 0;
};

struct PickingRequest {
	PickingRequestId request_id = 0;
	std::uint32_t view_index = 0;
	std::uint16_t x_px = 0;
	std::uint16_t y_px = 0;
};

struct PickingResult {
	PickingRequestId request_id = 0;
	bool hit = false;
	PickingPayload payload;
};

struct PackedPickingIdRgba8 {
	std::array<std::uint8_t, 4> bytes{};
};

[[nodiscard]] PackedPickingIdRgba8 encode_picking_id_rgba8(std::uint32_t raw_id) noexcept;
[[nodiscard]] std::uint32_t decode_picking_id_rgba8(PackedPickingIdRgba8 encoded) noexcept;
[[nodiscard]] std::array<float, 4> picking_id_rgba8_to_unorm(PackedPickingIdRgba8 encoded) noexcept;
[[nodiscard]] bool picking_readback_requested(std::span<const PickingRequest> requests) noexcept;

class PickingIdAllocator final {
public:
	explicit PickingIdAllocator(PickingRequestId request_id = 0);

	void begin_request(PickingRequestId request_id);

	[[nodiscard]] PickingRequestId request_id() const noexcept;
	[[nodiscard]] std::uint32_t allocate(PickingPayload payload);
	[[nodiscard]] std::optional<PickingPayload> resolve(std::uint32_t raw_id) const;
	[[nodiscard]] std::span<const PickingPayload> payloads() const noexcept;

private:
	PickingRequestId request_id_ = 0;
	std::vector<PickingPayload> payloads_;
};

[[nodiscard]] PickingPayload picking_payload_for_object(const RenderObject &object) noexcept;

} // namespace crimson
