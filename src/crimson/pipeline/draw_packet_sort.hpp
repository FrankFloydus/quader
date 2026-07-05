#pragma once

#include "crimson/pipeline/draw_packet.hpp"

#include <span>

namespace crimson {

void sort_opaque_draw_packets(std::span<DrawPacket> packets);
void sort_transparent_draw_packets(std::span<DrawPacket> packets);
[[nodiscard]] bool opaque_draw_packet_less(const DrawPacket& left, const DrawPacket& right) noexcept;
[[nodiscard]] bool transparent_draw_packet_less(const DrawPacket& left, const DrawPacket& right) noexcept;

} // namespace crimson
