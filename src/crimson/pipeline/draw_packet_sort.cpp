#include "crimson/pipeline/draw_packet_sort.hpp"

#include <algorithm>
#include <tuple>

namespace crimson {

bool opaque_draw_packet_less(const DrawPacket &left, const DrawPacket &right) noexcept {
	return std::tuple{
		left.queue,
		left.program,
		left.base_shader,
		left.material.index,
		left.material.generation,
		left.mesh.index,
		left.mesh.generation,
		left.submesh_index,
		left.object_id,
	} < std::tuple{
		right.queue,
		right.program,
		right.base_shader,
		right.material.index,
		right.material.generation,
		right.mesh.index,
		right.mesh.generation,
		right.submesh_index,
		right.object_id,
	};
}

bool transparent_draw_packet_less(const DrawPacket &left, const DrawPacket &right) noexcept {
	if (left.camera_distance_sq != right.camera_distance_sq) {
		return left.camera_distance_sq > right.camera_distance_sq;
	}
	if (left.object_id != right.object_id) {
		return left.object_id < right.object_id;
	}
	return left.submesh_index < right.submesh_index;
}

void sort_opaque_draw_packets(std::span<DrawPacket> packets) {
	std::sort(packets.begin(), packets.end(), opaque_draw_packet_less);
}

void sort_transparent_draw_packets(std::span<DrawPacket> packets) {
	std::sort(packets.begin(), packets.end(), transparent_draw_packet_less);
}

} // namespace crimson
