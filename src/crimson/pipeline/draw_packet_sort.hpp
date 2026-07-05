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

#include "crimson/pipeline/draw_packet.hpp"

#include <span>

namespace crimson {

/**
 * Sort opaque draw packets for deterministic front-end submission.
 *
 * @param[in, out] packets Packets sorted in place.
 */
void sort_opaque_draw_packets(std::span<DrawPacket> packets);
/**
 * Sort transparent draw packets for back-to-front submission.
 *
 * @param[in, out] packets Packets sorted in place.
 */
void sort_transparent_draw_packets(std::span<DrawPacket> packets);
/**
 * Compare opaque packets for queue/material/mesh-oriented sorting.
 *
 * @param left First packet.
 * @param right Second packet.
 * @return True when `left` should sort before `right`.
 */
[[nodiscard]] bool opaque_draw_packet_less(const DrawPacket &left, const DrawPacket &right) noexcept;
/**
 * Compare transparent packets for back-to-front sorting.
 *
 * @param left First packet.
 * @param right Second packet.
 * @return True when `left` should sort before `right`.
 */
[[nodiscard]] bool transparent_draw_packet_less(const DrawPacket &left, const DrawPacket &right) noexcept;

} // namespace crimson
