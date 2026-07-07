cmake_minimum_required(VERSION 3.24)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" QUADER_VERSION LIMIT_COUNT 1)
string(STRIP "${QUADER_VERSION}" QUADER_VERSION)

project(Quader VERSION ${QUADER_VERSION} LANGUAGES C CXX)

set(QUADER_APP_TARGET quader_app)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

include(CTest)

option(QUADER_ENABLE_BENCHMARKS "Build Quader developer benchmark executables" OFF)

include(FetchContent)

set(BX_AMALGAMATED OFF CACHE BOOL "" FORCE)
set(BGFX_AMALGAMATED OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)

FetchContent_Declare(
    bgfx_cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG v1.129.8958-499
    GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(bgfx_cmake)

set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_USE_STD_FORMAT OFF CACHE BOOL "" FORCE)

set(QUADER_SPDLOG_TAG v1.15.3)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG ${QUADER_SPDLOG_TAG}
)
FetchContent_MakeAvailable(spdlog)

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(QUADER_GLM_TAG 8d1fd52e5ab5590e2c81768ace50c72bae28f2ed) # v1.0.3
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG ${QUADER_GLM_TAG}
)
FetchContent_MakeAvailable(glm)

if(TARGET glm::glm-header-only)
    set(QUADER_GLM_TARGET glm::glm-header-only)
elseif(TARGET glm::glm)
    set(QUADER_GLM_TARGET glm::glm)
else()
    message(FATAL_ERROR "GLM target was not created by FetchContent")
endif()

set(QUADER_OPENMESH_VERSION 11.0.0)
set(QUADER_OPENMESH_SHA256 9d22e65bdd6a125ac2043350a019ec4346ea83922cafdf47e125a03c16f6fa07)

set(BUILD_APPS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    openmesh
    URL "https://www.graphics.rwth-aachen.de/media/openmesh_static/Releases/11.0/OpenMesh-${QUADER_OPENMESH_VERSION}.tar.bz2"
    URL_HASH "SHA256=${QUADER_OPENMESH_SHA256}"
)
FetchContent_MakeAvailable(openmesh)

if(NOT TARGET OpenMeshCore)
    message(FATAL_ERROR "OpenMeshCore target was not created by pinned OpenMesh FetchContent")
endif()

if(MINGW)
    foreach(openmesh_target OpenMeshCore OpenMeshTools)
        if(TARGET ${openmesh_target})
            target_compile_options(${openmesh_target} PRIVATE -Wa,-mbig-obj)
        endif()
    endforeach()
endif()

if(BUILD_TESTING)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    set(QUADER_GOOGLETEST_TAG b514bdc898e2951020cbdca1304b75f5950d1f59) # v1.15.2
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG ${QUADER_GOOGLETEST_TAG}
    )
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)
endif()

function(quader_add_interface_module target_name)
    add_library(${target_name} INTERFACE)
    target_include_directories(${target_name}
        INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
endfunction()

function(quader_apply_compile_definitions target_name)
    target_compile_definitions(${target_name}
        PRIVATE
            QT_NO_KEYWORDS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
    )
endfunction()


function(quader_discover_gtests target_name)
    target_link_libraries(${target_name}
        PRIVATE
            GTest::gtest_main
    )

    quader_apply_compile_definitions(${target_name})

    gtest_discover_tests(${target_name}
        TEST_PREFIX "${target_name}."
        DISCOVERY_MODE PRE_TEST
        DISCOVERY_TIMEOUT 30
    )

    if(TARGET quader_tests)
        add_dependencies(quader_tests ${target_name})
    endif()
endfunction()

function(quader_discover_qt_gtests target_name)
    target_sources(${target_name}
        PRIVATE
            tests/unit/qt_gtest_main.cpp
    )

    target_link_libraries(${target_name}
        PRIVATE
            GTest::gtest
            Qt6::Widgets
    )

    quader_apply_compile_definitions(${target_name})

    gtest_discover_tests(${target_name}
        TEST_PREFIX "${target_name}."
        DISCOVERY_MODE PRE_TEST
        DISCOVERY_TIMEOUT 30
    )

    if(TARGET quader_tests)
        add_dependencies(quader_tests ${target_name})
    endif()
endfunction()
add_library(modeling_foundation STATIC
    src/foundation/assert.hpp
    src/foundation/diagnostic.hpp
    src/foundation/id.hpp
    src/foundation/logging.cpp
    src/foundation/logging.hpp
    src/foundation/result.hpp
)

target_include_directories(modeling_foundation
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(modeling_foundation
    PRIVATE
        spdlog::spdlog
)

quader_apply_compile_definitions(modeling_foundation)

quader_add_interface_module(modeling_math)
target_link_libraries(modeling_math
    INTERFACE
        ${QUADER_GLM_TARGET}
        modeling_foundation
)

quader_add_interface_module(modeling_geometry)
target_link_libraries(modeling_geometry
    INTERFACE
        modeling_foundation
        modeling_math
)

add_library(modeling_mesh_core STATIC
    src/mesh/core/polyhedron.cpp
    src/mesh/core/polyhedron.hpp
    src/mesh/core/mesh_attributes.hpp
    src/mesh/core/mesh_error.hpp
    src/mesh/core/mesh_ids.hpp
    src/mesh/core/mesh_traversal.hpp
    src/mesh/core/mesh_validation.cpp
    src/mesh/core/mesh_validation.hpp
    src/mesh/core/detail/openmesh_storage.cpp
    src/mesh/core/detail/openmesh_id_map.hpp
    src/mesh/core/detail/openmesh_storage.hpp
    src/mesh/core/detail/openmesh_traits.hpp
)

target_include_directories(modeling_mesh_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(modeling_mesh_core
    PUBLIC
        modeling_foundation
        modeling_math
        modeling_geometry
    PRIVATE
        OpenMeshCore
)

quader_apply_compile_definitions(modeling_mesh_core)

add_library(modeling_mesh_ops STATIC
    src/mesh/ops/box_builder.cpp
    src/mesh/ops/box_builder.hpp
)

target_include_directories(modeling_mesh_ops
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(modeling_mesh_ops
    PUBLIC
        modeling_foundation
        modeling_math
        modeling_geometry
        modeling_mesh_core
)

quader_apply_compile_definitions(modeling_mesh_ops)

add_library(modeling_document STATIC
    src/document/document.cpp
    src/document/document.hpp
    src/document/document_error.hpp
    src/document/document_events.hpp
    src/document/material.hpp
    src/document/object_id.hpp
    src/document/object_store.cpp
    src/document/object_store.hpp
    src/document/scene_object.hpp
    src/document/selection.cpp
    src/document/selection.hpp
    src/document/transform.hpp
)

target_include_directories(modeling_document
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(modeling_document
    PUBLIC
        modeling_foundation
        modeling_math
        modeling_geometry
        modeling_mesh_core
)

quader_apply_compile_definitions(modeling_document)

add_library(modeling_io STATIC
    src/io/document_fragment_validator.cpp
    src/io/document_fragment_validator.hpp
    src/io/export_options.hpp
    src/io/export_service.cpp
    src/io/export_service.hpp
    src/io/exporter.hpp
    src/io/file_format.hpp
    src/io/import_export_registry.cpp
    src/io/import_export_registry.hpp
    src/io/import_options.hpp
    src/io/import_service.cpp
    src/io/import_service.hpp
    src/io/imported_document.hpp
    src/io/importer.hpp
    src/io/io_diagnostic.cpp
    src/io/io_diagnostic.hpp
    src/io/io_error.hpp
)

target_include_directories(modeling_io
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(modeling_io
    PUBLIC
        modeling_foundation
        modeling_math
        modeling_mesh_core
        modeling_document
)

quader_apply_compile_definitions(modeling_io)

add_library(modeling_commands STATIC
    src/commands/command.cpp
    src/commands/command.hpp
    src/commands/command_history.cpp
    src/commands/command_history.hpp
    src/commands/command_result.cpp
    src/commands/command_result.hpp
    src/commands/document_commands.cpp
    src/commands/document_commands.hpp
    src/commands/selection_commands.cpp
    src/commands/selection_commands.hpp
)

target_include_directories(modeling_commands
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(modeling_commands
    PUBLIC
        modeling_foundation
        modeling_document
        modeling_mesh_ops
)

quader_apply_compile_definitions(modeling_commands)

add_library(modeling_tools STATIC
    src/tools/box_tool.cpp
    src/tools/box_tool.hpp
    src/tools/editor_input_event.hpp
    src/tools/tool.cpp
    src/tools/tool.hpp
    src/tools/tool_context.cpp
    src/tools/tool_context.hpp
    src/tools/tool_id.hpp
    src/tools/tool_manager.cpp
    src/tools/tool_manager.hpp
    src/tools/tool_preview.cpp
    src/tools/tool_preview.hpp
    src/tools/transform_gizmo_tool.cpp
    src/tools/transform_gizmo_tool.hpp
)

target_include_directories(modeling_tools
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(modeling_tools
    PUBLIC
        modeling_foundation
        modeling_math
        modeling_geometry
        modeling_document
        modeling_commands
)

quader_apply_compile_definitions(modeling_tools)

add_library(crimson STATIC
    src/crimson/color/color_space.cpp
    src/crimson/color/color_space.hpp
    src/crimson/diagnostics/diagnostic_ring.cpp
    src/crimson/diagnostics/diagnostic_ring.hpp
    src/crimson/diagnostics/frame_profiler.cpp
    src/crimson/diagnostics/frame_profiler.hpp
    src/crimson/diagnostics/renderer_diagnostics_snapshot.cpp
    src/crimson/diagnostics/renderer_diagnostics_snapshot.hpp
    src/crimson/frame/frame_builder.cpp
    src/crimson/frame/frame_builder.hpp
    src/crimson/frame/frame_render_result.hpp
    src/crimson/frame/frame_snapshot.cpp
    src/crimson/frame/frame_snapshot.hpp
    src/crimson/frame/frame_stats.cpp
    src/crimson/frame/frame_stats.hpp
    src/crimson/frame/frame_targets.cpp
    src/crimson/frame/frame_targets.hpp
    src/crimson/graph/render_graph.cpp
    src/crimson/graph/render_graph.hpp
    src/crimson/graph/render_pass.hpp
    src/crimson/graph/render_resource_desc.hpp
    src/crimson/graph/render_resource_registry.hpp
    src/crimson/material/base_shader.cpp
    src/crimson/material/base_shader.hpp
    src/crimson/material/base_shader_registry.cpp
    src/crimson/material/base_shader_registry.hpp
    src/crimson/material/default_material.cpp
    src/crimson/material/default_material.hpp
    src/crimson/material/material_instance.hpp
    src/crimson/material/material_parameter.cpp
    src/crimson/material/material_parameter.hpp
    src/crimson/material/material_system.cpp
    src/crimson/material/material_system.hpp
    src/crimson/material/material_texture.cpp
    src/crimson/material/material_texture.hpp
    src/crimson/material/material_ui_schema.hpp
    src/crimson/material/material_validation.cpp
    src/crimson/material/material_validation.hpp
    src/crimson/mesh/render_mesh.hpp
    src/crimson/mesh/render_mesh_upload.cpp
    src/crimson/mesh/render_mesh_upload.hpp
    src/crimson/native_surface.hpp
    src/crimson/overlays/grid_overlay.cpp
    src/crimson/overlays/grid_overlay.hpp
    src/crimson/overlays/overlay_command.hpp
    src/crimson/overlays/overlay_system.cpp
    src/crimson/overlays/overlay_system.hpp
    src/crimson/overlays/source_wire_visibility.cpp
    src/crimson/overlays/source_wire_visibility.hpp
    src/crimson/picking/picking.cpp
    src/crimson/picking/picking.hpp
    src/crimson/pipeline/draw_packet.cpp
    src/crimson/pipeline/draw_packet.hpp
    src/crimson/pipeline/draw_packet_sort.cpp
    src/crimson/pipeline/draw_packet_sort.hpp
    src/crimson/pipeline/frustum_culler.cpp
    src/crimson/pipeline/frustum_culler.hpp
    src/crimson/pipeline/instancing_batcher.cpp
    src/crimson/pipeline/instancing_batcher.hpp
    src/crimson/viewport_frame.hpp
    src/crimson/renderer.cpp
    src/crimson/renderer.hpp
    src/crimson/renderer_config.hpp
    src/crimson/renderer_diagnostics.hpp
    src/crimson/renderer_types.hpp
    src/crimson/post/exposure.cpp
    src/crimson/post/exposure.hpp
    src/crimson/post/tone_mapping.cpp
    src/crimson/post/tone_mapping.hpp
    src/crimson/scene/render_camera.hpp
    src/crimson/scene/render_camera_projection.cpp
    src/crimson/scene/render_camera_projection.hpp
    src/crimson/scene/render_environment.hpp
    src/crimson/scene/render_light.hpp
    src/crimson/scene/render_object.hpp
    src/crimson/scene/render_world.cpp
    src/crimson/scene/render_world.hpp
    src/crimson/scene/viewport_settings.hpp
    src/crimson/gpu/gpu_device.cpp
    src/crimson/gpu/gpu_device.hpp
    src/crimson/gpu/gpu_capabilities.cpp
    src/crimson/gpu/gpu_capabilities.hpp
    src/crimson/gpu/gpu_frame_resources.cpp
    src/crimson/gpu/gpu_frame_resources.hpp
    src/crimson/gpu/gpu_handles.hpp
    src/crimson/gpu/gpu_material_cache.cpp
    src/crimson/gpu/gpu_material_cache.hpp
    src/crimson/gpu/gpu_mesh_cache.cpp
    src/crimson/gpu/gpu_mesh_cache.hpp
    src/crimson/gpu/gpu_overlay_renderer.cpp
    src/crimson/gpu/gpu_overlay_renderer.hpp
    src/crimson/gpu/gpu_picking.cpp
    src/crimson/gpu/gpu_picking.hpp
    src/crimson/gpu/gpu_pbr_pass.cpp
    src/crimson/gpu/gpu_pbr_pass.hpp
    src/crimson/gpu/gpu_program_cache.cpp
    src/crimson/gpu/gpu_program_cache.hpp
    src/crimson/gpu/gpu_resources.cpp
    src/crimson/gpu/gpu_resources.hpp
    src/crimson/gpu/gpu_runtime_resources.hpp
    src/crimson/gpu/gpu_view_policy.hpp
    src/crimson/gpu/render_camera_projection_runtime.cpp
    src/crimson/gpu/shader_library.cpp
    src/crimson/gpu/shader_library.hpp
    src/crimson/gpu/shader_library_runtime.hpp
)

target_include_directories(crimson
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PRIVATE
        "${bgfx_cmake_SOURCE_DIR}/bimg/3rdparty/lodepng"
)

target_sources(crimson
    PRIVATE
        "${bgfx_cmake_SOURCE_DIR}/bimg/3rdparty/lodepng/lodepng.cpp"
)

target_link_libraries(crimson
    PUBLIC
        modeling_foundation
        modeling_math
        modeling_document
    PRIVATE
        bgfx
        bx
)

quader_apply_compile_definitions(crimson)

add_library(modeling_io_crimson STATIC
    src/io/gltf/gltf_material_mapper.cpp
    src/io/gltf/gltf_material_mapper.hpp
    src/io/gltf/gltf_material_source.hpp
)

target_include_directories(modeling_io_crimson
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(modeling_io_crimson
    PUBLIC
        modeling_io
        crimson
)

quader_apply_compile_definitions(modeling_io_crimson)

add_library(modeling_ui STATIC
    src/ui/delegates/property_value_delegate.cpp
    src/ui/delegates/property_value_delegate.hpp
    src/ui/actions/action_id.cpp
    src/ui/actions/action_id.hpp
    src/ui/actions/editor_action_controller.cpp
    src/ui/actions/editor_action_controller.hpp
    src/ui/actions/editor_action_state_provider.cpp
    src/ui/actions/editor_action_state_provider.hpp
    src/ui/actions/action_registry.cpp
    src/ui/actions/action_registry.hpp
    src/ui/actions/action_state_updater.cpp
    src/ui/actions/action_state_updater.hpp
    src/ui/actions/editor_state_snapshot.cpp
    src/ui/actions/editor_state_snapshot.hpp
    src/ui/models/document_item_model.cpp
    src/ui/models/document_item_model.hpp
    src/ui/models/document_selection_adapter.cpp
    src/ui/models/document_selection_adapter.hpp
    src/ui/models/diagnostics_item_model.cpp
    src/ui/models/diagnostics_item_model.hpp
    src/ui/models/item_model_roles.cpp
    src/ui/models/item_model_roles.hpp
    src/ui/models/property_descriptor.cpp
    src/ui/models/property_descriptor.hpp
    src/ui/models/property_item_model.cpp
    src/ui/models/property_item_model.hpp
    src/ui/panels/panel_context.hpp
    src/ui/panels/diagnostics_panel.cpp
    src/ui/panels/diagnostics_panel.hpp
    src/ui/panels/panel_host.cpp
    src/ui/panels/panel_host.hpp
    src/ui/panels/panel_id.cpp
    src/ui/panels/panel_id.hpp
    src/ui/panels/properties_panel.cpp
    src/ui/panels/properties_panel.hpp
    src/ui/panels/scene_panel.cpp
    src/ui/panels/scene_panel.hpp
    src/ui/qt_app/main_window.cpp
    src/ui/qt_app/main_window.hpp
    src/ui/qt_app/qt_application_bootstrap.cpp
    src/ui/qt_app/qt_application_bootstrap.hpp
    src/ui/qt_app/ui_context.hpp
    src/ui/services/document_ui_controller.cpp
    src/ui/services/document_ui_controller.hpp
    src/ui/services/file_dialog_service.cpp
    src/ui/services/file_dialog_service.hpp
    src/ui/services/import_ui_controller.cpp
    src/ui/services/import_ui_controller.hpp
    src/ui/services/notification_service.cpp
    src/ui/services/notification_service.hpp
    src/ui/services/settings_service.cpp
    src/ui/services/settings_service.hpp
    src/ui/services/viewport_diagnostics_service.cpp
    src/ui/services/viewport_diagnostics_service.hpp
    src/ui/style/fusion_style_policy.cpp
    src/ui/style/fusion_style_policy.hpp
    src/ui/style/ui_metrics.hpp
    src/ui/viewport/crimson_viewport_render_host.cpp
    src/ui/viewport/crimson_viewport_render_host.hpp
    src/ui/viewport/tool_preview_overlay_adapter.cpp
    src/ui/viewport/tool_preview_overlay_adapter.hpp
    src/ui/viewport/viewport_camera_controller.cpp
    src/ui/viewport/viewport_camera_controller.hpp
    src/ui/viewport/viewport_controller.cpp
    src/ui/viewport/viewport_controller.hpp
    src/ui/viewport/viewport_layout_state.cpp
    src/ui/viewport/viewport_layout_state.hpp
    src/ui/viewport/viewport_render_host.cpp
    src/ui/viewport/viewport_render_host.hpp
    src/ui/viewport/viewport_types.hpp
    src/ui/viewport/viewport_widget.cpp
    src/ui/viewport/viewport_widget.hpp
)

set_target_properties(modeling_ui PROPERTIES AUTOMOC ON)

target_include_directories(modeling_ui
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(modeling_ui
    PUBLIC
        modeling_math
        Qt6::Widgets
    PRIVATE
        modeling_io
        modeling_commands
        modeling_document
        modeling_tools
        crimson
)

quader_apply_compile_definitions(modeling_ui)

add_executable(${QUADER_APP_TARGET}
    src/app/app_services.hpp
    src/app/application.cpp
    src/app/application.hpp
    src/app/application_config.hpp
    src/main.cpp
)

set_target_properties(${QUADER_APP_TARGET}
    PROPERTIES
        OUTPUT_NAME quader
)

target_link_libraries(${QUADER_APP_TARGET}
    PRIVATE
        modeling_commands
        modeling_document
        modeling_io
        modeling_tools
        modeling_ui
        Qt6::Widgets
)

quader_apply_compile_definitions(${QUADER_APP_TARGET})

add_custom_target(check_architecture
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_architecture.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Checking architecture include boundaries"
    VERBATIM
)

set(QUADER_CLANG_FORMAT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_clang_format.py")
set(QUADER_CLANG_TIDY_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_clang_tidy.py")
set(QUADER_QT_LLVM_BIN_DIR "C:/Qt/Tools/QtCreator/bin/clang/bin")
set(QUADER_CLANG_TIDY_JOBS "0" CACHE STRING "Parallel clang-tidy jobs. Use 0 for all logical cores.")

find_program(QUADER_QT_CLANG_FORMAT_EXECUTABLE
    NAMES clang-format
    HINTS "${QUADER_QT_LLVM_BIN_DIR}"
    NO_DEFAULT_PATH
)
find_program(QUADER_PATH_CLANG_FORMAT_EXECUTABLE
    NAMES clang-format
)
if(QUADER_QT_CLANG_FORMAT_EXECUTABLE)
    set(QUADER_CLANG_FORMAT_EXECUTABLE "${QUADER_QT_CLANG_FORMAT_EXECUTABLE}" CACHE FILEPATH "Path to clang-format." FORCE)
elseif(QUADER_PATH_CLANG_FORMAT_EXECUTABLE)
    set(QUADER_CLANG_FORMAT_EXECUTABLE "${QUADER_PATH_CLANG_FORMAT_EXECUTABLE}" CACHE FILEPATH "Path to clang-format." FORCE)
endif()

find_program(QUADER_QT_CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy
    HINTS "${QUADER_QT_LLVM_BIN_DIR}"
    NO_DEFAULT_PATH
)
find_program(QUADER_PATH_CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy
)
if(QUADER_QT_CLANG_TIDY_EXECUTABLE)
    set(QUADER_CLANG_TIDY_EXECUTABLE "${QUADER_QT_CLANG_TIDY_EXECUTABLE}" CACHE FILEPATH "Path to clang-tidy." FORCE)
elseif(QUADER_PATH_CLANG_TIDY_EXECUTABLE)
    set(QUADER_CLANG_TIDY_EXECUTABLE "${QUADER_PATH_CLANG_TIDY_EXECUTABLE}" CACHE FILEPATH "Path to clang-tidy." FORCE)
endif()

set(QUADER_CLANG_FORMAT_ARGS)
if(QUADER_CLANG_FORMAT_EXECUTABLE)
    list(APPEND QUADER_CLANG_FORMAT_ARGS --clang-format "${QUADER_CLANG_FORMAT_EXECUTABLE}")
endif()

set(QUADER_CLANG_TIDY_ARGS)
if(QUADER_CLANG_TIDY_EXECUTABLE)
    list(APPEND QUADER_CLANG_TIDY_ARGS --clang-tidy "${QUADER_CLANG_TIDY_EXECUTABLE}")
endif()
list(APPEND QUADER_CLANG_TIDY_ARGS --jobs "${QUADER_CLANG_TIDY_JOBS}")

add_custom_target(check_format
    COMMAND "${Python3_EXECUTABLE}" "${QUADER_CLANG_FORMAT_SCRIPT}"
        --root "${CMAKE_CURRENT_SOURCE_DIR}"
        ${QUADER_CLANG_FORMAT_ARGS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Checking clang-format style"
    VERBATIM
)

add_custom_target(check_clang_tidy
    COMMAND "${Python3_EXECUTABLE}" "${QUADER_CLANG_TIDY_SCRIPT}"
        --root "${CMAKE_CURRENT_SOURCE_DIR}"
        --build-dir "${CMAKE_CURRENT_BINARY_DIR}"
        ${QUADER_CLANG_TIDY_ARGS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Checking clang-tidy diagnostics"
    VERBATIM
)

add_custom_target(check_static_analysis)
add_dependencies(check_static_analysis
    check_architecture
    check_format
    check_clang_tidy
)

if(BUILD_TESTING)
    add_custom_target(quader_tests)

    add_executable(foundation_tests
        tests/unit/foundation/foundation_tests.cpp
    )

    target_link_libraries(foundation_tests
        PRIVATE
            modeling_foundation
    )

    quader_discover_gtests(foundation_tests)

    add_executable(math_geometry_tests
        tests/unit/math/math_geometry_tests.cpp
    )

    target_link_libraries(math_geometry_tests
        PRIVATE
            modeling_geometry
    )

    quader_discover_gtests(math_geometry_tests)

    add_executable(mesh_core_tests
        tests/unit/mesh/mesh_corruption_fixtures.hpp
        tests/unit/mesh/mesh_core_tests.cpp
        tests/unit/mesh/mesh_fixtures.hpp
    )

    target_link_libraries(mesh_core_tests
        PRIVATE
            modeling_mesh_core
    )

    target_compile_definitions(mesh_core_tests
        PRIVATE
            QUADER_MESH_CORE_TEST_HOOKS
    )

    quader_discover_gtests(mesh_core_tests)

    add_executable(mesh_ops_tests
        tests/unit/mesh/mesh_ops_tests.cpp
    )

    target_link_libraries(mesh_ops_tests
        PRIVATE
            modeling_mesh_ops
    )

    quader_discover_gtests(mesh_ops_tests)

    add_executable(document_tests
        tests/unit/document/document_test_helpers.hpp
        tests/unit/document/document_tests.cpp
    )

    target_link_libraries(document_tests
        PRIVATE
            modeling_document
    )

    quader_discover_gtests(document_tests)

    add_executable(io_diagnostic_tests
        tests/unit/io/io_diagnostic_tests.cpp
    )

    target_link_libraries(io_diagnostic_tests
        PRIVATE
            modeling_io
    )

    quader_discover_gtests(io_diagnostic_tests)

    add_executable(io_registry_tests
        tests/unit/io/io_registry_tests.cpp
    )

    target_link_libraries(io_registry_tests
        PRIVATE
            modeling_io
    )

    quader_discover_gtests(io_registry_tests)

    add_executable(io_import_export_service_tests
        tests/unit/io/io_import_export_service_tests.cpp
        tests/unit/mesh/mesh_corruption_fixtures.hpp
    )

    target_link_libraries(io_import_export_service_tests
        PRIVATE
            modeling_io
    )

    target_compile_definitions(io_import_export_service_tests
        PRIVATE
            QUADER_MESH_CORE_TEST_HOOKS
    )

    quader_discover_gtests(io_import_export_service_tests)

    add_executable(io_document_fragment_tests
        tests/unit/io/io_document_fragment_tests.cpp
        tests/unit/document/document_test_helpers.hpp
        tests/unit/mesh/mesh_corruption_fixtures.hpp
    )

    target_link_libraries(io_document_fragment_tests
        PRIVATE
            modeling_io
    )

    target_compile_definitions(io_document_fragment_tests
        PRIVATE
            QUADER_MESH_CORE_TEST_HOOKS
    )

    quader_discover_gtests(io_document_fragment_tests)

    add_executable(gltf_material_mapper_tests
        tests/unit/io/gltf_material_mapper_tests.cpp
    )

    target_link_libraries(gltf_material_mapper_tests
        PRIVATE
            modeling_io_crimson
    )

    quader_discover_gtests(gltf_material_mapper_tests)

    add_executable(command_tests
        tests/unit/commands/command_tests.cpp
        tests/unit/document/document_test_helpers.hpp
    )

    target_link_libraries(command_tests
        PRIVATE
            modeling_commands
    )

    quader_discover_gtests(command_tests)

    add_executable(tool_manager_tests
        tests/unit/document/document_test_helpers.hpp
        tests/unit/tools/tool_manager_tests.cpp
    )

    target_link_libraries(tool_manager_tests
        PRIVATE
            modeling_tools
    )

    quader_discover_gtests(tool_manager_tests)

    add_executable(crimson_renderer_public_types_tests
        tests/unit/crimson/renderer_public_types_tests.cpp
    )

    target_link_libraries(crimson_renderer_public_types_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_renderer_public_types_tests)

    add_executable(crimson_gpu_shell_tests
        tests/unit/crimson/gpu_shell_tests.cpp
    )

    target_link_libraries(crimson_gpu_shell_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_gpu_shell_tests)

    add_executable(crimson_backend_capability_tests
        tests/unit/crimson/backend_capability_tests.cpp
    )

    target_link_libraries(crimson_backend_capability_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_backend_capability_tests)

    add_executable(crimson_gpu_resource_tests
        tests/unit/crimson/gpu_resource_tests.cpp
    )

    target_link_libraries(crimson_gpu_resource_tests
        PRIVATE
            crimson
            bgfx
            bx
    )

    quader_discover_gtests(crimson_gpu_resource_tests)

    add_executable(crimson_frame_stats_diagnostics_tests
        tests/unit/crimson/frame_stats_diagnostics_tests.cpp
    )

    target_link_libraries(crimson_frame_stats_diagnostics_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_frame_stats_diagnostics_tests)

    add_executable(crimson_render_packet_pipeline_tests
        tests/unit/crimson/render_packet_pipeline_tests.cpp
    )

    target_link_libraries(crimson_render_packet_pipeline_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_render_packet_pipeline_tests)

    add_executable(crimson_render_upload_tests
        tests/unit/crimson/render_upload_tests.cpp
    )

    target_link_libraries(crimson_render_upload_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_render_upload_tests)

    add_executable(crimson_frame_snapshot_tests
        tests/unit/crimson/frame_snapshot_tests.cpp
    )

    target_link_libraries(crimson_frame_snapshot_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_frame_snapshot_tests)

    add_executable(crimson_render_graph_tests
        tests/unit/crimson/render_graph_tests.cpp
    )

    target_link_libraries(crimson_render_graph_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_render_graph_tests)

    add_executable(crimson_render_graph_correctness_tests
        tests/unit/crimson/render_graph_correctness_tests.cpp
    )

    target_link_libraries(crimson_render_graph_correctness_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_render_graph_correctness_tests)

    add_executable(crimson_color_space_tests
        tests/unit/crimson/color_space_tests.cpp
    )

    target_link_libraries(crimson_color_space_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_color_space_tests)

    add_executable(crimson_shader_manifest_tests
        tests/unit/crimson/shader_manifest_tests.cpp
    )

    target_link_libraries(crimson_shader_manifest_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_shader_manifest_tests)

    add_executable(crimson_material_system_tests
        tests/unit/crimson/material_system_tests.cpp
    )

    target_link_libraries(crimson_material_system_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_material_system_tests)

    add_executable(crimson_overlay_tests
        tests/unit/crimson/overlay_tests.cpp
    )

    target_link_libraries(crimson_overlay_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_overlay_tests)

    add_executable(crimson_picking_tests
        tests/unit/crimson/picking_tests.cpp
    )

    target_link_libraries(crimson_picking_tests
        PRIVATE
            crimson
    )

    quader_discover_gtests(crimson_picking_tests)

    add_executable(golden_image_compare_tests
        tests/render_tests/golden_image.cpp
        tests/render_tests/golden_image.hpp
        tests/render_tests/golden_image_compare_tests.cpp
        tests/render_tests/render_test_scene.cpp
        tests/render_tests/render_test_scene.hpp
    )

    target_include_directories(golden_image_compare_tests
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/render_tests"
    )

    quader_discover_gtests(golden_image_compare_tests)

    add_executable(ui_shell_tests
        tests/unit/ui/ui_shell_tests.cpp
    )

    target_link_libraries(ui_shell_tests
        PRIVATE
            modeling_commands
            modeling_document
            modeling_io
            modeling_ui
            Qt6::Widgets
    )

    quader_discover_qt_gtests(ui_shell_tests)

    add_executable(ui_action_wiring_tests
        tests/unit/document/document_test_helpers.hpp
        tests/unit/ui/ui_action_wiring_tests.cpp
    )

    target_link_libraries(ui_action_wiring_tests
        PRIVATE
            modeling_commands
            modeling_io
            modeling_tools
            modeling_ui
            Qt6::Widgets
    )

    quader_discover_qt_gtests(ui_action_wiring_tests)

    add_executable(ui_file_dialog_service_tests
        tests/unit/ui/ui_file_dialog_service_tests.cpp
    )

    target_link_libraries(ui_file_dialog_service_tests
        PRIVATE
            modeling_ui
            Qt6::Widgets
    )

    quader_discover_qt_gtests(ui_file_dialog_service_tests)

    add_executable(ui_import_controller_tests
        tests/unit/ui/ui_import_controller_tests.cpp
    )

    target_link_libraries(ui_import_controller_tests
        PRIVATE
            modeling_io
            modeling_ui
            Qt6::Widgets
    )

    quader_discover_qt_gtests(ui_import_controller_tests)

    add_executable(ui_model_tests
        tests/unit/document/document_test_helpers.hpp
        tests/unit/ui/ui_model_tests.cpp
    )

    target_link_libraries(ui_model_tests
        PRIVATE
            modeling_commands
            modeling_document
            modeling_ui
            Qt6::Widgets
            Qt6::Test
    )

    quader_discover_qt_gtests(ui_model_tests)

    add_executable(ui_diagnostics_model_tests
        tests/unit/ui/ui_diagnostics_model_tests.cpp
    )

    target_link_libraries(ui_diagnostics_model_tests
        PRIVATE
            modeling_ui
            Qt6::Widgets
            Qt6::Test
    )

    quader_discover_qt_gtests(ui_diagnostics_model_tests)

    add_executable(ui_viewport_diagnostics_service_tests
        tests/unit/ui/ui_viewport_diagnostics_service_tests.cpp
    )

    target_link_libraries(ui_viewport_diagnostics_service_tests
        PRIVATE
            modeling_ui
            Qt6::Widgets
    )

    quader_discover_qt_gtests(ui_viewport_diagnostics_service_tests)

    add_executable(ui_viewport_tests
        tests/unit/ui/viewport_tests.cpp
    )

    target_link_libraries(ui_viewport_tests
        PRIVATE
            modeling_ui
            Qt6::Widgets
    )

    quader_discover_qt_gtests(ui_viewport_tests)

    add_test(
        NAME architecture_boundaries
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_architecture.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_test(
        NAME clang_format
        COMMAND "${Python3_EXECUTABLE}" "${QUADER_CLANG_FORMAT_SCRIPT}"
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
            ${QUADER_CLANG_FORMAT_ARGS}
    )
    set_tests_properties(clang_format PROPERTIES LABELS style)

    add_test(
        NAME clang_tidy
        COMMAND "${Python3_EXECUTABLE}" "${QUADER_CLANG_TIDY_SCRIPT}"
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
            --build-dir "${CMAKE_CURRENT_BINARY_DIR}"
            ${QUADER_CLANG_TIDY_ARGS}
    )
    set_tests_properties(clang_tidy PROPERTIES LABELS style)
endif()

add_executable(quader_renderer_benchmarks EXCLUDE_FROM_ALL
    benchmarks/benchmark_main.cpp
    benchmarks/benchmark_main.hpp
    benchmarks/crimson_packet_pipeline_bench.cpp
    benchmarks/crimson_upload_bench.cpp
)

target_link_libraries(quader_renderer_benchmarks
    PRIVATE
        crimson
)

quader_apply_compile_definitions(quader_renderer_benchmarks)

add_custom_target(quader_benchmarks)
add_dependencies(quader_benchmarks quader_renderer_benchmarks)

if(QUADER_ENABLE_BENCHMARKS)
    add_custom_target(quader_benchmarks_all ALL)
    add_dependencies(quader_benchmarks_all quader_renderer_benchmarks)
endif()

if(WIN32)
    set(BGFX_SHADER_PLATFORM windows)
elseif(APPLE)
    set(BGFX_SHADER_PLATFORM osx)
else()
    set(BGFX_SHADER_PLATFORM linux)
endif()

set(BGFX_SHADER_INCLUDE_DIR "${bgfx_cmake_SOURCE_DIR}/bgfx/src")
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(COMPILED_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders")
set(SHADER_MANIFEST "${SHADER_SOURCE_DIR}/manifest.json")
set(SHADER_COMPILE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/build_tools/compile_shaders.py")
set(SHADER_VALIDATE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/build_tools/validate_shader_manifest.py")
set(SHADER_STAMP "${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders.stamp")
set(RUNTIME_SHADER_STAMP "${CMAKE_CURRENT_BINARY_DIR}/runtime_shaders.stamp")
set(MODELING_ASSET_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/modeling_assets")
set(RUNTIME_MODELING_ASSET_STAMP "${CMAKE_CURRENT_BINARY_DIR}/runtime_modeling_assets.stamp")
set(MODELING_ASSET_FILES
    "${MODELING_ASSET_SOURCE_DIR}/materials/default/default_albedo.png"
    "${MODELING_ASSET_SOURCE_DIR}/materials/default/default_roughness.png"
)
set(SHADER_SOURCE_FILES
    "${SHADER_SOURCE_DIR}/common/color_space.sh"
    "${SHADER_SOURCE_DIR}/overlay_unlit.fs.sc"
    "${SHADER_SOURCE_DIR}/overlay_edit_line.fs.sc"
    "${SHADER_SOURCE_DIR}/overlay_edit_line.vs.sc"
    "${SHADER_SOURCE_DIR}/overlay_line.fs.sc"
    "${SHADER_SOURCE_DIR}/overlay_line.vs.sc"
    "${SHADER_SOURCE_DIR}/overlay_device_solid.vs.sc"
    "${SHADER_SOURCE_DIR}/overlay_solid.fs.sc"
    "${SHADER_SOURCE_DIR}/overlay_solid.vs.sc"
    "${SHADER_SOURCE_DIR}/pbr/alpha_cutout_pbr.fs.sc"
    "${SHADER_SOURCE_DIR}/pbr/alpha_cutout_pbr.vs.sc"
    "${SHADER_SOURCE_DIR}/pbr/opaque_pbr.fs.sc"
    "${SHADER_SOURCE_DIR}/pbr/opaque_pbr.vs.sc"
    "${SHADER_SOURCE_DIR}/pbr/unlit_surface.fs.sc"
    "${SHADER_SOURCE_DIR}/picking/picking.fs.sc"
    "${SHADER_SOURCE_DIR}/picking/picking.vs.sc"
    "${SHADER_SOURCE_DIR}/pbr/transparent_pbr.fs.sc"
    "${SHADER_SOURCE_DIR}/pbr/transparent_pbr.vs.sc"
    "${SHADER_SOURCE_DIR}/post/fullscreen_triangle.vs.sc"
    "${SHADER_SOURCE_DIR}/post/present.fs.sc"
    "${SHADER_SOURCE_DIR}/post/tone_map.fs.sc"
    "${SHADER_SOURCE_DIR}/varying.def.sc"
    "${SHADER_SOURCE_DIR}/overlay_unlit.vs.sc"
)

add_custom_command(
    OUTPUT "${SHADER_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${COMPILED_SHADER_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${COMPILED_SHADER_DIR}"
    COMMAND "${Python3_EXECUTABLE}" "${SHADER_COMPILE_SCRIPT}"
        --manifest "${SHADER_MANIFEST}"
        --shaderc "$<TARGET_FILE:shaderc>"
        --source-root "${SHADER_SOURCE_DIR}"
        --output "${COMPILED_SHADER_DIR}"
        --include "${BGFX_SHADER_INCLUDE_DIR}"
        --varyingdef "${SHADER_SOURCE_DIR}/varying.def.sc"
        --host-platform "${BGFX_SHADER_PLATFORM}"
    COMMAND "${Python3_EXECUTABLE}" "${SHADER_VALIDATE_SCRIPT}"
        --manifest "${SHADER_MANIFEST}"
        --compiled "${COMPILED_SHADER_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch "${SHADER_STAMP}"
    DEPENDS
        shaderc
        "${SHADER_COMPILE_SCRIPT}"
        "${SHADER_VALIDATE_SCRIPT}"
        "${SHADER_MANIFEST}"
        ${SHADER_SOURCE_FILES}
    VERBATIM
)

add_custom_target(compile_shaders
    DEPENDS
        "${SHADER_STAMP}"
)
add_dependencies(${QUADER_APP_TARGET} compile_shaders)

add_custom_command(
    OUTPUT "${RUNTIME_SHADER_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${QUADER_APP_TARGET}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${COMPILED_SHADER_DIR}"
        "$<TARGET_FILE_DIR:${QUADER_APP_TARGET}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E touch "${RUNTIME_SHADER_STAMP}"
    DEPENDS
        ${QUADER_APP_TARGET}
        "${SHADER_STAMP}"
    VERBATIM
)

add_custom_target(copy_runtime_shaders ALL
    DEPENDS
        "${RUNTIME_SHADER_STAMP}"
)

add_custom_command(
    OUTPUT "${RUNTIME_MODELING_ASSET_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${QUADER_APP_TARGET}>/resources"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${QUADER_APP_TARGET}>/resources/modeling_assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${MODELING_ASSET_SOURCE_DIR}"
        "$<TARGET_FILE_DIR:${QUADER_APP_TARGET}>/resources/modeling_assets"
    COMMAND ${CMAKE_COMMAND} -E touch "${RUNTIME_MODELING_ASSET_STAMP}"
    DEPENDS
        ${QUADER_APP_TARGET}
        ${MODELING_ASSET_FILES}
    VERBATIM
)

add_custom_target(copy_runtime_modeling_assets ALL
    DEPENDS
        "${RUNTIME_MODELING_ASSET_STAMP}"
)

add_custom_target(quader_app_runtime)
add_dependencies(quader_app_runtime copy_runtime_shaders)
add_dependencies(quader_app_runtime copy_runtime_modeling_assets)

if(BUILD_TESTING)
    add_test(
        NAME crimson_shader_manifest_compiled_outputs
        COMMAND "${Python3_EXECUTABLE}" "${SHADER_VALIDATE_SCRIPT}"
            --manifest "${SHADER_MANIFEST}"
            --compiled "${COMPILED_SHADER_DIR}"
    )
endif()

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${Qt6_DIR}/../../../bin"
    )

    if(WINDEPLOYQT_EXECUTABLE)
        get_filename_component(QT_BIN_DIR "${WINDEPLOYQT_EXECUTABLE}" DIRECTORY)
        get_filename_component(COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

        add_custom_target(deploy
            COMMAND ${CMAKE_COMMAND} -E env
                "PATH=${COMPILER_BIN_DIR};${QT_BIN_DIR};$ENV{PATH}"
                "${WINDEPLOYQT_EXECUTABLE}"
                --compiler-runtime
                "$<TARGET_FILE:${QUADER_APP_TARGET}>"
            DEPENDS
                ${QUADER_APP_TARGET}
                copy_runtime_shaders
                copy_runtime_modeling_assets
            COMMENT "Deploying Qt and MinGW runtime DLLs"
            VERBATIM
        )
    endif()
endif()
