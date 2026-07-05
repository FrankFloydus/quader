/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include "crimson/scene/render_object.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace crimson {

/// Stable id used to match asynchronous picking readbacks to UI requests.
using PickingRequestId = std::uint64_t;

/// Kind of renderer payload stored in a picking id.
enum class PickingElementKind : std::uint8_t {
	None,    ///< No payload.
	Object,  ///< Object-level hit.
	Submesh, ///< Submesh-level hit.
	Face,    ///< Mesh face hit.
	Edge,    ///< Mesh edge hit.
	Vertex,  ///< Mesh vertex hit.
};

/// Decoded picking payload associated with a submitted raw id.
struct PickingPayload {
	/// Stable renderer object id.
	RenderObjectId object_id = 0;
	/// Submesh index within the object.
	std::uint32_t submesh_index = 0;
	/// Element category.
	PickingElementKind element_kind = PickingElementKind::None;
	/// Element index for component hits.
	std::uint32_t element_index = 0;
};

/// One viewport pixel readback request.
struct PickingRequest {
	/// Request id returned with the result.
	PickingRequestId request_id = 0;
	/// View index containing the pixel.
	std::uint32_t view_index = 0;
	/// X coordinate in physical pixels.
	std::uint16_t x_px = 0;
	/// Y coordinate in physical pixels.
	std::uint16_t y_px = 0;
};

/// Completed picking request result.
struct PickingResult {
	/// Request id copied from the original request.
	PickingRequestId request_id = 0;
	/// True when a payload id was read at the request pixel.
	bool hit = false;
	/// Decoded payload when `hit` is true.
	PickingPayload payload;
};

/// RGBA8 representation of a raw picking id.
struct PackedPickingIdRgba8 {
	/// Four bytes storing the encoded id.
	std::array<std::uint8_t, 4> bytes{};
};

/// Encode a raw picking id into RGBA8 bytes.
[[nodiscard]] PackedPickingIdRgba8 encode_picking_id_rgba8(std::uint32_t raw_id) noexcept;
/// Decode RGBA8 bytes into a raw picking id.
[[nodiscard]] std::uint32_t decode_picking_id_rgba8(PackedPickingIdRgba8 encoded) noexcept;
/// Convert encoded RGBA8 bytes to normalized float channels for shader uploads.
[[nodiscard]] std::array<float, 4> picking_id_rgba8_to_unorm(PackedPickingIdRgba8 encoded) noexcept;
/// Return true when at least one picking request needs readback.
[[nodiscard]] bool picking_readback_requested(std::span<const PickingRequest> requests) noexcept;
/// Return true when a picking kind references document topology components.
[[nodiscard]] bool picking_element_kind_is_component(PickingElementKind kind) noexcept;
/// Return true when a payload must be resolved and validated against document topology outside Crimson.
[[nodiscard]] bool picking_payload_requires_external_resolution(PickingPayload payload) noexcept;
/// Build a typed component picking payload, rejecting object-level and empty kinds.
[[nodiscard]] std::optional<PickingPayload> make_component_picking_payload(
		RenderObjectId object_id,
		PickingElementKind kind,
		std::uint32_t element_index,
		std::uint32_t submesh_index = 0) noexcept;

/**
 * Allocates per-frame raw picking ids and resolves them back to payloads.
 *
 * Raw id zero is reserved for "no hit"; allocated ids are one-based indexes
 * into payload storage.
 */
class PickingIdAllocator final {
public:
	/// Construct an allocator for a request id.
	explicit PickingIdAllocator(PickingRequestId request_id = 0);

	/// Clear payload storage and begin a new request.
	void begin_request(PickingRequestId request_id);

	/// Return the request id associated with current allocations.
	[[nodiscard]] PickingRequestId request_id() const noexcept;
	/// Store a payload and return its non-zero raw id.
	[[nodiscard]] std::uint32_t allocate(PickingPayload payload);
	/// Resolve a raw id, returning empty for zero or out-of-range ids.
	[[nodiscard]] std::optional<PickingPayload> resolve(std::uint32_t raw_id) const;
	/// Borrow allocated payloads until the allocator is reset or destroyed.
	[[nodiscard]] std::span<const PickingPayload> payloads() const noexcept;

private:
	PickingRequestId request_id_ = 0;
	std::vector<PickingPayload> payloads_;
};

/// Build the default object-level picking payload for a render object.
[[nodiscard]] PickingPayload picking_payload_for_object(const RenderObject &object) noexcept;

} // namespace crimson
