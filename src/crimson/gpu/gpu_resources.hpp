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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "crimson/renderer_types.hpp"

namespace crimson::gpu {

/**
 * Move-only RAII wrapper for a native GPU handle.
 *
 * @tparam Handle Native handle type.
 * @tparam Traits Traits providing invalid, validity, and destroy operations.
 */
template <typename Handle, typename Traits>
class UniqueGpuHandle final {
public:
	/// Create an invalid handle wrapper.
	UniqueGpuHandle() noexcept = default;

	/**
	 * Take ownership of a native handle.
	 *
	 * @param handle Native handle to own.
	 */
	explicit UniqueGpuHandle(Handle handle) noexcept
			: handle_(handle) {
	}

	/// Destroy the owned handle when valid.
	~UniqueGpuHandle() {
		reset();
	}

	/// Unique GPU handles cannot be copied.
	UniqueGpuHandle(const UniqueGpuHandle &) = delete;
	/// Unique GPU handles cannot be copied.
	UniqueGpuHandle &operator=(const UniqueGpuHandle &) = delete;

	/// Move ownership from another wrapper.
	UniqueGpuHandle(UniqueGpuHandle &&other) noexcept
			: handle_(other.release()) {
	}

	/// Move ownership from another wrapper.
	UniqueGpuHandle &operator=(UniqueGpuHandle &&other) noexcept {
		if (this != &other) {
			reset(other.release());
		}

		return *this;
	}

	/**
	 * Check whether the owned native handle is valid.
	 *
	 * @return True when `Traits::is_valid()` accepts the handle.
	 */
	[[nodiscard]] bool valid() const noexcept {
		return Traits::is_valid(handle_);
	}

	/**
	 * Return the native handle without transferring ownership.
	 *
	 * @return Owned native handle, possibly invalid.
	 */
	[[nodiscard]] Handle get() const noexcept {
		return handle_;
	}

	/**
	 * Release ownership without destroying the handle.
	 *
	 * @return Previously owned native handle.
	 */
	[[nodiscard]] Handle release() noexcept {
		Handle released = handle_;
		handle_ = Traits::invalid();
		return released;
	}

	/**
	 * Replace the owned handle.
	 *
	 * @param handle New native handle to own.
	 */
	void reset(Handle handle = Traits::invalid()) noexcept {
		if (Traits::is_valid(handle_)) {
			Traits::destroy(handle_);
		}
		handle_ = handle;
	}

private:
	Handle handle_ = Traits::invalid();
};

/**
 * Generation-checked resource table for public renderer handles.
 *
 * @tparam Resource Resource type stored in slots.
 * @tparam PublicHandle Public handle type with `index` and `generation`.
 */
template <typename Resource, typename PublicHandle>
class GpuResourceTable final {
public:
	/**
	 * Store a resource and return its public handle.
	 *
	 * @param resource Resource to move into the table.
	 * @return Generation-checked handle for the stored resource.
	 */
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

	/**
	 * Store or replace a resource at a caller-owned public handle.
	 *
	 * @param handle Generation-checked handle that should resolve after the call.
	 * @param resource Resource to move into the table.
	 * @return True when the handle was valid and the slot was written.
	 */
	bool upsert(PublicHandle handle, Resource resource) {
		if (!is_valid_handle(handle)) {
			return false;
		}

		if (handle.index > slots_.size()) {
			slots_.resize(handle.index);
		}

		Slot &slot = slots_[static_cast<std::size_t>(handle.index - 1)];
		if (!slot.occupied) {
			++live_count_;
		}

		slot.resource = std::move(resource);
		slot.generation = handle.generation;
		slot.occupied = true;

		const auto kFree = std::find(free_indices_.begin(), free_indices_.end(), handle.index);
		if (kFree != free_indices_.end()) {
			free_indices_.erase(kFree);
		}
		return true;
	}

	/**
	 * Resolve a mutable resource.
	 *
	 * @param handle Public handle to resolve.
	 * @return Resource pointer, or `nullptr` for invalid/stale handles.
	 */
	Resource *get(PublicHandle handle) noexcept {
		Slot *slot = slot_for(handle);
		return slot == nullptr ? nullptr : &slot->resource;
	}

	/**
	 * Resolve an immutable resource.
	 *
	 * @param handle Public handle to resolve.
	 * @return Resource pointer, or `nullptr` for invalid/stale handles.
	 */
	const Resource *get(PublicHandle handle) const noexcept {
		const Slot *slot = slot_for(handle);
		return slot == nullptr ? nullptr : &slot->resource;
	}

	/**
	 * Destroy a resource slot.
	 *
	 * @param handle Public handle to destroy.
	 * @return True when a live slot was destroyed.
	 */
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

	/// Destroy all table slots and invalidate outstanding handles.
	void clear() noexcept {
		slots_.clear();
		free_indices_.clear();
		live_count_ = 0;
	}

	/**
	 * Return the number of occupied slots.
	 *
	 * @return Live resource count.
	 */
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
