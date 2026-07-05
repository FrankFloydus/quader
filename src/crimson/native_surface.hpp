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

#include <string_view>

#include "crimson/renderer_types.hpp"

namespace crimson {

/// Host platform used to interpret native surface handles.
enum class NativeSurfacePlatform {
	/// Unknown or unsupported platform.
	Unknown,
	/// Win32 window handle.
	Windows,
	/// X11 window and display handles.
	LinuxX11,
	/// Wayland surface/display handles.
	LinuxWayland,
	/// macOS layer handle.
	MacOS,
};

/// Native host surface passed to Crimson at startup.
struct NativeSurfaceDescriptor {
	/// Platform for the native handles.
	NativeSurfacePlatform platform = NativeSurfacePlatform::Unknown;
	/// Native window or surface handle; required for all supported platforms.
	void *native_window_handle = nullptr;
	/// Optional native display handle for platforms that need one.
	void *native_display_handle = nullptr;
	/// Optional native layer handle for platforms that need one.
	void *native_layer_handle = nullptr;
	/// Initial render target extent.
	ViewportExtent extent;
};

/**
 * Return the stable native-surface platform name.
 *
 * @param platform Platform to name.
 * @return Static platform name.
 */
constexpr std::string_view native_surface_platform_name(NativeSurfacePlatform platform) noexcept {
	switch (platform) {
		case NativeSurfacePlatform::Unknown:
			return "Unknown";
		case NativeSurfacePlatform::Windows:
			return "Windows";
		case NativeSurfacePlatform::LinuxX11:
			return "LinuxX11";
		case NativeSurfacePlatform::LinuxWayland:
			return "LinuxWayland";
		case NativeSurfacePlatform::MacOS:
			return "MacOS";
	}

	return "Unknown";
}

/**
 * Check whether a native surface descriptor has required fields.
 *
 * @param surface Surface descriptor to validate.
 * @return True when platform, window handle, and extent are valid.
 */
constexpr bool is_valid_native_surface_descriptor(const NativeSurfaceDescriptor &surface) noexcept {
	return surface.platform != NativeSurfacePlatform::Unknown && surface.native_window_handle != nullptr && is_valid_extent(surface.extent);
}

} // namespace crimson
