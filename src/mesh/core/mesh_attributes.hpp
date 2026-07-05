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

#include "foundation/id.hpp"
#include "foundation/result.hpp"
#include "math/vec2.hpp"
#include "math/vec3.hpp"
#include "mesh/core/mesh_error.hpp"
#include "mesh/core/mesh_ids.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace quader::mesh {

/// Mesh element domain that owns an attribute array.
enum class AttributeDomain {
	/// One value per vertex slot.
	Vertex,
	/// One value per halfedge slot.
	Halfedge,
	/// One value per edge slot.
	Edge,
	/// One value per face slot.
	Face,
};

/// Runtime value kind stored by an attribute array.
enum class AttributeValueType {
	/// Single `float` value.
	Float32,
	/// `Vec2` single-precision value.
	Vec2Float32,
	/// `Vec3` single-precision value.
	Vec3Float32,
	/// Unsigned 32-bit integer value.
	UInt32,
};

/// Descriptor for one mesh attribute array.
struct AttributeDescriptor {
	/// Stable attribute identifier.
	AttributeId id;
	/// Mesh element domain indexed by this attribute.
	AttributeDomain domain = AttributeDomain::Vertex;
	/// Stored value type.
	AttributeValueType value_type = AttributeValueType::Float32;
	/// Unique attribute name within `domain`.
	std::string name;
};

/**
 * Compile-time mapping from C++ attribute value types to runtime descriptors.
 *
 * @tparam T Candidate attribute value type.
 */
template <class T>
struct AttributeValueTraits {
	/// False for unsupported attribute value types.
	static constexpr bool kSupported = false;
};

/// Attribute value traits for `float`.
template <>
struct AttributeValueTraits<float> {
	/// `float` is supported as an attribute value.
	static constexpr bool kSupported = true;
	/// Runtime descriptor value type for `float`.
	static constexpr AttributeValueType kValueType = AttributeValueType::Float32;
};

/// Attribute value traits for `Vec2`.
template <>
struct AttributeValueTraits<quader::math::Vec2> {
	/// `Vec2` is supported as an attribute value.
	static constexpr bool kSupported = true;
	/// Runtime descriptor value type for `Vec2`.
	static constexpr AttributeValueType kValueType = AttributeValueType::Vec2Float32;
};

/// Attribute value traits for `Vec3`.
template <>
struct AttributeValueTraits<quader::math::Vec3> {
	/// `Vec3` is supported as an attribute value.
	static constexpr bool kSupported = true;
	/// Runtime descriptor value type for `Vec3`.
	static constexpr AttributeValueType kValueType = AttributeValueType::Vec3Float32;
};

/// Attribute value traits for `std::uint32_t`.
template <>
struct AttributeValueTraits<std::uint32_t> {
	/// `std::uint32_t` is supported as an attribute value.
	static constexpr bool kSupported = true;
	/// Runtime descriptor value type for `std::uint32_t`.
	static constexpr AttributeValueType kValueType = AttributeValueType::UInt32;
};

/**
 * Owns typed attribute arrays for mesh element domains.
 *
 * Attribute arrays are resized by domain and keep a default value used for new
 * slots or slot reset. Attribute IDs are index-only and remain valid while
 * their storage slot exists.
 */
class MeshAttributes final {
public:
	/// Create an empty attribute collection.
	MeshAttributes() = default;

	/**
	 * Deep-copy all attribute storage.
	 *
	 * @param other Attribute collection to copy.
	 */
	MeshAttributes(const MeshAttributes &other) : domain_sizes_(other.domain_sizes_) {
		storages_.reserve(other.storages_.size());
		for (const auto &storage : other.storages_) {
			storages_.push_back(storage->clone());
		}
	}

	/**
	 * Deep-copy all attribute storage.
	 *
	 * @param other Attribute collection to copy.
	 * @return This collection.
	 */
	MeshAttributes &operator=(const MeshAttributes &other) {
		if (this == &other) {
			return *this;
		}

		std::vector<std::unique_ptr<AttributeStorage>> storages;
		storages.reserve(other.storages_.size());
		for (const auto &storage : other.storages_) {
			storages.push_back(storage->clone());
		}

		domain_sizes_ = other.domain_sizes_;
		storages_ = std::move(storages);
		return *this;
	}

	/// Move an attribute collection.
	MeshAttributes(MeshAttributes &&) noexcept = default;
	/// Move-assign an attribute collection.
	MeshAttributes &operator=(MeshAttributes &&) noexcept = default;

	/**
	 * Create a typed attribute array.
	 *
	 * @tparam T Supported attribute value type.
	 * @param domain Mesh element domain indexed by the attribute.
	 * @param name Name unique within `domain`.
	 * @param default_value Value used for all current and future slots.
	 * @return Attribute id, or an error when the name already exists.
	 */
	template <class T>
	[[nodiscard]] quader::foundation::Result<AttributeId, MeshError> create(AttributeDomain domain,
			std::string name,
			T default_value) {
		static_assert(AttributeValueTraits<T>::kSupported, "Unsupported mesh attribute value type.");

		if (has_attribute(domain, name)) {
			return quader::foundation::Result<AttributeId, MeshError>::failure(
					make_mesh_error(MeshErrorCode::AttributeNameAlreadyExists,
							"mesh attribute name already exists in the requested domain"));
		}

		const AttributeId kId{ static_cast<quader::foundation::IdIndex>(storages_.size()) };
		auto storage = std::make_unique<TypedAttributeStorage<T>>(
				AttributeDescriptor{ kId, domain, AttributeValueTraits<T>::kValueType, std::move(name) },
				std::move(default_value),
				domain_size(domain));
		storages_.push_back(std::move(storage));

		return quader::foundation::Result<AttributeId, MeshError>::success(kId);
	}

	/**
	 * Return the number of attribute arrays.
	 *
	 * @return Attribute storage count.
	 */
	[[nodiscard]] std::size_t attribute_count() const noexcept {
		return storages_.size();
	}

	/**
	 * Find an attribute descriptor.
	 *
	 * @param id Attribute id to resolve.
	 * @return Pointer valid until mutation, or `nullptr` for invalid ids.
	 */
	[[nodiscard]] const AttributeDescriptor *descriptor(AttributeId id) const noexcept {
		if (!is_valid_attribute_id(id)) {
			return nullptr;
		}

		return &storages_[id.index()]->descriptor();
	}

	/**
	 * Access a mutable attribute slot.
	 *
	 * @tparam T Expected attribute value type.
	 * @param id Attribute id to resolve.
	 * @param slot Domain slot index.
	 * @return Pointer valid until attribute storage mutation, or an error.
	 */
	template <class T>
	[[nodiscard]] quader::foundation::Result<T *, MeshError> value(AttributeId id,
			quader::foundation::IdIndex slot) {
		auto *storage = typed_storage<T>(id);
		if (storage == nullptr) {
			return quader::foundation::Result<T *, MeshError>::failure(
					make_mesh_error(MeshErrorCode::AttributeTypeMismatch,
							"mesh attribute type does not match requested value type"));
		}
		if (slot >= storage->values().size()) {
			return quader::foundation::Result<T *, MeshError>::failure(
					make_mesh_error(MeshErrorCode::AttributeIndexOutOfRange,
							"mesh attribute slot is outside the attribute array"));
		}

		return quader::foundation::Result<T *, MeshError>::success(&storage->values()[slot]);
	}

	/**
	 * Access an immutable attribute slot.
	 *
	 * @tparam T Expected attribute value type.
	 * @param id Attribute id to resolve.
	 * @param slot Domain slot index.
	 * @return Pointer valid until attribute storage mutation, or an error.
	 */
	template <class T>
	[[nodiscard]] quader::foundation::Result<const T *, MeshError> value(AttributeId id,
			quader::foundation::IdIndex slot) const {
		const auto *storage = typed_storage<T>(id);
		if (storage == nullptr) {
			return quader::foundation::Result<const T *, MeshError>::failure(
					make_mesh_error(MeshErrorCode::AttributeTypeMismatch,
							"mesh attribute type does not match requested value type"));
		}
		if (slot >= storage->values().size()) {
			return quader::foundation::Result<const T *, MeshError>::failure(
					make_mesh_error(MeshErrorCode::AttributeIndexOutOfRange,
							"mesh attribute slot is outside the attribute array"));
		}

		return quader::foundation::Result<const T *, MeshError>::success(&storage->values()[slot]);
	}

	/**
	 * Resize all attribute arrays for a domain.
	 *
	 * @param domain Domain whose arrays are resized.
	 * @param size New slot count.
	 */
	void resize_domain(AttributeDomain domain, std::size_t size) {
		domain_sizes_[domain_index(domain)] = size;
		for (auto &storage : storages_) {
			if (storage->descriptor().domain == domain) {
				storage->resize(size);
			}
		}
	}

	/**
	 * Reset a domain slot to each attribute array's default value.
	 *
	 * @param domain Domain containing `slot`.
	 * @param slot Slot index to reset when present.
	 */
	void reset_slot_to_defaults(AttributeDomain domain, quader::foundation::IdIndex slot) {
		for (auto &storage : storages_) {
			if (storage->descriptor().domain == domain) {
				storage->reset(slot);
			}
		}
	}

private:
	class AttributeStorage {
	public:
		explicit AttributeStorage(AttributeDescriptor descriptor) : descriptor_(std::move(descriptor)) {
		}

		virtual ~AttributeStorage() = default;

		[[nodiscard]] virtual std::unique_ptr<AttributeStorage> clone() const = 0;

		[[nodiscard]] const AttributeDescriptor &descriptor() const noexcept {
			return descriptor_;
		}

		virtual void resize(std::size_t size) = 0;
		virtual void reset(quader::foundation::IdIndex slot) = 0;

	private:
		AttributeDescriptor descriptor_;
	};

	template <class T>
	class TypedAttributeStorage final : public AttributeStorage {
	public:
		TypedAttributeStorage(AttributeDescriptor descriptor, T default_value, std::size_t size) : AttributeStorage(std::move(descriptor)), default_value_(std::move(default_value)), values_(size, default_value_) {
		}

		[[nodiscard]] std::unique_ptr<AttributeStorage> clone() const override {
			return std::make_unique<TypedAttributeStorage<T>>(*this);
		}

		void resize(std::size_t size) override {
			values_.resize(size, default_value_);
		}

		void reset(quader::foundation::IdIndex slot) override {
			if (slot < values_.size()) {
				values_[slot] = default_value_;
			}
		}

		[[nodiscard]] std::vector<T> &values() noexcept {
			return values_;
		}

		[[nodiscard]] const std::vector<T> &values() const noexcept {
			return values_;
		}

	private:
		T default_value_;
		std::vector<T> values_;
	};

	[[nodiscard]] static constexpr std::size_t domain_index(AttributeDomain domain) noexcept {
		return static_cast<std::size_t>(domain);
	}

	[[nodiscard]] std::size_t domain_size(AttributeDomain domain) const noexcept {
		return domain_sizes_[domain_index(domain)];
	}

	[[nodiscard]] bool is_valid_attribute_id(AttributeId id) const noexcept {
		return id.is_valid() && id.index() < storages_.size();
	}

	[[nodiscard]] bool has_attribute(AttributeDomain domain, std::string_view name) const noexcept {
		for (const auto &storage : storages_) {
			const auto &descriptor = storage->descriptor();
			if (descriptor.domain == domain && descriptor.name == name) {
				return true;
			}
		}

		return false;
	}

	template <class T>
	[[nodiscard]] TypedAttributeStorage<T> *typed_storage(AttributeId id) noexcept {
		if (!is_valid_attribute_id(id)) {
			return nullptr;
		}

		return dynamic_cast<TypedAttributeStorage<T> *>(storages_[id.index()].get());
	}

	template <class T>
	[[nodiscard]] const TypedAttributeStorage<T> *typed_storage(AttributeId id) const noexcept {
		if (!is_valid_attribute_id(id)) {
			return nullptr;
		}

		return dynamic_cast<const TypedAttributeStorage<T> *>(storages_[id.index()].get());
	}

	std::array<std::size_t, 4> domain_sizes_{};
	std::vector<std::unique_ptr<AttributeStorage>> storages_;
};

} // namespace quader::mesh
