#include "crimson/material/base_shader_registry.hpp"
#include "crimson/renderer_types.hpp"
#include "foundation/logging.hpp"
#include "io/gltf/gltf_material_mapper.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

using quader::io::gltf::GltfAlphaMode;
using quader::io::gltf::GltfMaterialSource;
using quader::io::gltf::GltfTextureRef;

[[nodiscard]] quader::foundation::Result<quader::io::gltf::GltfMaterialMapping, quader::io::IoError>
map_material(const GltfMaterialSource& source)
{
    return quader::io::gltf::map_gltf_material_to_crimson(source, crimson::make_v1_base_shader_registry());
}

[[nodiscard]] const crimson::MaterialParameter* find_parameter(
    const crimson::MaterialInstance& material,
    std::string_view name)
{
    for (const crimson::MaterialParameter& parameter : material.parameters) {
        if (parameter.name == name) {
            return &parameter;
        }
    }

    return nullptr;
}

[[nodiscard]] const quader::io::gltf::TextureSlotMapping* find_texture_mapping(
    const quader::io::gltf::GltfMaterialMapping& mapping,
    std::string_view slot_name)
{
    for (const quader::io::gltf::TextureSlotMapping& texture : mapping.texture_slots) {
        if (texture.slot_name == slot_name) {
            return &texture;
        }
    }

    return nullptr;
}

template <typename T>
[[nodiscard]] T parameter_value(const crimson::MaterialInstance& material, std::string_view name)
{
    const crimson::MaterialParameter* parameter = find_parameter(material, name);
    EXPECT_TRUE(parameter != nullptr);
    if (parameter == nullptr) {
        return T{};
    }

    const auto* value = std::get_if<T>(&parameter->value);
    EXPECT_TRUE(value != nullptr);
    return value == nullptr ? T{} : *value;
}

TEST(GltfMaterialMapper, AlphaModesMapToV1BaseShaders)
{
    GltfMaterialSource opaque;
    opaque.name = "OpaqueMaterial";
    opaque.alpha_mode = GltfAlphaMode::Opaque;
    auto opaque_mapping = map_material(opaque);
    EXPECT_TRUE(opaque_mapping);
    EXPECT_EQ(opaque_mapping.value().material.base_shader_id, crimson::BaseShaderId::OpaquePbr);
    EXPECT_TRUE(find_parameter(opaque_mapping.value().material, "alpha_cutoff") == nullptr);
    EXPECT_TRUE(find_parameter(opaque_mapping.value().material, "opacity") == nullptr);

    GltfMaterialSource mask;
    mask.name = "CutoutMaterial";
    mask.alpha_mode = GltfAlphaMode::Mask;
    mask.alpha_cutoff = 0.33F;
    auto mask_mapping = map_material(mask);
    EXPECT_TRUE(mask_mapping);
    EXPECT_EQ(mask_mapping.value().material.base_shader_id, crimson::BaseShaderId::AlphaCutoutPbr);
    EXPECT_EQ(parameter_value<float>(mask_mapping.value().material, "alpha_cutoff"), 0.33F);

    GltfMaterialSource blend;
    blend.name = "TransparentMaterial";
    blend.alpha_mode = GltfAlphaMode::Blend;
    blend.pbr.base_color_factor = crimson::MaterialColorSrgb{0.2F, 0.3F, 0.4F, 0.25F};
    auto blend_mapping = map_material(blend);
    EXPECT_TRUE(blend_mapping);
    EXPECT_EQ(blend_mapping.value().material.base_shader_id, crimson::BaseShaderId::TransparentPbr);
    EXPECT_EQ(parameter_value<float>(blend_mapping.value().material, "opacity"), 0.25F);
    EXPECT_TRUE(find_parameter(blend_mapping.value().material, "alpha_cutoff") == nullptr);
}

TEST(GltfMaterialMapper, PbrFactorsAndDoubleSidedMapThroughDeclaredParameters)
{
    GltfMaterialSource source;
    source.name = "PaintedMetal";
    source.pbr.base_color_factor = crimson::MaterialColorSrgb{0.25F, 0.5F, 0.75F, 0.8F};
    source.pbr.metallic_factor = 0.65F;
    source.pbr.roughness_factor = 0.35F;
    source.normal_scale = 2.0F;
    source.occlusion_strength = 0.45F;
    source.emissive_factor = crimson::MaterialColorSrgb{0.1F, 0.2F, 0.3F, 1.0F};
    source.double_sided = true;

    auto mapping = map_material(source);
    EXPECT_TRUE(mapping);
    if (!mapping) {
        return;
    }

    const crimson::MaterialInstance& material = mapping.value().material;
    EXPECT_EQ(parameter_value<crimson::MaterialColorSrgb>(material, "base_color").r, 0.25F);
    EXPECT_EQ(parameter_value<crimson::MaterialColorSrgb>(material, "base_color").g, 0.5F);
    EXPECT_EQ(parameter_value<crimson::MaterialColorSrgb>(material, "base_color").b, 0.75F);
    EXPECT_EQ(parameter_value<float>(material, "metallic"), 0.65F);
    EXPECT_EQ(parameter_value<float>(material, "roughness"), 0.35F);
    EXPECT_EQ(parameter_value<float>(material, "normal_strength"), 2.0F);
    EXPECT_EQ(parameter_value<float>(material, "occlusion_strength"), 0.45F);
    EXPECT_EQ(parameter_value<crimson::MaterialColorSrgb>(material, "emissive_color").g, 0.2F);
    EXPECT_EQ(parameter_value<float>(material, "emissive_strength"), 1.0F);
    EXPECT_EQ(parameter_value<bool>(material, "double_sided"), true);
    EXPECT_TRUE(material.overrides.double_sided);
}

TEST(GltfMaterialMapper, TextureSlotMappingsFollowSchemaOrderAndColorSpaces)
{
    GltfMaterialSource source;
    source.name = "Textured";
    source.pbr.base_color_texture = GltfTextureRef{"albedo.png", 0};
    source.pbr.metallic_roughness_texture = GltfTextureRef{"metal_rough.png", 1};
    source.normal_texture = GltfTextureRef{"normal.png", 0};
    source.occlusion_texture = GltfTextureRef{"ao.png", 1};
    source.emissive_texture = GltfTextureRef{"emissive.png", 0};

    auto mapping = map_material(source);
    EXPECT_TRUE(mapping);
    if (!mapping) {
        return;
    }

    EXPECT_EQ(mapping.value().texture_slots.size(), 5U);
    EXPECT_EQ(mapping.value().texture_slots[0].slot_name, std::string("base_color"));
    EXPECT_EQ(mapping.value().texture_slots[0].expected_color_space, crimson::TextureColorSpace::Srgb);
    EXPECT_EQ(mapping.value().texture_slots[1].slot_name, std::string("metallic_roughness"));
    EXPECT_EQ(mapping.value().texture_slots[1].expected_color_space, crimson::TextureColorSpace::Linear);
    EXPECT_EQ(mapping.value().texture_slots[2].slot_name, std::string("normal"));
    EXPECT_EQ(mapping.value().texture_slots[2].expected_color_space, crimson::TextureColorSpace::Data);
    EXPECT_EQ(mapping.value().texture_slots[3].slot_name, std::string("occlusion"));
    EXPECT_EQ(mapping.value().texture_slots[3].expected_color_space, crimson::TextureColorSpace::Linear);
    EXPECT_EQ(mapping.value().texture_slots[4].slot_name, std::string("emissive"));
    EXPECT_EQ(mapping.value().texture_slots[4].expected_color_space, crimson::TextureColorSpace::Srgb);
    EXPECT_EQ(mapping.value().texture_slots[1].source.texcoord, 1);

    for (const crimson::MaterialTextureBinding& texture : mapping.value().material.textures) {
        EXPECT_FALSE(crimson::is_valid_handle(texture.texture));
    }
}

TEST(GltfMaterialMapper, MissingTexturesDoNotCreateSourceSlotMappingsOrRuntimeHandles)
{
    GltfMaterialSource source;
    source.name = "NoTextures";

    auto mapping = map_material(source);
    EXPECT_TRUE(mapping);
    if (!mapping) {
        return;
    }

    EXPECT_TRUE(mapping.value().texture_slots.empty());
    EXPECT_FALSE(mapping.value().material.textures.empty());
    for (const crimson::MaterialTextureBinding& texture : mapping.value().material.textures) {
        EXPECT_FALSE(crimson::is_valid_handle(texture.texture));
    }
}

TEST(GltfMaterialMapper, TransparentOcclusionIsPreservedAsNonfatalDiagnostic)
{
    GltfMaterialSource source;
    source.name = "TransparentOcclusion";
    source.alpha_mode = GltfAlphaMode::Blend;
    source.occlusion_texture = GltfTextureRef{"ao.png", 0};
    source.occlusion_strength = 0.25F;

    auto mapping = map_material(source);
    EXPECT_TRUE(mapping);
    if (!mapping) {
        return;
    }

    EXPECT_TRUE(find_texture_mapping(mapping.value(), "occlusion") == nullptr);
    EXPECT_FALSE(mapping.value().diagnostics.has_errors());
    EXPECT_EQ(mapping.value().diagnostics.size(), 1U);
    EXPECT_EQ(mapping.value().diagnostics[0].code, quader::io::IoDiagnosticCode::UnsupportedFeaturePreserved);
    EXPECT_EQ(mapping.value().metadata.diagnostics.size(), 1U);
}

TEST(GltfMaterialMapper, UnsupportedExtensionsAreSortedUniqueAndWarned)
{
    GltfMaterialSource source;
    source.name = "ExtensionMaterial";
    source.unsupported_extensions = {
        "KHR_materials_volume",
        "KHR_texture_transform",
        "KHR_materials_ior",
        "KHR_materials_ior",
    };

    auto mapping = map_material(source);
    EXPECT_TRUE(mapping);
    if (!mapping) {
        return;
    }

    const std::vector<std::string>& preserved = mapping.value().metadata.preserved_extensions;
    EXPECT_EQ(preserved.size(), 3U);
    EXPECT_EQ(preserved[0], std::string("KHR_materials_ior"));
    EXPECT_EQ(preserved[1], std::string("KHR_materials_volume"));
    EXPECT_EQ(preserved[2], std::string("KHR_texture_transform"));
    EXPECT_EQ(mapping.value().diagnostics.size(), 3U);
    EXPECT_FALSE(mapping.value().diagnostics.has_errors());
}

TEST(GltfMaterialMapper, SchemaValidationFailureReturnsIoDiagnostic)
{
    const crimson::BaseShaderRegistry registry = crimson::make_v1_base_shader_registry();
    const crimson::BaseShaderDefinition* opaque = registry.find(crimson::BaseShaderId::OpaquePbr);
    EXPECT_TRUE(opaque != nullptr);
    if (opaque == nullptr) {
        return;
    }

    crimson::BaseShaderDefinition invalid_definition = *opaque;
    invalid_definition.parameters[0].kind = crimson::MaterialParameterKind::Float;
    crimson::BaseShaderRegistry invalid_registry{std::vector<crimson::BaseShaderDefinition>{invalid_definition}};

    GltfMaterialSource source;
    source.name = "InvalidSchema";
    auto mapping = quader::io::gltf::map_gltf_material_to_crimson(source, invalid_registry);
    EXPECT_FALSE(mapping);
    EXPECT_EQ(mapping.error().code, quader::io::IoErrorCode::ValidationFailed);
    EXPECT_EQ(mapping.error().diagnostic.code, quader::io::IoDiagnosticCode::MaterialMappingFailed);
    EXPECT_TRUE(mapping.error().related.has_errors());
}

} // namespace
