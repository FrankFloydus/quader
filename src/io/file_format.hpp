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
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace quader::io {

/// Descriptor for an import/export file format.
struct FileFormatDescriptor {
	/// Stable programmatic format id.
	std::string id;
	/// User-facing format name.
	std::string display_name;
	/// Supported extensions, with or without a leading dot.
	std::vector<std::string> extensions;
	/// True when the format can import a full document.
	bool can_import_document = false;
	/// True when the format can import a fragment for append.
	bool can_import_fragment = false;
	/// True when the format can export a full document.
	bool can_export_document = false;
	/// True when the format can export a fragment.
	bool can_export_fragment = false;
};

/**
 * Normalize a file extension for descriptor matching.
 *
 * @param extension Extension with or without a leading dot.
 * @return Lowercase extension without a leading dot.
 */
[[nodiscard]] inline std::string normalize_extension(std::string_view extension) {
	if (!extension.empty() && extension.front() == '.') {
		extension.remove_prefix(1);
	}

	std::string normalized;
	normalized.reserve(extension.size());
	for (const char kCharacter : extension) {
		normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(kCharacter))));
	}
	return normalized;
}

/**
 * Return a path extension in normalized descriptor form.
 *
 * @param path Path to inspect.
 * @return Lowercase extension without a leading dot.
 */
[[nodiscard]] inline std::string path_extension_without_dot(const std::filesystem::path &path) {
	return normalize_extension(path.extension().string());
}

/**
 * Check whether a descriptor supports a path extension.
 *
 * @param descriptor Descriptor to test.
 * @param path Path whose extension is matched.
 * @return True when any descriptor extension matches the path extension.
 */
[[nodiscard]] inline bool descriptor_supports_extension(const FileFormatDescriptor &descriptor,
		const std::filesystem::path &path) {
	const auto kExtension = path_extension_without_dot(path);
	if (kExtension.empty()) {
		return false;
	}

	return std::any_of(descriptor.extensions.begin(),
			descriptor.extensions.end(),
			[&kExtension](const std::string &candidate) {
				return normalize_extension(candidate) == kExtension;
			});
}

} // namespace quader::io
