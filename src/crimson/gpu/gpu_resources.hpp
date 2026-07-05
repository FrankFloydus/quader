#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "crimson/renderer_types.hpp"

namespace crimson::gpu {

template <typename Handle, typename Traits>
class UniqueGpuHandle final {
public:
	UniqueGpuHandle() noexcept = default;

	explicit UniqueGpuHandle(Handle handle) noexcept
			: handle_(handle) {
	}

	~UniqueGpuHandle() {
		reset();
	}

	UniqueGpuHandle(const UniqueGpuHandle &) = delete;
	UniqueGpuHandle &operator=(const UniqueGpuHandle &) = delete;

	UniqueGpuHandle(UniqueGpuHandle &&other) noexcept
			: handle_(other.release()) {
	}

	UniqueGpuHandle &operator=(UniqueGpuHandle &&other) noexcept {
		if (this != &other) {
			reset(other.release());
		}

		return *this;
	}

	[[nodiscard]] bool valid() const noexcept {
		return Traits::is_valid(handle_);
	}

	[[nodiscard]] Handle get() const noexcept {
		return handle_;
	}

	[[nodiscard]] Handle release() noexcept {
		Handle released = handle_;
		handle_ = Traits::invalid();
		return released;
	}

	void reset(Handle handle = Traits::invalid()) noexcept {
		if (Traits::is_valid(handle_)) {
			Traits::destroy(handle_);
		}
		handle_ = handle;
	}

private:
	Handle handle_ = Traits::invalid();
};

template <typename Resource, typename PublicHandle>
class GpuResourceTable final {
public:
	PublicHandle create(Resource resource) {
		std::uint32_t index = 0;
		if (!free_indices_.empty()) {
			index = free_indices_.back();
			free_indices_.pop_back();

			Slot &slot = slots_[static_cast<std::size_t>(index - 1)];
			slot.resource = std::move(resource);
			slot.occupied = true;
		} else {
			index = static_cast<std::uint32_t>(slots_.size() + 1);
			slots_.push_back(Slot{
					.resource = std::move(resource),
					.generation = 1,
					.occupied = true,
			});
		}

		++live_count_;
		const Slot &slot = slots_[static_cast<std::size_t>(index - 1)];
		return PublicHandle{ index, slot.generation };
	}

	Resource *get(PublicHandle handle) noexcept {
		Slot *slot = slot_for(handle);
		return slot == nullptr ? nullptr : &slot->resource;
	}

	const Resource *get(PublicHandle handle) const noexcept {
		const Slot *slot = slot_for(handle);
		return slot == nullptr ? nullptr : &slot->resource;
	}

	bool destroy(PublicHandle handle) noexcept {
		Slot *slot = slot_for(handle);
		if (slot == nullptr) {
			return false;
		}

		slot->resource = Resource{};
		slot->occupied = false;
		slot->generation = next_generation(slot->generation);
		free_indices_.push_back(handle.index);
		--live_count_;
		return true;
	}

	void clear() noexcept {
		slots_.clear();
		free_indices_.clear();
		live_count_ = 0;
	}

	std::size_t live_count() const noexcept {
		return live_count_;
	}

private:
	struct Slot {
		Resource resource{};
		std::uint32_t generation = 1;
		bool occupied = false;
	};

	static std::uint32_t next_generation(std::uint32_t generation) noexcept {
		++generation;
		if (generation == 0) {
			generation = 1;
		}
		return generation;
	}

	Slot *slot_for(PublicHandle handle) noexcept {
		if (!is_valid_handle(handle) || handle.index > slots_.size()) {
			return nullptr;
		}

		Slot &slot = slots_[static_cast<std::size_t>(handle.index - 1)];
		if (!slot.occupied || slot.generation != handle.generation) {
			return nullptr;
		}

		return &slot;
	}

	const Slot *slot_for(PublicHandle handle) const noexcept {
		if (!is_valid_handle(handle) || handle.index > slots_.size()) {
			return nullptr;
		}

		const Slot &slot = slots_[static_cast<std::size_t>(handle.index - 1)];
		if (!slot.occupied || slot.generation != handle.generation) {
			return nullptr;
		}

		return &slot;
	}

	std::vector<Slot> slots_;
	std::vector<std::uint32_t> free_indices_;
	std::size_t live_count_ = 0;
};

} // namespace crimson::gpu
