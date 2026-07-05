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

#include "foundation/assert.hpp"

#include <type_traits>
#include <utility>
#include <variant>

namespace quader::foundation {

/**
 * Tagged success-or-error result for operations with a value payload.
 *
 * `Result` stores either a `T` success value or an `E` error value. Calling
 * `value()` or `error()` for the inactive state is a programmer error and is
 * checked with `QUADER_ASSERT`.
 *
 * @tparam T Success value type.
 * @tparam E Error value type.
 */
template <class T, class E>
class Result final {
	static_assert(!std::is_void_v<T>, "Use Result<void, E> for void success values.");
	static_assert(!std::is_void_v<E>, "Result error type must not be void.");
	static_assert(!std::is_reference_v<T>, "Result value type must not be a reference.");
	static_assert(!std::is_reference_v<E>, "Result error type must not be a reference.");

public:
	/**
	 * Create a successful result.
	 *
	 * @param value Value stored in the success state.
	 * @return Result containing `value`.
	 */
	[[nodiscard]] static Result success(T value) {
		return Result(ValueStorage{ std::move(value) });
	}

	/**
	 * Create a failed result.
	 *
	 * @param error Error stored in the failure state.
	 * @return Result containing `error`.
	 */
	[[nodiscard]] static Result failure(E error) {
		return Result(ErrorStorage{ std::move(error) });
	}

	/**
	 * Check whether this result is successful.
	 *
	 * @return True when a `T` value is active.
	 */
	[[nodiscard]] bool has_value() const noexcept {
		return std::holds_alternative<ValueStorage>(storage_);
	}

	/**
	 * Check whether this result is successful.
	 *
	 * @return True when a `T` value is active.
	 */
	[[nodiscard]] explicit operator bool() const noexcept {
		return has_value();
	}

	/**
	 * Access the success value.
	 *
	 * @return Mutable reference to the stored value.
	 */
	[[nodiscard]] T &value() & {
		auto *storage = std::get_if<ValueStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->value;
	}

	/**
	 * Access the success value.
	 *
	 * @return Immutable reference to the stored value.
	 */
	[[nodiscard]] const T &value() const & {
		const auto *storage = std::get_if<ValueStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->value;
	}

	/**
	 * Move the success value out of the result.
	 *
	 * @return Rvalue reference to the stored value.
	 */
	[[nodiscard]] T &&value() && {
		auto *storage = std::get_if<ValueStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return std::move(storage->value);
	}

	/**
	 * Access the error value.
	 *
	 * @return Mutable reference to the stored error.
	 */
	[[nodiscard]] E &error() & {
		auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->error;
	}

	/**
	 * Access the error value.
	 *
	 * @return Immutable reference to the stored error.
	 */
	[[nodiscard]] const E &error() const & {
		const auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->error;
	}

	/**
	 * Move the error value out of the result.
	 *
	 * @return Rvalue reference to the stored error.
	 */
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

/**
 * Tagged success-or-error result for operations with no success payload.
 *
 * @tparam E Error value type.
 */
template <class E>
class Result<void, E> final {
	static_assert(!std::is_void_v<E>, "Result error type must not be void.");
	static_assert(!std::is_reference_v<E>, "Result error type must not be a reference.");

public:
	/**
	 * Create a successful void result.
	 *
	 * @return Result in the success state.
	 */
	[[nodiscard]] static Result success() {
		return Result(SuccessStorage{});
	}

	/**
	 * Create a failed void result.
	 *
	 * @param error Error stored in the failure state.
	 * @return Result containing `error`.
	 */
	[[nodiscard]] static Result failure(E error) {
		return Result(ErrorStorage{ std::move(error) });
	}

	/**
	 * Check whether this result is successful.
	 *
	 * @return True when the success state is active.
	 */
	[[nodiscard]] bool has_value() const noexcept {
		return std::holds_alternative<SuccessStorage>(storage_);
	}

	/**
	 * Check whether this result is successful.
	 *
	 * @return True when the success state is active.
	 */
	[[nodiscard]] explicit operator bool() const noexcept {
		return has_value();
	}

	/// Assert that this result is successful.
	void value() const {
		QUADER_ASSERT(has_value());
	}

	/**
	 * Access the error value.
	 *
	 * @return Mutable reference to the stored error.
	 */
	[[nodiscard]] E &error() & {
		auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->error;
	}

	/**
	 * Access the error value.
	 *
	 * @return Immutable reference to the stored error.
	 */
	[[nodiscard]] const E &error() const & {
		const auto *storage = std::get_if<ErrorStorage>(&storage_);
		QUADER_ASSERT(storage != nullptr);
		return storage->error;
	}

	/**
	 * Move the error value out of the result.
	 *
	 * @return Rvalue reference to the stored error.
	 */
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
