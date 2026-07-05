#include "crimson/color/color_space.hpp"
#include "crimson/post/exposure.hpp"
#include "crimson/post/tone_mapping.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <string_view>

namespace {

void expect_true(bool condition, std::string_view message)
{
    EXPECT_TRUE(condition) << message;
}

void expect_near(float actual, float expected, float tolerance, std::string_view message)
{
    EXPECT_NEAR(actual, expected, tolerance) << message;
}

TEST(ColorSpace, SrgbReferenceConversionMatchesKnownMidpoint)
{
    const crimson::ColorLinear linear = crimson::srgb_to_linear(crimson::ColorSrgb{0.5F, 0.5F, 0.5F, 1.0F});
    expect_near(linear.r, 0.214041F, 0.0005F, "sRGB 0.5 decodes to reference linear value");
    expect_near(linear.g, linear.r, 0.00001F, "sRGB conversion is channel-stable for equal input");

    const crimson::ColorSrgb srgb = crimson::linear_to_srgb(linear);
    expect_near(srgb.r, 0.5F, 0.0005F, "linear midpoint round-trips back to sRGB");
}

TEST(ColorSpace, TextureRolesPreserveColorOrDataEncoding)
{
    expect_true(
        crimson::texture_role_uses_srgb(crimson::TextureDataRole::BaseColor),
        "base color textures use sRGB sampling");
    expect_true(
        crimson::texture_role_uses_srgb(crimson::TextureDataRole::Emissive),
        "emissive textures use sRGB sampling");

    expect_true(
        !crimson::texture_role_uses_srgb(crimson::TextureDataRole::MetallicRoughness),
        "metallic/roughness data remains linear");
    expect_true(
        !crimson::texture_role_uses_srgb(crimson::TextureDataRole::Normal),
        "normal maps remain linear data");
    expect_true(
        !crimson::texture_role_uses_srgb(crimson::TextureDataRole::Occlusion),
        "occlusion maps remain linear data");
    expect_true(
        !crimson::texture_role_uses_srgb(crimson::TextureDataRole::PickingId),
        "picking IDs are never color managed");
    expect_true(
        !crimson::texture_role_uses_srgb(crimson::TextureDataRole::Depth),
        "depth targets are data");
}

TEST(ColorSpace, ToneMappersAndExposureAreFinite)
{
    const float ev12 = crimson::exposure_multiplier_from_ev100(12.0F, 0.0F);
    const float ev12_plus_one = crimson::exposure_multiplier_from_ev100(12.0F, 1.0F);
    expect_true(std::isfinite(ev12) && ev12 > 0.0F, "EV100 exposure multiplier is finite and positive");
    expect_near(ev12_plus_one / ev12, 2.0F, 0.0005F, "one exposure compensation EV doubles exposure");

    const crimson::ColorLinear hdr{4.0F, 2.0F, 0.5F, 1.0F};
    for (const crimson::ToneMapper mapper : {
             crimson::ToneMapper::AcesFitted,
             crimson::ToneMapper::AgxApprox,
             crimson::ToneMapper::Reinhard,
             crimson::ToneMapper::Linear,
         }) {
        const crimson::ColorLinear mapped = crimson::apply_tone_mapper(mapper, hdr);
        expect_true(std::isfinite(mapped.r) && mapped.r >= 0.0F && mapped.r <= 1.0F, "tone mapper red is finite SDR");
        expect_true(std::isfinite(mapped.g) && mapped.g >= 0.0F && mapped.g <= 1.0F, "tone mapper green is finite SDR");
        expect_true(std::isfinite(mapped.b) && mapped.b >= 0.0F && mapped.b <= 1.0F, "tone mapper blue is finite SDR");
    }
}

TEST(ColorSpace, DisplayConversionSelectsOneFinalPath)
{
    crimson::DisplayConversionSettings settings;
    auto hardware = crimson::choose_display_conversion_path(true, settings);
    expect_true(
        hardware == crimson::DisplayConversionPath::SrgbBackbuffer,
        "supported preferred sRGB backbuffer chooses hardware conversion");

    auto manual = crimson::choose_display_conversion_path(false, settings);
    expect_true(
        manual == crimson::DisplayConversionPath::ManualFinalShader,
        "missing sRGB backbuffer falls back to one manual final conversion");

    settings.allow_manual_final_srgb = false;
    auto missing = crimson::choose_display_conversion_path(false, settings);
    expect_true(!missing.has_value(), "no supported final conversion path is represented explicitly");
}

} // namespace
