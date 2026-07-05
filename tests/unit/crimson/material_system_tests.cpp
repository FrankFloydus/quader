/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_material_cache.hpp"
#include "crimson/gpu/gpu_pbr_pass.hpp"
#include "crimson/material/base_shader_registry.hpp"
#include "crimson/material/default_material.hpp"
#include "crimson/material/material_system.hpp"
#include "crimson/material/material_validation.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <variant>
#include <vector>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

[[nodiscard]] const crimson::BaseShaderDefinition *find_shader(
		const crimson::BaseShaderRegistry &registry,
		crimson::BaseShaderId id) {
	const crimson::BaseShaderDefinition *definition = registry.find(id);
	expect_true(definition != nullptr, "expected base shader definition exists");
	return definition;
}

[[nodiscard]] bool has_parameter(const crimson::BaseShaderDefinition &definition, std::string_view name) {
	return crimson::find_parameter_desc(definition, name) != nullptr;
}

[[nodiscard]] bool has_texture_slot(
		const crimson::BaseShaderDefinition &definition,
		std::string_view name,
		crimson::TextureColorSpace color_space) {
	const crimson::MaterialTextureSlotDesc *slot = crimson::find_texture_slot_desc(definition, name);
	return slot != nullptr && slot->color_space == color_space;
}

[[nodiscard]] const crimson::MaterialParameter *find_material_parameter(
		const crimson::MaterialInstance &material,
		std::string_view name) {
	for (const crimson::MaterialParameter &parameter : material.parameters) {
		if (parameter.name == name) {
			return &parameter;
		}
	}

	return nullptr;
}

[[nodiscard]] const crimson::MaterialTextureBinding *find_material_texture(
		const crimson::MaterialInstance &material,
		std::string_view name) {
	for (const crimson::MaterialTextureBinding &texture : material.textures) {
		if (texture.name == name) {
			return &texture;
		}
	}

	return nullptr;
}

TEST(MaterialSystem, V1RegistryContainsExpectedBaseShadersAndQueues) {
	const crimson::BaseShaderRegistry kRegistry = crimson::make_v1_base_shader_registry();
	expect_true(kRegistry.definitions().size() == 5, "V1 registry contains exactly five base shaders");

	const crimson::BaseShaderDefinition *opaque = find_shader(kRegistry, crimson::BaseShaderId::OpaquePbr);
	const crimson::BaseShaderDefinition *unlit = find_shader(kRegistry, crimson::BaseShaderId::UnlitSurface);
	const crimson::BaseShaderDefinition *cutout = find_shader(kRegistry, crimson::BaseShaderId::AlphaCutoutPbr);
	const crimson::BaseShaderDefinition *transparent = find_shader(kRegistry, crimson::BaseShaderId::TransparentPbr);
	const crimson::BaseShaderDefinition *overlay = find_shader(kRegistry, crimson::BaseShaderId::OverlayUnlit);
	if (opaque == nullptr || unlit == nullptr || cutout == nullptr || transparent == nullptr || overlay == nullptr) {
		return;
	}

	expect_true(opaque->domain == crimson::RenderDomain::LitSurface, "OpaquePbr is a lit surface");
	expect_true(opaque->default_queue == crimson::RenderQueue::Opaque, "OpaquePbr defaults to opaque queue");
	expect_true(opaque->depth_mode == crimson::DepthMode::TestWrite, "OpaquePbr writes depth");
	expect_true(opaque->blend_mode == crimson::BlendMode::Off, "OpaquePbr disables blending");
	expect_true(opaque->shadow_mode == crimson::ShadowMode::CastAndReceive, "OpaquePbr casts and receives shadows");

	expect_true(unlit->domain == crimson::RenderDomain::LitSurface, "UnlitSurface is routed as a lit surface queue material");
	expect_true(unlit->default_queue == crimson::RenderQueue::Opaque, "UnlitSurface defaults to opaque queue");
	expect_true(unlit->shadow_mode == crimson::ShadowMode::None, "UnlitSurface has no shadows");

	expect_true(cutout->default_queue == crimson::RenderQueue::AlphaCutout, "AlphaCutoutPbr uses cutout queue");
	expect_true(cutout->blend_mode == crimson::BlendMode::Off, "AlphaCutoutPbr disables blending");
	expect_true(
			cutout->shadow_mode == crimson::ShadowMode::AlphaTestedCastAndReceive,
			"AlphaCutoutPbr declares alpha-tested shadows");

	expect_true(
			transparent->domain == crimson::RenderDomain::TransparentSurface,
			"TransparentPbr is a transparent surface");
	expect_true(transparent->default_queue == crimson::RenderQueue::Transparent, "TransparentPbr uses transparent queue");
	expect_true(transparent->depth_mode == crimson::DepthMode::TestReadOnly, "TransparentPbr does not write depth");
	expect_true(transparent->blend_mode == crimson::BlendMode::AlphaBlend, "TransparentPbr alpha blends");
	expect_true(transparent->shadow_mode == crimson::ShadowMode::None, "TransparentPbr does not cast V1 shadows");

	expect_true(overlay->domain == crimson::RenderDomain::Overlay, "OverlayUnlit is an overlay domain shader");
	expect_true(
			overlay->default_queue == crimson::RenderQueue::OverlayDepthTested,
			"OverlayUnlit defaults to an overlay queue");
	expect_true(overlay->depth_mode == crimson::DepthMode::OverlayCommand, "OverlayUnlit depth comes from overlay data");
	expect_true(overlay->blend_mode == crimson::BlendMode::AlphaBlend, "OverlayUnlit alpha blends");
	expect_true(overlay->shadow_mode == crimson::ShadowMode::None, "OverlayUnlit has no shadows");
}

TEST(MaterialSystem, SchemasExposeOnlyDeclaredMaterialFields) {
	const crimson::BaseShaderRegistry kRegistry = crimson::make_v1_base_shader_registry();
	const crimson::BaseShaderDefinition *opaque = find_shader(kRegistry, crimson::BaseShaderId::OpaquePbr);
	const crimson::BaseShaderDefinition *unlit = find_shader(kRegistry, crimson::BaseShaderId::UnlitSurface);
	const crimson::BaseShaderDefinition *cutout = find_shader(kRegistry, crimson::BaseShaderId::AlphaCutoutPbr);
	const crimson::BaseShaderDefinition *transparent = find_shader(kRegistry, crimson::BaseShaderId::TransparentPbr);
	const crimson::BaseShaderDefinition *overlay = find_shader(kRegistry, crimson::BaseShaderId::OverlayUnlit);
	if (opaque == nullptr || unlit == nullptr || cutout == nullptr || transparent == nullptr || overlay == nullptr) {
		return;
	}

	expect_true(has_parameter(*opaque, "base_color"), "OpaquePbr exposes base color");
	expect_true(has_parameter(*opaque, "metallic"), "OpaquePbr exposes metallic");
	expect_true(has_parameter(*opaque, "occlusion_strength"), "OpaquePbr exposes AO strength");
	expect_true(!has_parameter(*opaque, "opacity"), "OpaquePbr does not expose opacity");
	expect_true(!has_parameter(*opaque, "alpha_cutoff"), "OpaquePbr does not expose alpha cutoff");
	expect_true(!has_parameter(*opaque, "transmission"), "OpaquePbr does not expose transmission");
	expect_true(!has_parameter(*opaque, "ior"), "OpaquePbr does not expose IOR");

	expect_true(has_parameter(*unlit, "base_color"), "UnlitSurface exposes base color");
	expect_true(has_parameter(*unlit, "emissive_color"), "UnlitSurface exposes emissive color");
	expect_true(!has_parameter(*unlit, "metallic"), "UnlitSurface does not expose metallic");
	expect_true(!has_parameter(*unlit, "roughness"), "UnlitSurface does not expose roughness");

	expect_true(has_parameter(*cutout, "alpha_cutoff"), "AlphaCutoutPbr exposes alpha cutoff");
	expect_true(has_parameter(*cutout, "alpha_source_channel"), "AlphaCutoutPbr exposes alpha source channel");
	expect_true(!has_parameter(*cutout, "opacity"), "AlphaCutoutPbr does not expose opacity blending");
	expect_true(!has_parameter(*cutout, "transmission"), "AlphaCutoutPbr does not expose transmission");

	expect_true(has_parameter(*transparent, "opacity"), "TransparentPbr exposes opacity");
	expect_true(has_parameter(*transparent, "roughness"), "TransparentPbr exposes roughness");
	expect_true(!has_parameter(*transparent, "alpha_cutoff"), "TransparentPbr does not expose alpha cutoff");
	expect_true(!has_parameter(*transparent, "occlusion_strength"), "TransparentPbr does not expose AO strength");
	expect_true(!has_parameter(*transparent, "ior"), "TransparentPbr does not expose IOR");

	expect_true(has_parameter(*overlay, "color"), "OverlayUnlit exposes color");
	expect_true(has_parameter(*overlay, "line_width_mode"), "OverlayUnlit exposes line width mode");
	expect_true(has_parameter(*overlay, "depth_mode"), "OverlayUnlit exposes overlay depth mode");
	expect_true(!has_parameter(*overlay, "base_color"), "OverlayUnlit does not expose PBR base color");
	expect_true(!has_parameter(*overlay, "metallic"), "OverlayUnlit does not expose metallic");
	expect_true(!has_parameter(*overlay, "roughness"), "OverlayUnlit does not expose roughness");
}

TEST(MaterialSystem, TextureSlotsHaveDeclaredColorSpaces) {
	const crimson::BaseShaderRegistry kRegistry = crimson::make_v1_base_shader_registry();
	const crimson::BaseShaderDefinition *opaque = find_shader(kRegistry, crimson::BaseShaderId::OpaquePbr);
	const crimson::BaseShaderDefinition *unlit = find_shader(kRegistry, crimson::BaseShaderId::UnlitSurface);
	const crimson::BaseShaderDefinition *cutout = find_shader(kRegistry, crimson::BaseShaderId::AlphaCutoutPbr);
	const crimson::BaseShaderDefinition *transparent = find_shader(kRegistry, crimson::BaseShaderId::TransparentPbr);
	const crimson::BaseShaderDefinition *overlay = find_shader(kRegistry, crimson::BaseShaderId::OverlayUnlit);
	if (opaque == nullptr || unlit == nullptr || cutout == nullptr || transparent == nullptr || overlay == nullptr) {
		return;
	}

	expect_true(
			has_texture_slot(*opaque, "base_color", crimson::TextureColorSpace::Srgb),
			"OpaquePbr base color texture is sRGB");
	expect_true(
			has_texture_slot(*opaque, "emissive", crimson::TextureColorSpace::Srgb),
			"OpaquePbr emissive texture is sRGB");
	expect_true(
			has_texture_slot(*opaque, "metallic_roughness", crimson::TextureColorSpace::Linear),
			"OpaquePbr metallic/roughness texture is linear");
	expect_true(
			has_texture_slot(*opaque, "normal", crimson::TextureColorSpace::Data),
			"OpaquePbr normal texture is data");
	expect_true(
			has_texture_slot(*opaque, "occlusion", crimson::TextureColorSpace::Linear),
			"OpaquePbr occlusion texture is linear");

	expect_true(
			has_texture_slot(*unlit, "base_color", crimson::TextureColorSpace::Srgb),
			"UnlitSurface base color texture is sRGB");
	expect_true(
			crimson::find_texture_slot_desc(*unlit, "metallic_roughness") == nullptr,
			"UnlitSurface has no PBR data-map texture slots");

	expect_true(
			has_texture_slot(*cutout, "base_color", crimson::TextureColorSpace::Srgb),
			"AlphaCutoutPbr base color texture is sRGB");
	expect_true(
			has_texture_slot(*transparent, "base_color", crimson::TextureColorSpace::Srgb),
			"TransparentPbr base color texture is sRGB");
	expect_true(
			has_texture_slot(*transparent, "normal", crimson::TextureColorSpace::Data),
			"TransparentPbr normal texture is data");
	expect_true(
			crimson::find_texture_slot_desc(*transparent, "occlusion") == nullptr,
			"TransparentPbr has no occlusion texture slot in V1");
	expect_true(overlay->texture_slots.empty(), "OverlayUnlit has no PBR texture slots");
}

TEST(MaterialSystem, MaterialDefaultsAreDeterministicAndNormalized) {
	crimson::MaterialSystem materials;
	const crimson::BaseShaderDefinition *opaque = find_shader(materials.registry(), crimson::BaseShaderId::OpaquePbr);
	if (opaque == nullptr) {
		return;
	}

	const crimson::MaterialInstance kFirstDefault = crimson::make_default_material_instance(*opaque);
	const crimson::MaterialInstance kSecondDefault = crimson::make_default_material_instance(*opaque);
	expect_true(
			kFirstDefault.parameters.size() == opaque->parameters.size(),
			"default material contains one parameter value per schema parameter");
	expect_true(kFirstDefault.textures.size() == opaque->texture_slots.size(), "default material binds every texture slot");
	expect_true(
			kFirstDefault.parameters.size() == kSecondDefault.parameters.size(),
			"default material parameter count is deterministic");
	expect_true(
			kFirstDefault.parameters[0].name == kSecondDefault.parameters[0].name,
			"default material parameter order is deterministic");
	expect_true(
			kFirstDefault.parameters[0].value == kSecondDefault.parameters[0].value,
			"default material parameter values are deterministic");

	crimson::MaterialInstance partial;
	partial.debug_name = "PartialOpaque";
	partial.base_shader_id = crimson::BaseShaderId::OpaquePbr;
	partial.parameters.push_back(crimson::MaterialParameter{
			.name = "roughness",
			.value = 0.8F,
	});

	auto handle = materials.create_material(partial);
	expect_true(handle.has_value(), "partial material with declared override is accepted");
	if (!handle) {
		return;
	}

	const crimson::MaterialInstance *stored = materials.get(handle.value());
	expect_true(stored != nullptr, "created material resolves from public handle");
	if (stored == nullptr) {
		return;
	}

	expect_true(stored->parameters.size() == opaque->parameters.size(), "stored material is normalized with defaults");
	const crimson::MaterialParameter *roughness = find_material_parameter(*stored, "roughness");
	const crimson::MaterialParameter *metallic = find_material_parameter(*stored, "metallic");
	expect_true(
			roughness != nullptr && std::get<float>(roughness->value) == 0.8F,
			"stored material keeps supplied roughness override");
	expect_true(
			metallic != nullptr && std::get<float>(metallic->value) == 0.0F,
			"stored material fills missing metallic default");
}

TEST(MaterialSystem, DefaultQuaderMaterialBindsReferenceAlbedoTexture) {
	crimson::MaterialSystem materials;
	const crimson::BaseShaderDefinition *opaque = find_shader(materials.registry(), crimson::BaseShaderId::OpaquePbr);
	const crimson::BaseShaderDefinition *unlit = find_shader(materials.registry(), crimson::BaseShaderId::UnlitSurface);
	if (opaque == nullptr || unlit == nullptr) {
		return;
	}

	const crimson::RenderTextureHandle kAlbedoTexture{ 42, 7 };
	const crimson::MaterialInstance kDefault =
			crimson::make_default_quader_material_instance(*opaque, kAlbedoTexture);
	const crimson::MaterialParameter *base_color = find_material_parameter(kDefault, "base_color");
	const crimson::MaterialParameter *roughness = find_material_parameter(kDefault, "roughness");
	const crimson::MaterialParameter *metallic = find_material_parameter(kDefault, "metallic");
	const crimson::MaterialTextureBinding *albedo = find_material_texture(kDefault, "base_color");

	expect_true(kDefault.debug_name == crimson::kDefaultQuaderMaterialName, "default material has the Quader default name");
	const auto *color = base_color == nullptr ? nullptr : std::get_if<crimson::MaterialColorSrgb>(&base_color->value);
	expect_true(
			color != nullptr && color->r == 1.0F && color->g == 1.0F && color->b == 1.0F && color->a == 1.0F,
			"default material uses white base color factor");
	expect_true(
			roughness != nullptr && std::get<float>(roughness->value) == 1.0F,
			"default material is fully rough");
	expect_true(
			metallic != nullptr && std::get<float>(metallic->value) == 0.0F,
			"default material is non-metallic");
	expect_true(
			albedo != nullptr && albedo->texture == kAlbedoTexture,
			"default material binds the provided base-color texture handle");

	const crimson::MaterialInstance kUnlitDefault =
			crimson::make_default_quader_material_instance(*unlit, kAlbedoTexture);
	const crimson::MaterialTextureBinding *unlit_albedo = find_material_texture(kUnlitDefault, "base_color");
	expect_true(
			unlit_albedo != nullptr && unlit_albedo->texture == kAlbedoTexture,
			"unlit default material binds the same base-color texture handle");
	expect_true(
			find_material_parameter(kUnlitDefault, "roughness") == nullptr,
			"unlit default material does not inject PBR roughness");
	expect_true(
			find_material_parameter(kUnlitDefault, "metallic") == nullptr,
			"unlit default material does not inject PBR metallic");
}

TEST(MaterialSystem, Rgba8MipChainUsesSrgbAwareDownsampling) {
	const std::vector<unsigned char> kPixels = {
		0, 0, 0, 255,
		255, 255, 255, 255,
		255, 255, 255, 255,
		0, 0, 0, 255,
	};

	const std::vector<crimson::gpu::Rgba8MipLevel> kMips =
			crimson::gpu::make_rgba8_mip_chain(kPixels, 2, 2, crimson::TextureColorSpace::Srgb);
	ASSERT_EQ(kMips.size(), 2U);
	EXPECT_EQ(kMips[0].width, 2);
	EXPECT_EQ(kMips[0].height, 2);
	EXPECT_EQ(kMips[1].width, 1);
	EXPECT_EQ(kMips[1].height, 1);
	ASSERT_EQ(kMips[1].rgba8.size(), 4U);
	EXPECT_NEAR(static_cast<double>(kMips[1].rgba8[0]), 188.0, 1.0);
	EXPECT_NEAR(static_cast<double>(kMips[1].rgba8[1]), 188.0, 1.0);
	EXPECT_NEAR(static_cast<double>(kMips[1].rgba8[2]), 188.0, 1.0);
	EXPECT_EQ(kMips[1].rgba8[3], 255);
}

TEST(MaterialSystem, Rgba8MipLevelCountReachesOneByOne) {
	EXPECT_EQ(crimson::gpu::rgba8_mip_level_count(4, 2), 3);
	EXPECT_EQ(crimson::gpu::rgba8_mip_level_count(1, 1), 1);
	EXPECT_EQ(crimson::gpu::rgba8_mip_level_count(0, 4), 0);
}

TEST(MaterialSystem, MaterialTextureSamplerFlagsUseReferenceAnisotropicRepeatFiltering) {
	const std::uint64_t flags = crimson::gpu::material_texture_sampler_flags();
	EXPECT_TRUE(crimson::gpu::material_texture_sampler_uses_anisotropic_repeat_filtering(flags));
	EXPECT_FALSE(crimson::gpu::material_texture_sampler_uses_anisotropic_repeat_filtering(0U));
}

TEST(MaterialSystem, ValidationRejectsUnknownAndWrongKind) {
	const crimson::BaseShaderRegistry kRegistry = crimson::make_v1_base_shader_registry();

	crimson::MaterialInstance overlay_with_pbr_field;
	overlay_with_pbr_field.debug_name = "BadOverlay";
	overlay_with_pbr_field.base_shader_id = crimson::BaseShaderId::OverlayUnlit;
	overlay_with_pbr_field.parameters.push_back(crimson::MaterialParameter{
			.name = "metallic",
			.value = 0.5F,
	});
	const auto kOverlayValidation = crimson::validate_material_instance(kRegistry, overlay_with_pbr_field);
	expect_true(!kOverlayValidation, "OverlayUnlit rejects PBR fields");
	expect_true(
			!kOverlayValidation && kOverlayValidation.error().code == crimson::RendererDiagnosticCode::MaterialValidationFailed,
			"unknown overlay field returns material validation failure");

	crimson::MaterialInstance wrong_kind;
	wrong_kind.debug_name = "WrongKind";
	wrong_kind.base_shader_id = crimson::BaseShaderId::OpaquePbr;
	wrong_kind.parameters.push_back(crimson::MaterialParameter{
			.name = "roughness",
			.value = true,
	});
	expect_true(!crimson::validate_material_instance(kRegistry, wrong_kind), "wrong parameter kind is rejected");

	crimson::MaterialInstance duplicate;
	duplicate.debug_name = "Duplicate";
	duplicate.base_shader_id = crimson::BaseShaderId::OpaquePbr;
	duplicate.parameters.push_back(crimson::MaterialParameter{ .name = "roughness", .value = 0.5F });
	duplicate.parameters.push_back(crimson::MaterialParameter{ .name = "roughness", .value = 0.6F });
	expect_true(!crimson::validate_material_instance(kRegistry, duplicate), "duplicate parameter is rejected");

	crimson::MaterialInstance unknown_texture;
	unknown_texture.debug_name = "UnknownTexture";
	unknown_texture.base_shader_id = crimson::BaseShaderId::OpaquePbr;
	unknown_texture.textures.push_back(crimson::MaterialTextureBinding{
			.name = "transmission",
			.texture = crimson::RenderTextureHandle{ 1, 1 },
	});
	expect_true(!crimson::validate_material_instance(kRegistry, unknown_texture), "unknown texture slot is rejected");
}

TEST(MaterialSystem, MaterialHandlesAreGenerationChecked) {
	crimson::MaterialSystem materials;
	auto handle = materials.create_default_material(crimson::BaseShaderId::OverlayUnlit);
	expect_true(handle.has_value(), "default overlay material is created");
	if (!handle) {
		return;
	}

	expect_true(materials.live_material_count() == 1, "material system tracks one live material");
	expect_true(materials.get(handle.value()) != nullptr, "fresh material handle resolves");
	expect_true(materials.destroy(handle.value()), "destroying a material succeeds");
	expect_true(materials.live_material_count() == 0, "destroy decrements live material count");
	expect_true(materials.get(handle.value()) == nullptr, "destroyed material handle is stale");

	auto replacement = materials.create_default_material(crimson::BaseShaderId::OverlayUnlit);
	expect_true(replacement.has_value(), "replacement material is created");
	if (!replacement) {
		return;
	}

	expect_true(
			replacement.value().index == handle.value().index,
			"material system reuses free slots deterministically");
	expect_true(
			replacement.value().generation != handle.value().generation,
			"reused material slot advances generation");
}

TEST(MaterialSystem, PbrMaterialBlocksPackSchemaValuesForGpuBinding) {
	crimson::MaterialSystem materials;
	auto handle = materials.create_default_material(crimson::BaseShaderId::OpaquePbr);
	expect_true(handle.has_value(), "default opaque material is created for GPU packing");
	if (!handle) {
		return;
	}

	auto base_color_set = materials.set_parameter(
			handle.value(),
			"base_color",
			crimson::MaterialColorSrgb{ 0.5F, 0.0F, 0.0F, 1.0F });
	auto roughness_set = materials.set_parameter(handle.value(), "roughness", 0.75F);
	expect_true(base_color_set.has_value() && roughness_set.has_value(), "declared PBR parameters can be changed");

	const crimson::MaterialInstance *material = materials.get(handle.value());
	const crimson::BaseShaderDefinition *definition = materials.registry().find(crimson::BaseShaderId::OpaquePbr);
	expect_true(material != nullptr && definition != nullptr, "material and schema resolve for packing");
	if (material == nullptr || definition == nullptr) {
		return;
	}

	const crimson::gpu::GpuPbrMaterialBlock kBlock = crimson::gpu::pack_pbr_material_block(*material, *definition);
	expect_true(kBlock.base_color_factor_linear[0] > 0.21F && kBlock.base_color_factor_linear[0] < 0.22F,
			"sRGB base color is converted to linear for GPU material block");
	expect_true(kBlock.factors[1] == 0.75F, "roughness factor is packed from material parameter");
	expect_true(kBlock.flags[1] == 0.0F && kBlock.flags[2] == 0.0F, "opaque PBR block does not set cutout or transparent flags");
}

TEST(MaterialSystem, PbrPacketPreparationBucketsAndSortsDrawPackets) {
	crimson::MaterialSystem materials;
	const crimson::RenderMaterialHandle kOpaque = materials.create_default_material(crimson::BaseShaderId::OpaquePbr).value();
	const crimson::RenderMaterialHandle kCutout =
			materials.create_default_material(crimson::BaseShaderId::AlphaCutoutPbr).value();
	const crimson::RenderMaterialHandle kTransparent =
			materials.create_default_material(crimson::BaseShaderId::TransparentPbr).value();
	const crimson::RenderMeshHandle kMesh{ 1, 1 };

	crimson::RenderObject opaque_object;
	opaque_object.object_id = 1;
	opaque_object.mesh = kMesh;
	opaque_object.material = kOpaque;
	opaque_object.base_shader = crimson::BaseShaderId::OpaquePbr;
	opaque_object.queue = crimson::RenderQueue::Opaque;

	crimson::RenderObject cutout_object;
	cutout_object.object_id = 2;
	cutout_object.mesh = kMesh;
	cutout_object.material = kCutout;
	cutout_object.base_shader = crimson::BaseShaderId::AlphaCutoutPbr;
	cutout_object.queue = crimson::RenderQueue::AlphaCutout;

	crimson::RenderObject transparent_a;
	transparent_a.object_id = 4;
	transparent_a.mesh = kMesh;
	transparent_a.material = kTransparent;
	transparent_a.base_shader = crimson::BaseShaderId::TransparentPbr;
	transparent_a.queue = crimson::RenderQueue::Transparent;

	crimson::RenderObject transparent_b = transparent_a;
	transparent_b.object_id = 3;

	const std::array<crimson::RenderObject, 4> kObjects = {
		transparent_a,
		opaque_object,
		transparent_b,
		cutout_object,
	};
	const crimson::RenderCamera kCamera{
		.eye = { 0.0F, 0.0F, 0.0F },
		.target = { 0.0F, 0.0F, -1.0F },
		.forward = { 0.0F, 0.0F, -1.0F },
	};

	const crimson::gpu::PbrPassPackets kPackets = crimson::gpu::prepare_pbr_pass_packets(
			kObjects,
			materials.registry(),
			materials,
			kCamera,
			crimson::RenderMeshHandle{ 9, 1 },
			kOpaque,
			1.0F);

	expect_true(kPackets.opaque.size() == 1, "opaque PBR packets are bucketed");
	expect_true(kPackets.alpha_cutout.size() == 1, "alpha cutout PBR packets are bucketed");
	expect_true(kPackets.transparent.size() == 2, "transparent PBR packets are bucketed");
	expect_true(
			kPackets.transparent[0].object_id == 3 && kPackets.transparent[1].object_id == 4,
			"same-depth transparent packets sort by stable object id tie-breaker");
	expect_true(
			kPackets.opaque[0].program == crimson::ShaderProgramId::OpaquePbr,
			"PBR packet uses the base shader program, not prototype object program data");
}

} // namespace
