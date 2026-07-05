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

#include <compare>
#include <cstdint>
#include <functional>
#include <limits>

namespace quader::foundation {

/// Storage type used for index portions of project-owned handles.
using IdIndex = std::uint32_t;
/// Storage type used to invalidate stale generational handles after reuse.
using IdGeneration = std::uint32_t;

/// Sentinel index used by invalid handles.
constexpr IdIndex kInvalidIdIndex = std::numeric_limits<IdIndex>::max();

/**
 * Strong non-generational identifier for resources addressed by index.
 *
 * @tparam Tag Empty tag type that separates otherwise identical ID domains.
 */
template <class Tag>
class Id final {
public:
	/// Create an invalid identifier.
	constexpr Id() noexcept = default;
	/**
	 * Create an identifier for an existing slot.
	 *
	 * @param index Slot index represented by the identifier.
	 */
	explicit constexpr Id(IdIndex index) noexcept
			: index_(index) {
	}

	/**
	 * Create an invalid identifier.
	 *
	 * @return Identifier whose `is_valid()` result is false.
	 */
	[[nodiscard]] static constexpr Id invalid() noexcept {
		return Id{};
	}

	/**
	 * Check whether the identifier names a non-sentinel slot.
	 *
	 * @return True when the index is not `kInvalidIdIndex`.
	 */
	[[nodiscard]] constexpr bool is_valid() const noexcept {
		return index_ != kInvalidIdIndex;
	}

	/**
	 * Return the raw slot index.
	 *
	 * @return Slot index, or `kInvalidIdIndex` for invalid identifiers.
	 */
	[[nodiscard]] constexpr IdIndex index() const noexcept {
		return index_;
	}

	/// Compare identifiers by raw index.
	friend constexpr auto operator<=>(Id, Id) noexcept = default;

private:
	IdIndex index_ = kInvalidIdIndex;
};

/**
 * Strong identifier that pairs a slot index with a reuse generation.
 *
 * @tparam Tag Empty tag type that separates otherwise identical ID domains.
 */
template <class Tag>
class GenerationalId final {
public:
	/// Create an invalid identifier with generation zero.
	constexpr GenerationalId() noexcept = default;
	/**
	 * Create an identifier for a generated slot.
	 *
	 * @param index Slot index represented by the identifier.
	 * @param generation Slot generation used to detect stale references.
	 */
	constexpr GenerationalId(IdIndex index, IdGeneration generation) noexcept
			: index_(index),
			  generation_(generation) {
	}

	/**
	 * Create an invalid generational identifier.
	 *
	 * @return Identifier whose `is_valid()` result is false.
	 */
	[[nodiscard]] static constexpr GenerationalId invalid() noexcept {
		return GenerationalId{};
	}

	/**
	 * Check whether the identifier names a non-sentinel slot.
	 *
	 * @return True when the index is not `kInvalidIdIndex`.
	 */
	[[nodiscard]] constexpr bool is_valid() const noexcept {
		return index_ != kInvalidIdIndex;
	}

	/**
	 * Return the raw slot index.
	 *
	 * @return Slot index, or `kInvalidIdIndex` for invalid identifiers.
	 */
	[[nodiscard]] constexpr IdIndex index() const noexcept {
		return index_;
	}

	/**
	 * Return the slot generation.
	 *
	 * @return Generation value associated with this identifier.
	 */
	[[nodiscard]] constexpr IdGeneration generation() const noexcept {
		return generation_;
	}

	/// Compare identifiers by index and generation.
	friend constexpr auto operator<=>(GenerationalId, GenerationalId) noexcept = default;

private:
	IdIndex index_ = kInvalidIdIndex;
	IdGeneration generation_ = 0;
};

} // namespace quader::foundation

namespace std {

/// Hash specialization for Quader index-only identifiers.
template <class Tag>
struct hash<quader::foundation::Id<Tag>> {
	/**
	 * Hash an index-only identifier.
	 *
	 * @param id Identifier to hash.
	 * @return Hash value derived from the raw index.
	 */
	size_t operator()(quader::foundation::Id<Tag> id) const noexcept {
		return hash<quader::foundation::IdIndex>{}(id.index());
	}
};

/// Hash specialization for Quader generational identifiers.
template <class Tag>
struct hash<quader::foundation::GenerationalId<Tag>> {
	/**
	 * Hash a generational identifier.
	 *
	 * @param id Identifier to hash.
	 * @return Hash value derived from the raw index and generation.
	 */
	size_t operator()(quader::foundation::GenerationalId<Tag> id) const noexcept {
		const auto kIndexHash = hash<quader::foundation::IdIndex>{}(id.index());
		const auto kGenerationHash = hash<quader::foundation::IdGeneration>{}(id.generation());
		return kIndexHash ^ (kGenerationHash + 0x9e3779b9U + (kIndexHash << 6U) + (kIndexHash >> 2U));
	}
};

} // namespace std
