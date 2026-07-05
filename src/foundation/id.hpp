#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <limits>

namespace quader::foundation {

using IdIndex = std::uint32_t;
using IdGeneration = std::uint32_t;

constexpr IdIndex kInvalidIdIndex = std::numeric_limits<IdIndex>::max();

template <class Tag>
class Id final {
public:
	constexpr Id() noexcept = default;
	explicit constexpr Id(IdIndex index) noexcept
			: index_(index) {
	}

	[[nodiscard]] static constexpr Id invalid() noexcept {
		return Id{};
	}

	[[nodiscard]] constexpr bool is_valid() const noexcept {
		return index_ != kInvalidIdIndex;
	}

	[[nodiscard]] constexpr IdIndex index() const noexcept {
		return index_;
	}

	friend constexpr auto operator<=>(Id, Id) noexcept = default;

private:
	IdIndex index_ = kInvalidIdIndex;
};

template <class Tag>
class GenerationalId final {
public:
	constexpr GenerationalId() noexcept = default;
	constexpr GenerationalId(IdIndex index, IdGeneration generation) noexcept
			: index_(index),
			  generation_(generation) {
	}

	[[nodiscard]] static constexpr GenerationalId invalid() noexcept {
		return GenerationalId{};
	}

	[[nodiscard]] constexpr bool is_valid() const noexcept {
		return index_ != kInvalidIdIndex;
	}

	[[nodiscard]] constexpr IdIndex index() const noexcept {
		return index_;
	}

	[[nodiscard]] constexpr IdGeneration generation() const noexcept {
		return generation_;
	}

	friend constexpr auto operator<=>(GenerationalId, GenerationalId) noexcept = default;

private:
	IdIndex index_ = kInvalidIdIndex;
	IdGeneration generation_ = 0;
};

} // namespace quader::foundation

namespace std {

template <class Tag>
struct hash<quader::foundation::Id<Tag>> {
	size_t operator()(quader::foundation::Id<Tag> id) const noexcept {
		return hash<quader::foundation::IdIndex>{}(id.index());
	}
};

template <class Tag>
struct hash<quader::foundation::GenerationalId<Tag>> {
	size_t operator()(quader::foundation::GenerationalId<Tag> id) const noexcept {
		const auto kIndexHash = hash<quader::foundation::IdIndex>{}(id.index());
		const auto kGenerationHash = hash<quader::foundation::IdGeneration>{}(id.generation());
		return kIndexHash ^ (kGenerationHash + 0x9e3779b9U + (kIndexHash << 6U) + (kIndexHash >> 2U));
	}
};

} // namespace std
