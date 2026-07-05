#include "crimson/picking/picking.hpp"

#include <cstddef>
#include <limits>

namespace crimson {

PackedPickingIdRgba8 encode_picking_id_rgba8(std::uint32_t raw_id) noexcept
{
    return PackedPickingIdRgba8{{
        static_cast<std::uint8_t>(raw_id & 0xffU),
        static_cast<std::uint8_t>((raw_id >> 8U) & 0xffU),
        static_cast<std::uint8_t>((raw_id >> 16U) & 0xffU),
        static_cast<std::uint8_t>((raw_id >> 24U) & 0xffU),
    }};
}

std::uint32_t decode_picking_id_rgba8(PackedPickingIdRgba8 encoded) noexcept
{
    return static_cast<std::uint32_t>(encoded.bytes[0])
        | (static_cast<std::uint32_t>(encoded.bytes[1]) << 8U)
        | (static_cast<std::uint32_t>(encoded.bytes[2]) << 16U)
        | (static_cast<std::uint32_t>(encoded.bytes[3]) << 24U);
}

std::array<float, 4> picking_id_rgba8_to_unorm(PackedPickingIdRgba8 encoded) noexcept
{
    constexpr float kByteToUnorm = 1.0F / 255.0F;
    return {
        static_cast<float>(encoded.bytes[0]) * kByteToUnorm,
        static_cast<float>(encoded.bytes[1]) * kByteToUnorm,
        static_cast<float>(encoded.bytes[2]) * kByteToUnorm,
        static_cast<float>(encoded.bytes[3]) * kByteToUnorm,
    };
}

bool picking_readback_requested(std::span<const PickingRequest> requests) noexcept
{
    return !requests.empty();
}

PickingIdAllocator::PickingIdAllocator(PickingRequestId request_id)
    : request_id_(request_id)
{
}

void PickingIdAllocator::begin_request(PickingRequestId request_id)
{
    request_id_ = request_id;
    payloads_.clear();
}

PickingRequestId PickingIdAllocator::request_id() const noexcept
{
    return request_id_;
}

std::uint32_t PickingIdAllocator::allocate(PickingPayload payload)
{
    if (payloads_.size() >= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return 0;
    }

    payloads_.push_back(payload);
    return static_cast<std::uint32_t>(payloads_.size());
}

std::optional<PickingPayload> PickingIdAllocator::resolve(std::uint32_t raw_id) const
{
    if (raw_id == 0 || raw_id > payloads_.size()) {
        return std::nullopt;
    }

    return payloads_[static_cast<std::size_t>(raw_id - 1U)];
}

std::span<const PickingPayload> PickingIdAllocator::payloads() const noexcept
{
    return payloads_;
}

PickingPayload picking_payload_for_object(const RenderObject& object) noexcept
{
    return PickingPayload{
        .object_id = object.object_id,
        .submesh_index = object.submesh_index,
        .element_kind = PickingElementKind::Object,
        .element_index = 0,
    };
}

} // namespace crimson
