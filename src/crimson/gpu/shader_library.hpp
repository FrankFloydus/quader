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

#include <filesystem>
#include <optional>
#include <string_view>

#include "crimson/renderer_types.hpp"

namespace crimson::gpu {

/// Compiled shader target platform.
enum class ShaderTarget {
	/// Vulkan shader target.
	Vulkan,
	/// Direct3D 11 shader target.
	Direct3D11,
	/// Direct3D 12 shader target.
	Direct3D12,
	/// Metal shader target.
	Metal,
};

/// Shader stage in a compiled program pair.
enum class ShaderStage {
	/// Vertex shader stage.
	Vertex,
	/// Fragment shader stage.
	Fragment,
};

/// Reference to one compiled shader binary.
struct ShaderBinaryRef {
	/// Shader program identifier.
	ShaderProgramId program = ShaderProgramId::OpaquePbr;
	/// Shader stage.
	ShaderStage stage = ShaderStage::Vertex;
	/// Compiled target platform.
	ShaderTarget target = ShaderTarget::Vulkan;
	/// Path relative to the shader root.
	std::filesystem::path relative_path;
};

/// Resolves compiled shader binary paths from a shader root.
class ShaderLibrary final {
public:
	/**
	 * Create a shader library rooted at a directory.
	 *
	 * @param shader_root Directory containing compiled shader target folders.
	 */
	explicit ShaderLibrary(std::filesystem::path shader_root);

	/// Return the shader root path.
	const std::filesystem::path &shader_root() const noexcept;
	/**
	 * Return the absolute path for one shader binary.
	 *
	 * @param program Shader program identifier.
	 * @param stage Shader stage.
	 * @param target Compiled target platform.
	 * @return Absolute shader binary path.
	 */
	std::filesystem::path shader_path(ShaderProgramId program, ShaderStage stage, ShaderTarget target) const;
	/**
	 * Return a shader binary reference when the shader is known.
	 *
	 * @param program Shader program identifier.
	 * @param stage Shader stage.
	 * @param target Compiled target platform.
	 * @return Binary reference, or empty for unknown shader names.
	 */
	std::optional<ShaderBinaryRef> shader_binary(
			ShaderProgramId program,
			ShaderStage stage,
			ShaderTarget target) const;

private:
	std::filesystem::path shader_root_;
};

/**
 * Return the stable shader target name.
 *
 * @param target Shader target to name.
 * @return Static target name.
 */
std::string_view shader_target_name(ShaderTarget target) noexcept;
/**
 * Return the stable shader target directory name.
 *
 * @param target Shader target to name as a directory.
 * @return Static directory name.
 */
std::string_view shader_target_directory_name(ShaderTarget target) noexcept;
/**
 * Return the stable shader stage name.
 *
 * @param stage Shader stage to name.
 * @return Static stage name.
 */
std::string_view shader_stage_name(ShaderStage stage) noexcept;
/**
 * Return the compiled shader file name.
 *
 * @param program Program id.
 * @param stage Shader stage.
 * @return Static shader file name.
 */
std::string_view shader_file_name(ShaderProgramId program, ShaderStage stage) noexcept;

} // namespace crimson::gpu
