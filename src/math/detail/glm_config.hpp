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

#if defined(GLM_FORCE_ALIGNED_GENTYPES) || defined(GLM_FORCE_DEFAULT_ALIGNED_GENTYPES) || defined(GLM_FORCE_INTRINSICS) || defined(GLM_FORCE_SSE2) || defined(GLM_FORCE_SSE3) || defined(GLM_FORCE_SSSE3) || defined(GLM_FORCE_SSE41) || defined(GLM_FORCE_SSE42) || defined(GLM_FORCE_AVX) || defined(GLM_FORCE_AVX2) || defined(GLM_FORCE_AVX512)
#error "Quader math wrappers must not enable GLM aligned or SIMD gentype configuration."
#endif

#ifndef GLM_FORCE_CXX20
#define GLM_FORCE_CXX20
#endif

#ifndef GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_EXPLICIT_CTOR
#endif
