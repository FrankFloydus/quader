#include "crimson/material/material_system.hpp"

#include "crimson/material/material_validation.hpp"

#include <string>
#include <utility>

namespace crimson {
namespace {

[[nodiscard]] std::uint32_t next_generation(std::uint32_t generation) noexcept
{
    ++generation;
    if (generation == 0) {
        generation = 1;
    }

    return generation;
}

[[nodiscard]] RendererDiagnostic material_system_diagnostic(
    RendererDiagnosticCode code,
    std::string message,
    std::string detail,
    std::string resource_name)
{
    return RendererDiagnostic{
        .severity = RendererDiagnosticSeverity::Error,
        .code = code,
        .message = std::move(message),
        .detail = std::move(detail),
        .subsystem = "material",
        .resource_name = std::move(resource_name),
    };
}

} // namespace

MaterialInstance make_default_material_instance(const BaseShaderDefinition& definition)
{
    MaterialInstance instance;
    instance.debug_name = definition.name + "Default";
    instance.base_shader_id = definition.id;
    instance.parameters.reserve(definition.parameters.size());
    for (const MaterialParameterDesc& parameter : definition.parameters) {
        instance.parameters.push_back(MaterialParameter{
            .name = parameter.name,
            .value = parameter.default_value,
        });
    }

    instance.textures.reserve(definition.texture_slots.size());
    for (const MaterialTextureSlotDesc& slot : definition.texture_slots) {
        instance.textures.push_back(MaterialTextureBinding{
            .name = slot.name,
            .texture = slot.default_texture,
        });
    }

    return instance;
}

MaterialSystem::MaterialSystem()
    : MaterialSystem(make_v1_base_shader_registry())
{
}

MaterialSystem::MaterialSystem(BaseShaderRegistry registry)
    : registry_(std::move(registry))
{
}

const BaseShaderRegistry& MaterialSystem::registry() const noexcept
{
    return registry_;
}

quader::foundation::Result<RenderMaterialHandle, RendererDiagnostic> MaterialSystem::create_default_material(
    BaseShaderId base_shader_id)
{
    const BaseShaderDefinition* definition = registry_.find(base_shader_id);
    if (definition == nullptr) {
        return quader::foundation::Result<RenderMaterialHandle, RendererDiagnostic>::failure(material_system_diagnostic(
            RendererDiagnosticCode::MaterialSchemaInvalid,
            "Crimson cannot create a default material for an unknown base shader.",
            std::string(base_shader_id_name(base_shader_id)),
            "MaterialSystem"));
    }

    return create_material(make_default_material_instance(*definition));
}

quader::foundation::Result<RenderMaterialHandle, RendererDiagnostic> MaterialSystem::create_material(
    MaterialInstance instance)
{
    const auto normalized = normalize(instance);
    if (!normalized) {
        return quader::foundation::Result<RenderMaterialHandle, RendererDiagnostic>::failure(normalized.error());
    }

    std::uint32_t index = 0;
    if (!free_indices_.empty()) {
        index = free_indices_.back();
        free_indices_.pop_back();

        Slot& slot = slots_[static_cast<std::size_t>(index - 1)];
        slot.material = normalized.value();
        slot.occupied = true;
    } else {
        index = static_cast<std::uint32_t>(slots_.size() + 1);
        slots_.push_back(Slot{
            .material = normalized.value(),
            .generation = 1,
            .occupied = true,
        });
    }

    ++live_material_count_;
    const Slot& slot = slots_[static_cast<std::size_t>(index - 1)];
    return quader::foundation::Result<RenderMaterialHandle, RendererDiagnostic>::success(
        RenderMaterialHandle{index, slot.generation});
}

const MaterialInstance* MaterialSystem::get(RenderMaterialHandle handle) const noexcept
{
    const Slot* slot = slot_for(handle);
    return slot == nullptr ? nullptr : &slot->material;
}

MaterialInstance* MaterialSystem::get(RenderMaterialHandle handle) noexcept
{
    Slot* slot = slot_for(handle);
    return slot == nullptr ? nullptr : &slot->material;
}

bool MaterialSystem::destroy(RenderMaterialHandle handle) noexcept
{
    Slot* slot = slot_for(handle);
    if (slot == nullptr) {
        return false;
    }

    slot->material = MaterialInstance{};
    slot->generation = next_generation(slot->generation);
    slot->occupied = false;
    free_indices_.push_back(handle.index);
    --live_material_count_;
    return true;
}

std::size_t MaterialSystem::live_material_count() const noexcept
{
    return live_material_count_;
}

quader::foundation::Result<void, RendererDiagnostic> MaterialSystem::set_parameter(
    RenderMaterialHandle handle,
    std::string name,
    MaterialParameterValue value)
{
    MaterialInstance* material = get(handle);
    if (material == nullptr) {
        return quader::foundation::Result<void, RendererDiagnostic>::failure(invalid_handle_diagnostic(handle));
    }

    MaterialInstance candidate = *material;
    bool replaced = false;
    for (MaterialParameter& parameter : candidate.parameters) {
        if (parameter.name == name) {
            parameter.value = std::move(value);
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        candidate.parameters.push_back(MaterialParameter{
            .name = std::move(name),
            .value = std::move(value),
        });
    }

    const auto normalized = normalize(candidate);
    if (!normalized) {
        return quader::foundation::Result<void, RendererDiagnostic>::failure(normalized.error());
    }

    *material = normalized.value();
    return quader::foundation::Result<void, RendererDiagnostic>::success();
}

quader::foundation::Result<void, RendererDiagnostic> MaterialSystem::bind_texture(
    RenderMaterialHandle handle,
    std::string name,
    RenderTextureHandle texture)
{
    MaterialInstance* material = get(handle);
    if (material == nullptr) {
        return quader::foundation::Result<void, RendererDiagnostic>::failure(invalid_handle_diagnostic(handle));
    }

    MaterialInstance candidate = *material;
    bool replaced = false;
    for (MaterialTextureBinding& binding : candidate.textures) {
        if (binding.name == name) {
            binding.texture = texture;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        candidate.textures.push_back(MaterialTextureBinding{
            .name = std::move(name),
            .texture = texture,
        });
    }

    const auto normalized = normalize(candidate);
    if (!normalized) {
        return quader::foundation::Result<void, RendererDiagnostic>::failure(normalized.error());
    }

    *material = normalized.value();
    return quader::foundation::Result<void, RendererDiagnostic>::success();
}

MaterialSystem::Slot* MaterialSystem::slot_for(RenderMaterialHandle handle) noexcept
{
    if (!is_valid_handle(handle) || handle.index > slots_.size()) {
        return nullptr;
    }

    Slot& slot = slots_[static_cast<std::size_t>(handle.index - 1)];
    if (!slot.occupied || slot.generation != handle.generation) {
        return nullptr;
    }

    return &slot;
}

const MaterialSystem::Slot* MaterialSystem::slot_for(RenderMaterialHandle handle) const noexcept
{
    if (!is_valid_handle(handle) || handle.index > slots_.size()) {
        return nullptr;
    }

    const Slot& slot = slots_[static_cast<std::size_t>(handle.index - 1)];
    if (!slot.occupied || slot.generation != handle.generation) {
        return nullptr;
    }

    return &slot;
}

quader::foundation::Result<MaterialInstance, RendererDiagnostic> MaterialSystem::normalize(
    const MaterialInstance& instance) const
{
    const auto validation = validate_material_instance(registry_, instance);
    if (!validation) {
        return quader::foundation::Result<MaterialInstance, RendererDiagnostic>::failure(validation.error());
    }

    const BaseShaderDefinition* definition = registry_.find(instance.base_shader_id);
    if (definition == nullptr) {
        return quader::foundation::Result<MaterialInstance, RendererDiagnostic>::failure(material_system_diagnostic(
            RendererDiagnosticCode::MaterialSchemaInvalid,
            "Crimson cannot normalize material for an unknown base shader.",
            std::string(base_shader_id_name(instance.base_shader_id)),
            instance.debug_name));
    }

    MaterialInstance normalized = make_default_material_instance(*definition);
    normalized.debug_name = instance.debug_name.empty() ? normalized.debug_name : instance.debug_name;
    normalized.overrides = instance.overrides;

    for (const MaterialParameter& input_parameter : instance.parameters) {
        for (MaterialParameter& output_parameter : normalized.parameters) {
            if (output_parameter.name == input_parameter.name) {
                output_parameter.value = input_parameter.value;
                break;
            }
        }
    }

    for (const MaterialTextureBinding& input_texture : instance.textures) {
        for (MaterialTextureBinding& output_texture : normalized.textures) {
            if (output_texture.name == input_texture.name) {
                output_texture.texture = input_texture.texture;
                break;
            }
        }
    }

    return quader::foundation::Result<MaterialInstance, RendererDiagnostic>::success(std::move(normalized));
}

RendererDiagnostic MaterialSystem::invalid_handle_diagnostic(RenderMaterialHandle handle) const
{
    return material_system_diagnostic(
        RendererDiagnosticCode::ResourceLifetimeError,
        "Crimson material handle is stale or invalid.",
        "index=" + std::to_string(handle.index) + " generation=" + std::to_string(handle.generation),
        "MaterialSystem");
}

} // namespace crimson
