#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace quader::io {

struct FileFormatDescriptor {
	std::string id;
	std::string display_name;
	std::vector<std::string> extensions;
	bool can_import_document = false;
	bool can_import_fragment = false;
	bool can_export_document = false;
	bool can_export_fragment = false;
};

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

[[nodiscard]] inline std::string path_extension_without_dot(const std::filesystem::path &path) {
	return normalize_extension(path.extension().string());
}

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
