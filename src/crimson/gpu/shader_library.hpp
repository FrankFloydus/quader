#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

#include "crimson/renderer_types.hpp"

namespace crimson::gpu {

enum class ShaderTarget {
    Vulkan,
    Direct3D11,
    Direct3D12,
    Metal,
};

enum class ShaderStage {
    Vertex,
    Fragment,
};

struct ShaderBinaryRef {
    ShaderProgramId program = ShaderProgramId::PrototypeLitCube;
    ShaderStage stage = ShaderStage::Vertex;
    ShaderTarget target = ShaderTarget::Vulkan;
    std::filesystem::path relative_path;
};

class ShaderLibrary final {
public:
    explicit ShaderLibrary(std::filesystem::path shader_root);

    const std::filesystem::path& shader_root() const noexcept;
    std::filesystem::path shader_path(ShaderProgramId program, ShaderStage stage, ShaderTarget target) const;
    std::optional<ShaderBinaryRef> shader_binary(
        ShaderProgramId program,
        ShaderStage stage,
        ShaderTarget target) const;

private:
    std::filesystem::path shader_root_;
};

std::string_view shader_target_name(ShaderTarget target) noexcept;
std::string_view shader_target_directory_name(ShaderTarget target) noexcept;
std::string_view shader_stage_name(ShaderStage stage) noexcept;
std::string_view shader_file_name(ShaderProgramId program, ShaderStage stage) noexcept;

} // namespace crimson::gpu
