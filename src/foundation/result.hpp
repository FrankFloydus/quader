#pragma once

#include "foundation/assert.hpp"

#include <type_traits>
#include <utility>
#include <variant>

namespace quader::foundation {

template <class T, class E>
class Result final {
	static_assert(!std::is_void_v<T>, "Use Result<void, E> for void success values.");
	static_assert(!std::is_void_v<E>, "Result error type must not be void.");
	static_assert(!std::is_reference_v<T>, "Result value type must not be a reference.");
	static_assert(!std::is_reference_v<E>, "Result error type must not be a reference.");

public:
	[[nodiscard]] static Result success(T value) {
		return Result(ValueStorage{ std::move(value) });
	}

	[[nodiscard]] static Result failure(E error) {
		return Result(ErrorStorage{ std::move(error) });
	}

	[[nodiscard]] bool has_value() const noexcept {
		return std::holds_alternative<ValueStorage>(storage_);
	}

	[[nodiscard]] explicit operator bool() const noexcept {
		return has_value();
	}

	[[nodiscard]] T &value() & {
		auto *storage = std::get_if<ValueStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->value;
	}

	[[nodiscard]] const T &value() const & {
		const auto *storage = std::get_if<ValueStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->value;
	}

	[[nodiscard]] T &&value() && {
		auto *storage = std::get_if<ValueStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return std::move(storage->value);
	}

	[[nodiscard]] E &error() & {
		auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->error;
	}

	[[nodiscard]] const E &error() const & {
		const auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->error;
	}

	[[nodiscard]] E &&error() && {
		auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return std::move(storage->error);
	}

private:
	struct ValueStorage {
		T value;
	};

	struct ErrorStorage {
		E error;
	};

	explicit Result(ValueStorage value) : storage_(std::move(value)) {
	}

	explicit Result(ErrorStorage error) : storage_(std::move(error)) {
	}

	std::variant<ValueStorage, ErrorStorage> storage_;
};

template <class E>
class Result<void, E> final {
	static_assert(!std::is_void_v<E>, "Result error type must not be void.");
	static_assert(!std::is_reference_v<E>, "Result error type must not be a reference.");

public:
	[[nodiscard]] static Result success() {
		return Result(SuccessStorage{});
	}

	[[nodiscard]] static Result failure(E error) {
		return Result(ErrorStorage{ std::move(error) });
	}

	[[nodiscard]] bool has_value() const noexcept {
		return std::holds_alternative<SuccessStorage>(storage_);
	}

	[[nodiscard]] explicit operator bool() const noexcept {
		return has_value();
	}

	void value() const {
		QUADER_ASSERT(has_value());
	}

	[[nodiscard]] E &error() & {
		auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->error;
	}

	[[nodiscard]] const E &error() const & {
		const auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->error;
	}

	[[nodiscard]] E &&error() && {
		auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return std::move(storage->error);
	}

private:
	struct SuccessStorage {
	};

	struct ErrorStorage {
		E error;
	};

	explicit Result(SuccessStorage success) : storage_(success) {
	}

	explicit Result(ErrorStorage error) : storage_(std::move(error)) {
	}

	std::variant<SuccessStorage, ErrorStorage> storage_;
};

} // namespace quader::foundation
