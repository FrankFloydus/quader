# Implementation Plan: Task #10 I/O And Asset Boundaries

Board task: `#10 Add I/O and asset boundaries`
Assignment: `A48`
Source authority: `agents/plans/audit_20260704_full_architecture_master.md` Batch 9
Owning architect: software
Renderer handoff: Crimson material schema mapping only

## Read Context

This plan was prepared after reading:

- `AGENTS.md`
- `agents/workflow.md`
- `agents/task_board.md`
- `agents/tests-policy.md`
- `project_board.md`
- `agents/plans/audit_20260704_full_architecture_master.md`
- `agents/architecture/architecture.md`
- `agents/architecture/renderer.md`

Current source context:

- Module targets already exist for `modeling_foundation`, `modeling_math`, `modeling_geometry`,
  `modeling_mesh_core`, `modeling_mesh_ops`, `modeling_document`, `modeling_commands`,
  `modeling_tools`, `crimson`, `modeling_ui`, and the app executable.
- There is no `modeling_io` target yet.
- `Document::create_mesh_object()` already validates mesh topology through `mesh::validate_mesh()`
  before accepting editable state.
- Crimson V1 material schemas exist under `src/crimson/material/` and expose
  `BaseShaderRegistry`, `BaseShaderDefinition`, `MaterialInstance`, material parameters, texture
  slots, and renderer-safe handles.
- UI has `ActionId::OpenScene`, `SaveScene`, and `SaveSceneAs`, but no file-dialog service or I/O
  controller yet.

## Goals

Implement the architecture boundaries for I/O and asset handoff without building a full asset
pipeline.

Required outcomes:

- Add importer/exporter interfaces and static registration.
- Define document and document-fragment import contracts.
- Use structured diagnostics for import, export, validation, and material mapping.
- Add a UI file-dialog service boundary so file selection stays in `modeling_ui`.
- Ensure imported mesh data validates before it can become editable document state.
- Add deterministic glTF-to-Crimson material mapping through Crimson public material schemas.
- Keep import/export code free of Qt Widgets and UI dependencies.
- Keep material mapping free of graphics-runtime internals and `src/crimson/gpu/` includes.

Non-goals for this task:

- Do not add a dynamic plugin system.
- Do not add full glTF geometry parsing unless separately approved with an explicit dependency
  decision.
- Do not add texture copying, project-local asset catalogs, multi-scene projects, or project
  manifests. Those belong to task #18.
- Do not add material editing UI or document-backed material assignment unless a follow-up task
  authorizes it.
- Do not expose bgfx/bx or other graphics-runtime handles through I/O, document, UI, or Crimson
  public material mapping contracts.
- Do not archive dev builds or bump version metadata.

## Dependency Direction

Add these targets:

```text
modeling_io
  PUBLIC: modeling_foundation, modeling_math, modeling_mesh_core, modeling_document
  PRIVATE: standard library only at first

modeling_io_crimson
  PUBLIC: modeling_io, crimson
  Purpose: adapter for deterministic glTF material descriptors to Crimson MaterialInstance.
  Restriction: may include src/crimson/material/* and src/crimson/renderer_types.hpp only.
  Forbidden: src/crimson/gpu/*, bgfx/*, bx/*, Qt Widgets, UI headers.

modeling_ui
  PRIVATE: modeling_io, modeling_io_crimson only if UI import wiring needs the material mapper.
  UI selects paths and displays diagnostics; it does not parse file formats.

app executable
  Owns concrete service construction and static registration.
```

Rationale:

- `modeling_io` stays independent of UI and renderer.
- `modeling_io_crimson` isolates the cross-domain material schema adapter required by task #10.
  This avoids making all I/O depend on Crimson while keeping glTF material mapping out of renderer
  core, as required by `renderer.md`.
- `crimson` must not depend on `modeling_io`.
- `commands` may depend on `modeling_io` only for a narrow import-fragment command if the UI import
  action mutates the active document during this task. If the implementation keeps this task to
  boundaries only, command integration can be deferred, but no UI direct mutation is allowed.

Update `tools/check_architecture.py`:

- Add `QFileDialog` to `QT_WIDGETS_HEADERS`.
- Reject Qt Widgets includes under `src/io/`.
- Reject `src/ui/` includes under `src/io/`.
- Reject graphics-runtime includes under `src/io/`.
- Reject `src/crimson/gpu/` includes from `src/io/` and `src/io/gltf/`.
- Optionally allow `src/io/gltf/*` to include only Crimson public material headers:
  `crimson/material/*` and `crimson/renderer_types.hpp`.

## Core Contracts

### I/O Diagnostics

Files:

```text
src/io/io_diagnostic.hpp
src/io/io_diagnostic.cpp
src/io/io_error.hpp
```

Types:

```cpp
namespace quader::io {

enum class IoSeverity {
    Info,
    Warning,
    Error,
};

enum class IoDiagnosticCode {
    Unknown,
    UnsupportedFormat,
    FileNotFound,
    FileReadFailed,
    FileWriteFailed,
    ParseFailed,
    ValidationFailed,
    MeshValidationFailed,
    DocumentValidationFailed,
    MaterialMappingFailed,
    UnsupportedFeaturePreserved,
    ExportNotSupported,
};

struct SourceLocation {
    std::uint32_t line = 0;
    std::uint32_t column = 0;
    std::string json_pointer;
};

struct IoDiagnostic {
    IoSeverity severity = IoSeverity::Info;
    IoDiagnosticCode code = IoDiagnosticCode::Unknown;
    std::string message;
    std::filesystem::path source_path;
    SourceLocation location;
    std::string subject;
};

class IoDiagnosticList final {
public:
    void add(IoDiagnostic diagnostic);
    void add_error(IoDiagnosticCode code, std::string message);
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool has_errors() const noexcept;
    [[nodiscard]] std::span<const IoDiagnostic> diagnostics() const noexcept;
};

enum class IoErrorCode {
    UnsupportedFormat,
    FileAccessFailed,
    ParseFailed,
    ValidationFailed,
    ExportFailed,
};

struct IoError {
    IoErrorCode code = IoErrorCode::UnsupportedFormat;
    IoDiagnostic diagnostic;
    IoDiagnosticList related;
};

} // namespace quader::io
```

Implementation notes:

- Use `std::filesystem::path` only in I/O and UI adapters. Domain objects should not store raw
  absolute paths as truth.
- Do not reuse `foundation::DiagnosticList` directly for I/O because import/export diagnostics need
  source path, source location, format subject, and stable diagnostic codes.
- Provide conversion helpers to `foundation::Diagnostic` for UI status summaries and logs.
- No Qt types in these headers.

### Import File Metadata

Files:

```text
src/io/file_format.hpp
src/io/import_options.hpp
src/io/export_options.hpp
```

Types:

```cpp
namespace quader::io {

enum class DocumentImportMode {
    ReplaceDocument,
    AppendFragment,
};

struct FileFormatDescriptor {
    std::string id;              // "gltf", "quader-scene", "obj", etc.
    std::string display_name;    // "glTF 2.0"
    std::vector<std::string> extensions; // lowercase without dot
    bool can_import_document = false;
    bool can_import_fragment = false;
    bool can_export_document = false;
    bool can_export_fragment = false;
};

struct ImportOptions {
    DocumentImportMode mode = DocumentImportMode::AppendFragment;
    bool preserve_unknown_material_metadata = true;
    bool validate_meshes_before_success = true;
};

struct ExportOptions {
    bool deterministic = true;
    std::string line_ending = "\n";
};

} // namespace quader::io
```

Rules:

- Extension matching must be case-insensitive and deterministic.
- `ExportOptions::deterministic` is not optional for tests. If a format cannot support stable
  output, its exporter must say so through diagnostics.

### Imported Document And Fragment Contracts

Files:

```text
src/io/imported_document.hpp
src/io/document_fragment_validator.hpp
src/io/document_fragment_validator.cpp
```

Types:

```cpp
namespace quader::io {

struct ImportedMaterialMetadata {
    std::string source_name;
    std::string source_format;
    std::string source_material_id;
    std::vector<std::string> preserved_extensions;
    IoDiagnosticList diagnostics;
};

struct ImportedMeshObject {
    std::string name;
    quader::mesh::HalfEdgeMesh mesh;
    quader::document::Transform transform;
    std::optional<std::size_t> material_index;
};

struct DocumentFragment {
    std::vector<ImportedMeshObject> mesh_objects;
    std::vector<ImportedMaterialMetadata> materials;
    IoDiagnosticList diagnostics;
};

struct ImportedDocument {
    DocumentFragment root_fragment;
    std::string source_name;
    IoDiagnosticList diagnostics;
};

enum class FragmentValidationCode {
    EmptyFragment,
    InvalidMesh,
    InvalidTransform,
    InvalidMaterialReference,
};

struct FragmentValidationResult {
    IoDiagnosticList diagnostics;
    [[nodiscard]] bool ok() const noexcept;
};

[[nodiscard]] FragmentValidationResult validate_document_fragment(const DocumentFragment& fragment);
[[nodiscard]] quader::foundation::Result<quader::document::Document, IoError>
build_document_from_import(ImportedDocument imported);

} // namespace quader::io
```

Validation rules:

- Every `ImportedMeshObject::mesh` must pass `quader::mesh::validate_mesh()`.
- Every transform must pass `document::is_finite()`.
- `material_index`, when present, must point inside `DocumentFragment::materials`.
- Empty fragments are warnings for append previews but errors for document replacement.
- Validation must report all object-level validation errors where practical, not just the first
  failure.
- `build_document_from_import()` must call `Document::create_mesh_object()` so the document layer
  remains the final editable-state gate.

Important:

- Do not assign document `ObjectId` values inside imported fragments. Object IDs are created by
  `Document` when the fragment is applied.
- Do not bypass `Document::create_mesh_object()` by writing into `ObjectStore`.
- Do not store Crimson handles in imported document data. Material metadata may carry source
  material records and deterministic mapping diagnostics only.

### Importer And Exporter Interfaces

Files:

```text
src/io/importer.hpp
src/io/exporter.hpp
src/io/import_export_registry.hpp
src/io/import_export_registry.cpp
src/io/import_service.hpp
src/io/import_service.cpp
src/io/export_service.hpp
src/io/export_service.cpp
```

Interfaces:

```cpp
namespace quader::io {

struct ImportRequest {
    std::filesystem::path path;
    ImportOptions options;
};

struct ImportResult {
    std::variant<ImportedDocument, DocumentFragment> payload;
    IoDiagnosticList diagnostics;
};

class IImporter {
public:
    virtual ~IImporter() = default;
    [[nodiscard]] virtual FileFormatDescriptor descriptor() const = 0;
    [[nodiscard]] virtual bool can_import(const std::filesystem::path& path) const = 0;
    [[nodiscard]] virtual quader::foundation::Result<ImportResult, IoError>
    import_file(const ImportRequest& request) const = 0;
};

struct ExportRequest {
    std::filesystem::path path;
    const quader::document::Document& document;
    ExportOptions options;
};

struct ExportResult {
    IoDiagnosticList diagnostics;
};

class IExporter {
public:
    virtual ~IExporter() = default;
    [[nodiscard]] virtual FileFormatDescriptor descriptor() const = 0;
    [[nodiscard]] virtual bool can_export(const std::filesystem::path& path) const = 0;
    [[nodiscard]] virtual quader::foundation::Result<ExportResult, IoError>
    export_file(const ExportRequest& request) const = 0;
};

class ImportExportRegistry final {
public:
    [[nodiscard]] quader::foundation::Result<void, IoError>
    register_importer(std::unique_ptr<IImporter> importer);
    [[nodiscard]] quader::foundation::Result<void, IoError>
    register_exporter(std::unique_ptr<IExporter> exporter);

    [[nodiscard]] const IImporter* importer_for_path(const std::filesystem::path& path) const;
    [[nodiscard]] const IExporter* exporter_for_path(const std::filesystem::path& path) const;
    [[nodiscard]] std::vector<FileFormatDescriptor> import_formats() const;
    [[nodiscard]] std::vector<FileFormatDescriptor> export_formats() const;
};

class ImportService final {
public:
    explicit ImportService(const ImportExportRegistry& registry);
    [[nodiscard]] quader::foundation::Result<ImportResult, IoError>
    import_file(const ImportRequest& request) const;
};

class ExportService final {
public:
    explicit ExportService(const ImportExportRegistry& registry);
    [[nodiscard]] quader::foundation::Result<ExportResult, IoError>
    export_file(const ExportRequest& request) const;
};

} // namespace quader::io
```

Rules:

- Registry owns importers/exporters through `std::unique_ptr`.
- Registration order must be stable and preserved for file-dialog filter generation.
- Duplicate format IDs or duplicate extension claims must fail with `IoErrorCode::ValidationFailed`.
- `ImportService::import_file()` must reject unsupported extensions before opening files.
- `ImportService::import_file()` must validate returned fragments/documents before returning
  success.
- `ExportService::export_file()` must treat deterministic output as the default.
- Neither service may show message boxes, open dialogs, or depend on Qt.

### Optional Import Command

Add only if this task wires the UI open/import action to mutate the active document.

Files:

```text
src/commands/import_commands.hpp
src/commands/import_commands.cpp
```

Type:

```cpp
namespace quader::commands {

class ImportFragmentCommand final : public ICommand {
public:
    explicit ImportFragmentCommand(quader::io::DocumentFragment fragment);

    [[nodiscard]] CommandResult execute(quader::document::Document& document) override;
    [[nodiscard]] CommandResult undo(quader::document::Document& document) override;
    [[nodiscard]] std::string_view name() const override;

private:
    quader::io::DocumentFragment fragment_;
    std::vector<quader::document::ObjectId> created_objects_;
    std::vector<quader::document::MeshObject> undo_objects_;
};

} // namespace quader::commands
```

Rules:

- `execute()` validates the fragment before mutation.
- Each object is added through `Document::create_mesh_object()`.
- If any object fails, the command rolls back objects already created during that execute call and
  returns a rejected `CommandResult`.
- `undo()` removes only the objects created by the command and stores enough data for redo.
- Tests must cover apply, undo, redo, and failed import rollback.

If implementation defers document mutation, explicitly leave `ImportFragmentCommand` out and keep
Open/Import actions disabled or warning-only. Do not add UI direct document mutation as a shortcut.

## glTF To Crimson Material Mapping

### Target And Files

Target:

```text
modeling_io_crimson
```

Files:

```text
src/io/gltf/gltf_material_source.hpp
src/io/gltf/gltf_material_mapper.hpp
src/io/gltf/gltf_material_mapper.cpp
```

This task must not add a full `fastgltf` dependency unless the PM/user explicitly authorizes it.
Use project-owned source structs so the mapping can be tested deterministically now and fed by a
real glTF parser later.

### Source Contract

Types:

```cpp
namespace quader::io::gltf {

enum class GltfAlphaMode {
    Opaque,
    Mask,
    Blend,
};

struct GltfTextureRef {
    std::string source_uri;
    std::int32_t texcoord = 0;
};

struct GltfPbrMetallicRoughness {
    crimson::MaterialColorSrgb base_color_factor{1.0F, 1.0F, 1.0F, 1.0F};
    std::optional<GltfTextureRef> base_color_texture;
    float metallic_factor = 1.0F;
    float roughness_factor = 1.0F;
    std::optional<GltfTextureRef> metallic_roughness_texture;
};

struct GltfMaterialSource {
    std::string name;
    GltfPbrMetallicRoughness pbr;
    std::optional<GltfTextureRef> normal_texture;
    float normal_scale = 1.0F;
    std::optional<GltfTextureRef> occlusion_texture;
    float occlusion_strength = 1.0F;
    crimson::MaterialColorSrgb emissive_factor{0.0F, 0.0F, 0.0F, 1.0F};
    std::optional<GltfTextureRef> emissive_texture;
    GltfAlphaMode alpha_mode = GltfAlphaMode::Opaque;
    float alpha_cutoff = 0.5F;
    bool double_sided = false;
    std::vector<std::string> unsupported_extensions;
};

struct TextureSlotMapping {
    std::string slot_name;
    GltfTextureRef source;
    crimson::TextureColorSpace expected_color_space = crimson::TextureColorSpace::Linear;
};

struct GltfMaterialMapping {
    crimson::MaterialInstance material;
    std::vector<TextureSlotMapping> texture_slots;
    ImportedMaterialMetadata metadata;
    IoDiagnosticList diagnostics;
};

[[nodiscard]] quader::foundation::Result<GltfMaterialMapping, IoError>
map_gltf_material_to_crimson(const GltfMaterialSource& source,
                             const crimson::BaseShaderRegistry& registry);

} // namespace quader::io::gltf
```

Mapping rules:

- `OPAQUE` maps to `BaseShaderId::OpaquePbr`.
- `MASK` maps to `BaseShaderId::AlphaCutoutPbr`.
- `BLEND` maps to `BaseShaderId::TransparentPbr`.
- `name` maps to `MaterialInstance::debug_name`. Empty names are allowed but must produce a stable
  fallback name such as `"Material_<index>"` at the parser/collection layer, not inside the single
  material mapper.
- `baseColorFactor` maps to `base_color`.
- `metallicFactor` maps to `metallic`.
- `roughnessFactor` maps to `roughness`.
- `normalTexture scale` maps to `normal_scale`.
- `occlusion strength` maps to `occlusion_strength` only when the selected base shader declares it.
  For V1 `TransparentPbr`, preserve it in metadata and emit a warning because the schema has no
  occlusion slot.
- `emissiveFactor` maps to `emissive_color` or the current Crimson schema field name. Use the exact
  existing parameter names from `BaseShaderRegistry`; do not invent new names.
- `alphaCutoff` maps only for `AlphaCutoutPbr`.
- `doubleSided` maps to `MaterialRenderOverrides::double_sided`.
- Texture refs map by declared Crimson texture slot:
  - `base_color` uses `TextureColorSpace::Srgb`.
  - `emissive` uses `TextureColorSpace::Srgb`.
  - `metallic_roughness` uses `TextureColorSpace::Linear`.
  - `normal` uses `TextureColorSpace::Data`.
  - `occlusion` uses `TextureColorSpace::Linear`.
- Missing textures do not create fake runtime handles. The mapper reports source texture slot
  mappings and lets the future asset/texture service resolve project assets to renderer-safe
  texture handles.
- Unsupported V2 extensions such as `KHR_materials_transmission`, `KHR_materials_volume`,
  `KHR_materials_ior`, `KHR_materials_clearcoat`, `KHR_materials_specular`,
  `KHR_materials_emissive_strength`, and unsupported `KHR_texture_transform` details are preserved
  in `ImportedMaterialMetadata::preserved_extensions` and emit non-fatal warnings.
- The mapper must normalize through `MaterialSystem` or `make_default_material_instance()` plus
  `validate_material_instance()` so output always satisfies the active Crimson base shader schema.
- Output parameter and texture mapping order must be deterministic:
  1. Follow `BaseShaderDefinition::parameters` order for material parameters.
  2. Follow `BaseShaderDefinition::texture_slots` order for texture slot mappings.
  3. Sort preserved extension names lexicographically if source parser order is not guaranteed.

Forbidden:

- No includes from `src/crimson/gpu/`.
- No `RenderTextureHandle` allocation.
- No shader program selection outside `BaseShaderDefinition`.
- No glTF parser dependency in Crimson.
- No UI type or file dialog logic.

Renderer review gate:

- After implementing the mapper and tests, route the implementation to the renderer architect for
  focused review of schema fidelity, color-space mapping, alpha-mode mapping, and absence of GPU
  internals before software final approval.

## UI File-Dialog Boundary

Files:

```text
src/ui/services/file_dialog_service.hpp
src/ui/services/file_dialog_service.cpp
src/ui/services/import_ui_controller.hpp
src/ui/services/import_ui_controller.cpp
```

Service:

```cpp
namespace quader::ui {

struct FileDialogFilter {
    QString label;
    QStringList patterns;
};

struct OpenFileDialogRequest {
    QString title;
    QString initial_directory;
    QList<FileDialogFilter> filters;
};

struct SaveFileDialogRequest {
    QString title;
    QString initial_directory;
    QList<FileDialogFilter> filters;
    QString default_suffix;
};

class IFileDialogService {
public:
    virtual ~IFileDialogService() = default;
    [[nodiscard]] virtual std::optional<QString>
    choose_open_file(QWidget* parent, const OpenFileDialogRequest& request) = 0;
    [[nodiscard]] virtual std::optional<QString>
    choose_save_file(QWidget* parent, const SaveFileDialogRequest& request) = 0;
};

class QtFileDialogService final : public IFileDialogService {
public:
    [[nodiscard]] std::optional<QString>
    choose_open_file(QWidget* parent, const OpenFileDialogRequest& request) override;
    [[nodiscard]] std::optional<QString>
    choose_save_file(QWidget* parent, const SaveFileDialogRequest& request) override;
};

} // namespace quader::ui
```

Rules:

- `QtFileDialogService` is the only class in this task that may call `QFileDialog`.
- `ImportUiController` may translate registry format descriptors into Qt filters.
- `ImportUiController` must pass `std::filesystem::path` to `ImportService`.
- `ImportUiController` displays diagnostics through `NotificationService` and future panels, not by
  asking importers to format UI messages.
- If no production importer is registered, `OpenScene` must either remain disabled or display a
  clear "No importer registered" warning without mutating the document.
- Save/export wiring may be left disabled if no exporter exists yet. Do not fake a successful save.

App service integration:

- Add `io::ImportExportRegistry`, `io::ImportService`, and `io::ExportService` to
  `quader::app::AppServices`.
- Add `ui::QtFileDialogService` and, if wired, `ui::ImportUiController` to `AppServices`.
- Add references to `UiContext` only if UI code needs direct access. Prefer an `ImportUiController`
  wired to `ActionRegistry` in `AppServices`, following `EditorActionController`.

## Staged Implementation Batches

### Batch 1: Core I/O Target And Diagnostics

Files:

```text
CMakeLists.txt
src/io/io_diagnostic.hpp
src/io/io_diagnostic.cpp
src/io/io_error.hpp
src/io/file_format.hpp
src/io/import_options.hpp
src/io/export_options.hpp
tests/unit/io/io_diagnostic_tests.cpp
tools/check_architecture.py
```

Implementation steps:

1. Add `modeling_io` target with public include root `src`.
2. Add I/O diagnostic, error, format, import option, and export option types.
3. Add CTest target `io_diagnostics`.
4. Extend architecture checks for `src/io` boundary rules.
5. Build.

Acceptance:

- `modeling_io` compiles without Qt, UI, Crimson, or graphics-runtime dependencies.
- Diagnostics can carry severity, stable code, path, source location, and subject.
- Boundary check catches Qt Widgets and graphics-runtime includes under `src/io`.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug -R "io_diagnostics|architecture_boundaries"
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Batch 2: Import/Export Interfaces And Static Registry

Files:

```text
CMakeLists.txt
src/io/importer.hpp
src/io/exporter.hpp
src/io/import_export_registry.hpp
src/io/import_export_registry.cpp
src/io/import_service.hpp
src/io/import_service.cpp
src/io/export_service.hpp
src/io/export_service.cpp
tests/unit/io/io_registry_tests.cpp
tests/unit/io/io_import_service_tests.cpp
tests/unit/io/io_export_service_tests.cpp
```

Implementation steps:

1. Add `IImporter`, `IExporter`, registry, import service, and export service.
2. Implement deterministic extension matching.
3. Reject duplicate format IDs and duplicate extensions.
4. Add test double importers/exporters inside tests only.
5. Make services return structured unsupported-format diagnostics.
6. Build.

Acceptance:

- Import/export service code has no UI dependency.
- Registry returns filters/descriptors in registration order.
- Unsupported formats fail before importer execution.
- Export service keeps deterministic export as the default contract.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug -R "io_registry|io_import_service|io_export_service|architecture_boundaries"
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Batch 3: Document Fragment Contracts And Validation Gate

Files:

```text
CMakeLists.txt
src/io/imported_document.hpp
src/io/document_fragment_validator.hpp
src/io/document_fragment_validator.cpp
tests/unit/io/document_fragment_validation_tests.cpp
tests/unit/io/imported_document_build_tests.cpp
```

Optional files if UI import mutation is included:

```text
src/commands/import_commands.hpp
src/commands/import_commands.cpp
tests/unit/commands/import_command_tests.cpp
```

Implementation steps:

1. Add `ImportedMeshObject`, `ImportedMaterialMetadata`, `DocumentFragment`, and `ImportedDocument`.
2. Add `validate_document_fragment()`.
3. Add `build_document_from_import()` that uses `Document::create_mesh_object()`.
4. Add tests for valid fragment, non-finite transforms, invalid mesh, invalid material index, and
   no editable document mutation on failed validation.
5. If adding `ImportFragmentCommand`, add undo/redo and rollback tests.
6. Build.

Acceptance:

- Imported meshes validate through `mesh::validate_mesh()` before success.
- Editable document state is entered only through `Document` APIs.
- Failed imports do not partially mutate documents.
- Tests assert semantic document state, not allocation order.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug -R "document_fragment|imported_document|import_command|document_state_selection|command_history"
ctest --preset qt-mingw-debug -R "architecture_boundaries"
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Batch 4: Deterministic glTF Material Mapping Handoff

Files:

```text
CMakeLists.txt
src/io/gltf/gltf_material_source.hpp
src/io/gltf/gltf_material_mapper.hpp
src/io/gltf/gltf_material_mapper.cpp
tests/unit/io/gltf_material_mapper_tests.cpp
```

Implementation steps:

1. Add `modeling_io_crimson` target depending on `modeling_io` and `crimson`.
2. Add project-owned glTF material source structs.
3. Implement `map_gltf_material_to_crimson()`.
4. Normalize mapped materials against `crimson::BaseShaderRegistry`.
5. Preserve unsupported V2 extension names and emit non-fatal warnings.
6. Add tests for alpha modes, factors, double-sided, texture slot color-space mapping, missing
   textures, unsupported extension preservation, deterministic ordering, and schema validation
   failure diagnostics.
7. Build.

Acceptance:

- `OPAQUE`, `MASK`, and `BLEND` map to the correct Crimson V1 base shaders.
- Mapper output never contains parameters or texture slots not declared by the selected base shader.
- Mapper uses Crimson public schemas but no GPU/runtime internals.
- Texture color-space handoff is deterministic and test-covered.
- Renderer architect has enough implementation context for focused review.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug -R "gltf_material_mapper|crimson_material_system|crimson_color_space|architecture_boundaries"
cmake --build --preset qt-mingw-debug --target check_architecture
```

Authority checkpoint:

- Send changed files, tests, and any deviations to the renderer architect for material mapping
  review before final software approval.

### Batch 5: UI File Dialog Service Boundary

Files:

```text
CMakeLists.txt
src/ui/services/file_dialog_service.hpp
src/ui/services/file_dialog_service.cpp
src/ui/services/import_ui_controller.hpp
src/ui/services/import_ui_controller.cpp
src/ui/qt_app/ui_context.hpp
src/app/app_services.hpp
src/app/application.cpp
src/ui/actions/action_state_updater.cpp
src/ui/actions/editor_state_snapshot.hpp
tests/unit/ui/ui_file_dialog_service_tests.cpp
tests/unit/ui/ui_import_controller_tests.cpp
```

Implementation steps:

1. Add `IFileDialogService` and `QtFileDialogService`.
2. Add fake file-dialog service in tests.
3. Generate import/export filters from `ImportExportRegistry` descriptors.
4. Wire `OpenScene` only if the behavior is honest:
   - If no production importer exists, keep disabled or show a warning.
   - If an importer is registered, route through `ImportService` and optionally
     `ImportFragmentCommand`.
5. Display structured diagnostics through `NotificationService`.
6. Build.

Acceptance:

- File dialogs exist only in UI services.
- Importers/exporters do not include Qt or display UI.
- Tests prove canceled dialogs do not call import services.
- Tests prove unsupported format diagnostics are presented without document mutation.
- If document mutation is wired, it goes through commands and supports undo/redo.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug -R "ui_file_dialog|ui_import_controller|ui_action_wiring|architecture_boundaries"
cmake --build --preset qt-mingw-debug --target check_architecture
```

Deploy note:

- If `OpenScene`, `SaveScene`, or user-visible file menu behavior changes, also run:

```powershell
cmake --build --preset qt-mingw-debug --target deploy
```

### Batch 6: Final Integration And Documentation

Files:

```text
CMakeLists.txt
README.md
dev-changelog.md
tools/check_architecture.py
tests/unit/io/*
tests/unit/ui/*
tests/unit/commands/* if import command was added
```

Implementation steps:

1. Ensure all new targets are included in CMake source lists.
2. Update README only if commands or user-visible file behavior changed.
3. Update `dev-changelog.md` for durable architecture/source/test changes.
4. Run full verification.
5. Return to software architect with changed files, build/test results, renderer review result, and
   deviations.

Final verification:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
python tools/project_board.py validate
```

If user-visible runtime behavior changed:

```powershell
cmake --build --preset qt-mingw-debug --target deploy
```

Do not run `tools/dev_builds.py archive` and do not edit `VERSION` or `DEV_VERSION`.

## Tests Policy Application

Tests-policy applies to all test work in this plan.

Required automated tests:

- I/O diagnostics preserve structured severity, code, source path, location, and subject.
- Registry extension matching is deterministic and case-insensitive.
- Duplicate importer/exporter registrations fail with useful diagnostics.
- Unsupported file extensions fail without importer execution.
- Import services validate fragments before success.
- Invalid mesh fragments fail and do not become document state.
- Build-document path uses `Document::create_mesh_object()` and preserves document validation.
- Optional import command supports apply, undo, redo, and rollback on partial failure.
- glTF material mapping covers alpha mode, factors, texture slots, color spaces, double-sided,
  unsupported extension preservation, and deterministic output ordering.
- UI file-dialog tests use fakes and assert semantic workflow outcomes: canceled dialog, unsupported
  format, no document mutation on failure, and command-routed mutation on success if enabled.
- Architecture boundary tests reject UI and graphics-runtime leakage into I/O.

Fixtures:

- Keep fixtures tiny and readable.
- Prefer in-test source structs and fake importers over large golden files.
- Do not use user files or machine-specific paths.
- Use temporary directories for any service tests that need filesystem paths.

Fuzz/property tests:

- Not required for this task if no real file parser is added.
- If a real parser is added in scope by PM/user approval, add parser robustness tests for empty,
  truncated, malformed, and valid inputs. Consider fuzz tests only when there is an actual parser
  surface to stress.

Sanitizers:

- No new sanitizer target is required for this task.
- If implementation touches low-level ownership or parser code beyond these contracts, executor
  should report whether ASan/UBSan is available locally and run relevant focused tests under it
  when practical.

Manual verification substitute:

- Manual GUI smoke is only a supplement for file-dialog behavior. It does not replace automated
  service/controller tests.
- If the deploy target is blocked by a running executable, report the exact blocker.

## Review And Authority Points

Executor setup:

- Before implementation, executor must set a Codex `/goal` for task #10/A48 or the PM assignment.
- The goal is complete only after the full assigned scope is implemented, verified, documented when
  required, and handed back through PM/authority review.

Build checkpoints:

- Run `cmake --build --preset qt-mingw-debug` after each coherent implementation slice.
- Do not start another independent code task after two slices without a successful build.
- Run relevant CTest filters after each slice that adds tests.
- Run full verification before requesting review.

Authority routing:

- Software architect owns final approval for the I/O and app-boundary design.
- Renderer architect must review the glTF-to-Crimson material mapping slice before final approval.
- UI architect review is recommended if implementation changes visible menu/action/dialog behavior
  beyond adding a file-dialog service and fake-tested controller.
- PM must integrate worker reports and may update board metadata. Executors and architects must not
  edit `project_board.md` directly.

Deviation handling:

- If an executor wants to add `fastgltf`, another parser dependency, project asset catalog work,
  texture copying, material editing UI, direct document mutation from UI, or Crimson GPU handles in
  I/O, stop and send a `Workaround/Deviation Report`.
- If `Document` lacks an API needed for safe import apply, add the narrow document API through the
  software authority path instead of writing into `ObjectStore`.

## Residual Risks

- This task establishes the boundary, not a complete production importer/exporter suite. The first
  production file format may still need a follow-up task.
- Material metadata cannot become real document material assignment until the document material
  model exists.
- Texture URI resolution and project-local texture copying are intentionally deferred to task #18 or
  a later asset-pipeline task.
- A full glTF parser would require an explicit dependency decision and parser robustness tests.
- `modeling_io_crimson` is a deliberate adapter target. If implementers collapse it into
  `modeling_io`, I/O will gain an unnecessary renderer dependency.
- UI action state may need careful wording if no production importer/exporter is registered yet.
  Prefer disabled or warning-only behavior over fake success.

## Final Handoff Requirements

When implementation is complete, return to the software architect with:

- Plan path: `agents/plans/implementation_task10_io_asset_boundaries_doc.md`
- Changed files
- Implementation notes by batch
- Renderer architect material-mapping review result
- Build/test commands and results
- Any deviations or blocked verification

If approved, the main/root agent should move this plan from `agents/plans/` to `agents/archive/`
only after the PM/authority workflow allows it. Do not delete the plan automatically.
