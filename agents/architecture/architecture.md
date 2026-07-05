# Overall App Architecture Guide for a C++ Polygon Modeling App

> **Audience:** humans and coding agents building a C++ modeling app with polygon / half-edge mesh editing.  
> **Goal:** keep the app clean, fast, testable, and easy for agentic AI to modify without turning the codebase into an unreadable architecture diagram.

> **UI:** detailed Qt Widgets guidance now lives in [ui.md](ui.md).

---

## 0. The core stance

A modeling app should be organized around this simple idea:

> **The core owns geometry and topology. Commands mutate documents. Tools convert user input into commands. Renderers and UI are views of state.**

Do not let every system talk to every other system. A modeling app becomes hard when topology, selection, undo, rendering, UI, tools, and file I/O all start sharing mutable state. The architecture should make the correct dependency direction obvious.

This document recommends a hybrid architecture:

- **Layered architecture** for dependency direction.
- **Hexagonal / ports-and-adapters thinking** at external boundaries such as renderer, filesystem, windowing, plugin loading, and import/export.
- **Data-oriented design** inside mesh, geometry, renderer, and hot loops.
- **Object-oriented design** where invariants, ownership, and behavior belong together.
- **Command pattern** for undoable edits.
- **Strategy pattern** for swappable algorithms and policies.
- **Observer / event queue** for document change notifications, but not as a global event-bus free-for-all.

The app should be boring on purpose. Boring code is easier to test, easier to refactor, easier to profile, and easier for AI agents to change safely.

---

## 1. The mantras

These are the rules to keep repeating during design and code review.

### 1.1 Core before UI

The mesh core should not know about Qt, ImGui, Vulkan, OpenGL, Metal, DirectX, mouse events, dialogs, cameras, gizmos, or file pickers.

Bad:

```cpp
void Polyhedron::split_edge(EdgeId edge, Viewport& viewport, UndoStack& undo);
```

Good:

```cpp
MeshEditResult split_edge(Polyhedron& mesh, EdgeId edge);
```

The UI can call a command. The command can call a mesh operation. The mesh operation should only care about topology, geometry, attributes, and invariants.

### 1.2 Invariants are not comments; they are code

Every important data structure needs a `validate()` function or equivalent debug checker.

For half-edge meshes, document invariants such as:

- `opposite(opposite(h)) == h`
- `next(prev(h)) == h`
- `prev(next(h)) == h`
- all halfedges in a face cycle reference that face
- all halfedges around a vertex reference that vertex as origin or target consistently
- boundary representation is consistent
- deleted records are never returned by iteration unless explicitly requested
- every user-facing ID is either valid or explicitly invalid

In debug builds, run validation after topology operations. In tests, run it constantly.

### 1.3 One mutation path

Do not let tools, UI panels, scripts, importers, and renderers all mutate the document directly.

Prefer this path:

```text
User input / script / importer
        ↓
Command
        ↓
Document mutation API
        ↓
Mesh operation / scene operation
        ↓
Change event + dirty flags
        ↓
UI and renderer update
```

This is how undo, redo, validation, selection remapping, dirty flags, and autosave stay sane.

### 1.4 IDs, not raw pointers, across systems

Mesh elements should be referenced by stable handle types:

```cpp
struct VertexId {
    uint32_t index = kInvalidIndex;
    uint32_t generation = 0;
};

struct EdgeId {
    uint32_t index = kInvalidIndex;
    uint32_t generation = 0;
};
```

Avoid storing raw `Vertex*`, `Face*`, or `Halfedge*` in selections, commands, renderer caches, tools, or UI state. Pointers into `std::vector` become invalid after reallocation. Even if you use a pool, raw pointers make deletion and undo more dangerous.

### 1.5 Public API should read like documentation

A good public header should let a human or coding agent understand how to use the module without opening ten implementation files.

Good public headers contain:

- purpose
- key invariants
- ownership rules
- thread-safety assumptions
- main types
- public operations
- clear preconditions
- clear return values

They should not contain large implementation bodies unless necessary for templates or tiny inline functions.

### 1.6 No hidden global state

Avoid singletons, global service locators, global mutable registries, hidden current documents, hidden active tools, and hidden current renderers.

Acceptable:

```cpp
class App {
public:
    App(Window& window, Renderer& renderer, FileSystem& files);
};
```

Dangerous:

```cpp
Renderer::global().draw(Document::current().mesh());
```

Global state makes tests painful, undo unreliable, multithreading fragile, and AI-generated changes harder to reason about.

### 1.7 Dependency direction is architecture

The dependency graph should be boring:

```text
app
 ├── ui
 ├── tools
 ├── render
 ├── io
 └── commands
      └── document
           └── mesh
                ├── geometry
                ├── math
                └── foundation
```

Lower layers must not include higher layers.

### 1.8 Data-oriented where it matters, OOP where it pays

Use object-oriented design for:

- modules with invariants
- ownership boundaries
- tool state machines
- command objects
- renderer GPU/runtime layers
- import/export plugins
- application services

Use data-oriented design for:

- mesh storage
- selection sets
- render buffers
- spatial acceleration
- geometry algorithms
- hot loops
- attribute arrays

Do not make every vertex an object with virtual functions. Do not make every algorithm a class hierarchy. Plain functions are often the cleanest design.

### 1.9 Measure before optimizing, but design to allow speed

Do not micro-optimize early, but do avoid architectural decisions that make performance impossible later.

Good early choices:

- stable IDs instead of raw pointers
- contiguous arrays for mesh records
- explicit dirty flags
- clear command mutation boundaries
- render snapshots
- benchmark hooks
- local validation for topology operations
- no UI dependencies in core algorithms

Bad early choices:

- `shared_ptr` graph for every mesh element
- virtual dispatch per halfedge traversal
- renderer reads directly from mutable mesh while tools edit it
- topology operations scattered across UI callbacks
- no tests for topology invariants

---

## 2. Recommended repository layout

Use a layout that makes the dependency graph visible.

```text
repo/
  AGENTS.md
  CMakeLists.txt
  CMakePresets.json
  README.md

  docs/
    architecture.md
    code_map.md
    invariants/
      polyhedron.md
      command_history.md
    decisions/
      ADR-0001-half-edge-storage.md
      ADR-0002-command-history.md
    tasks/
      TASK-template.md
    agent_mistakes.md

  src/
    foundation/
      result.hpp
      assert.hpp
      ids.hpp
      logging.hpp

    math/
      vec2.hpp
      vec3.hpp
      mat4.hpp
      bounds.hpp

    geometry/
      primitives.hpp
      intersections.hpp
      predicates.hpp
      normals.hpp

    mesh/
      core/
        polyhedron.hpp
        polyhedron.cpp
        mesh_records.hpp
        mesh_ids.hpp
        mesh_attributes.hpp
        mesh_validation.hpp
        mesh_validation.cpp

      ops/
        split_edge.hpp
        split_edge.cpp
        collapse_edge.hpp
        collapse_edge.cpp
        flip_edge.hpp
        flip_edge.cpp
        extrude_face.hpp
        extrude_face.cpp

      query/
        mesh_traversal.hpp
        mesh_traversal.cpp
        adjacency.hpp
        adjacency.cpp

    document/
      document.hpp
      document.cpp
      scene_object.hpp
      selection.hpp
      selection.cpp
      document_events.hpp

    commands/
      command.hpp
      command_history.hpp
      command_history.cpp
      mesh_commands.hpp
      mesh_commands.cpp
      selection_commands.hpp
      selection_commands.cpp

    tools/
      tool.hpp
      select_tool.hpp
      select_tool.cpp
      extrude_tool.hpp
      extrude_tool.cpp
      transform_tool.hpp
      transform_tool.cpp

    crimson/
      renderer.hpp
      render_scene.hpp
      render_snapshot.hpp
      mesh_render_cache.hpp
      gpu_buffer.hpp
      gpu/
        gpu_device.hpp
        gpu_resources.hpp

    io/
      mesh_importer.hpp
      mesh_exporter.hpp
      obj_importer.cpp
      gltf_importer.cpp

    ui/
      qt_app/
        qt_application_bootstrap.hpp
        qt_application_bootstrap.cpp
        main_window.hpp
        main_window.cpp

      actions/
        action_id.hpp
        action_registry.hpp
        action_registry.cpp
        action_state_updater.hpp
        action_state_updater.cpp

      models/
        item_model_ids.hpp
        document_item_model.hpp
        document_item_model.cpp
        property_item_model.hpp
        property_item_model.cpp
        command_history_item_model.hpp
        command_history_item_model.cpp

      delegates/
        standard_item_delegate.hpp
        standard_item_delegate.cpp
        numeric_item_delegate.hpp
        numeric_item_delegate.cpp

      panels/
        panel_id.hpp
        panel_contract.hpp
        panel_context.hpp
        panel_host.hpp
        panel_host.cpp
        panel_factory.hpp
        panel_factory.cpp

      dialogs/
        dialog_service.hpp
        dialog_service.cpp

      style/
        fusion_style_policy.hpp
        fusion_style_policy.cpp
        ui_metrics.hpp
        ui_metrics.cpp
        icon_registry.hpp
        icon_registry.cpp

      viewport/
        viewport_widget.hpp
        viewport_widget.cpp
        viewport_controller.hpp
        viewport_controller.cpp
        viewport_render_host.hpp
        viewport_render_host.cpp

      services/
        settings_service.hpp
        settings_service.cpp
        notification_service.hpp
        notification_service.cpp

    app/
      application.hpp
      application.cpp
      main.cpp

  tests/
    unit/
      mesh/
      commands/
      document/
    golden/
      mesh_ops/
    fuzz/
      mesh_ops/
    fixtures/

  benchmarks/
    mesh_traversal_bench.cpp
    mesh_ops_bench.cpp
    render_upload_bench.cpp

  tools/
    check_architecture.py
    generate_code_map.py
```

This structure is intentionally simple. It avoids a giant `engine/` folder and avoids hiding everything behind vague names like `core`, `common`, and `systems`.

---

## 3. Layer responsibilities

### 3.1 `foundation`

Contains tiny, dependency-light utilities used everywhere.

Allowed:

- assertions
- result/error types
- strong IDs
- small fixed utilities
- logging interface
- compile-time configuration
- basic memory helpers

Not allowed:

- mesh logic
- UI logic
- renderer logic
- application state
- random “misc” dumping ground

`foundation` should be boring and small. Once it becomes a landfill, every file includes it and compile times suffer.

### 3.2 `math`

Contains generic math types.

Examples:

```cpp
Vec2
Vec3
Vec4
Mat4
Quat
Aabb
Ray
Plane
```

Rules:

- no dependency on mesh, document, UI, or renderer
- keep value types cheap and predictable
- avoid clever expression-template math unless it is absolutely necessary
- write tests for numerical edge cases
- document coordinate conventions

### 3.3 `geometry`

Contains algorithms that work on geometric primitives, not app state.

Examples:

```cpp
intersect_ray_triangle()
closest_point_on_segment()
compute_face_normal()
compute_area_weighted_vertex_normals()
project_point_to_plane()
```

Rules:

- pure functions where possible
- no document mutation
- no UI
- no renderer
- no undo
- careful handling of epsilons and degenerates

### 3.4 `mesh/core`

Owns the mesh data structure and invariants.

The mesh core should contain:

- element record storage
- IDs and handles
- attribute storage
- creation/deletion primitives
- low-level traversal
- validation
- serialization-friendly access to raw records if needed

It should not contain:

- UI tools
- renderer buffer generation
- file dialogs
- command history
- undo stack
- app settings
- editor panels

### 3.5 `mesh/ops`

Contains topology and geometry operations.

Examples:

```cpp
split_edge()
collapse_edge()
flip_edge()
extrude_face()
inset_face()
bevel_edge()
triangulate_face()
merge_vertices()
delete_face()
```

Each operation should be documented with:

- purpose
- preconditions
- modified elements
- created/deleted elements
- selection remapping behavior
- attribute interpolation behavior
- validation expectations
- undo patch strategy if relevant

Operations should return structured results:

```cpp
struct SplitEdgeResult {
    VertexId new_vertex;
    EdgeId first_edge;
    EdgeId second_edge;
    std::vector<FaceId> affected_faces;
};

Result<SplitEdgeResult, MeshEditError> split_edge(Polyhedron& mesh, EdgeId edge);
```

Avoid returning raw booleans when errors matter.

### 3.6 `document`

Owns the editable scene state.

A `Document` is not just a mesh. It may contain:

- scene objects
- object transforms
- materials
- active selection
- layers / collections
- active mode
- document metadata
- dirty state
- change events
- object naming
- asset references

The document is the boundary between low-level mesh operations and editor-level state.

### 3.7 `commands`

Commands are undoable user actions.

Examples:

```cpp
class SplitEdgeCommand final : public ICommand {
public:
    explicit SplitEdgeCommand(ObjectId object, EdgeId edge);

    CommandResult execute(Document& document) override;
    CommandResult undo(Document& document) override;

    std::string_view name() const override;

private:
    ObjectId object_;
    EdgeId edge_;
    MeshPatch undo_patch_;
    SelectionSnapshot old_selection_;
    SelectionSnapshot new_selection_;
};
```

Commands should:

- validate inputs
- call document / mesh operations
- store undo data
- update selection when needed
- emit document events indirectly through the document
- be testable without UI

Commands should not:

- ask the user questions
- open dialogs
- call renderer APIs
- depend on mouse events
- own large services unnecessarily

### 3.8 `tools`

Tools are interactive state machines.

A tool converts input into previews and commands.

Examples:

```text
SelectTool
TransformTool
ExtrudeTool
KnifeTool
BevelTool
LoopCutTool
```

A tool may track temporary interaction state:

- hovered element
- drag start
- preview geometry
- snapping candidate
- active axis
- numeric input buffer

A tool should not permanently mutate the document except by creating and executing commands. Temporary previews should live outside the document or be clearly marked as preview state.

### 3.9 `crimson`

Crimson consumes renderer snapshots. It should not own editor truth.

Recommended flow:

```text
Document changed
    ↓
Dirty flags / document event
    ↓
RenderSnapshotBuilder
    ↓
RenderSceneSnapshot
    ↓
Crimson GPU layer
```

The renderer should not traverse arbitrary mutable mesh state during drawing if the mesh can be edited at the same time. Build render-friendly buffers from document state, then render those buffers.

### 3.10 `io`

Importers and exporters translate between external formats and internal documents.

Rules:

- importer errors must be structured
- importer should not directly touch UI
- importer should preserve source diagnostics
- importer should validate imported meshes
- exporter should be deterministic for tests where possible

For plugin-style I/O, start with a static registry. Add dynamic plugins later only when there is a real need.

### 3.11 `ui`

The UI displays state, captures user intent, and adapts Qt Widgets to the editor architecture.

UI code can know about:

- commands
- tools
- document views
- app services
- renderer viewport integration
- Qt presentation models and delegates
- action state and workspace state

UI code may use Qt Widgets types such as `QWidget`, `QMainWindow`, `QAction`, `QAbstractItemModel`, `QItemSelectionModel`, `QDockWidget`, and dialogs. That permission stops at the UI boundary. Lower layers must not include Qt Widgets headers.

UI code should not implement mesh topology algorithms. A button click can create a command; it should not hand-edit halfedge links. A Qt item model can present document state; it should not become the document. A widget can show temporary interaction state; it should not become the source of truth for editable scene data.

### 3.12 `app`

Wires everything together.

`app` owns:

- application lifetime
- window initialization
- service construction
- dependency injection
- settings loading
- main loop
- shutdown

It should be the composition root, not a dumping ground.

---

## 4. Dependency rules

Use hard dependency rules. They prevent architectural decay.

### 4.1 Allowed dependency direction

```text
foundation  ←  math  ←  geometry  ←  mesh  ←  document  ←  commands  ←  tools/ui/app
                                      ↑
                                    crimson reads snapshots from document, not mesh internals directly
```

More explicitly:

| Module       | May depend on                                                     |
| ------------ | ----------------------------------------------------------------- |
| `foundation` | nothing project-specific                                          |
| `math`       | `foundation`                                                      |
| `geometry`   | `foundation`, `math`                                              |
| `mesh/core`  | `foundation`, `math`, `geometry`                                  |
| `mesh/ops`   | `foundation`, `math`, `geometry`, `mesh/core`                     |
| `document`   | `foundation`, `math`, `geometry`, `mesh`                          |
| `commands`   | `foundation`, `document`, `mesh/ops`                              |
| `tools`      | `foundation`, `document`, `commands`, `geometry`, `math`          |
| `crimson`    | `foundation`, `math`, `document` snapshots, internal GPU/runtime APIs |
| `io`         | `foundation`, `document`, `mesh`, external file libs              |
| `ui`         | `app services`, `document`, `commands`, `tools`, `crimson`        |
| `app`        | everything needed to wire the application                         |

### 4.2 Forbidden dependencies

These should be considered architecture violations:

- `mesh` includes `ui`
- `mesh` includes `crimson`
- `mesh` includes `commands`
- `commands` include `ui`
- `document` includes Crimson GPU/runtime internals
- `foundation` includes anything app-specific
- `geometry` depends on document state
- any lower layer calls `Application::instance()`
- any low-level module calls file dialogs or message boxes

### 4.3 Enforce with CMake targets

Use one target per module or module group:

```cmake
add_library(modeling_foundation ...)
add_library(modeling_math ...)
add_library(modeling_geometry ...)
add_library(modeling_mesh_core ...)
add_library(modeling_mesh_ops ...)
add_library(modeling_document ...)
add_library(modeling_commands ...)
add_library(modeling_tools ...)
add_library(crimson ...)
add_library(modeling_ui ...)
add_executable(modeling_app ...)

target_link_libraries(modeling_mesh_core
    PUBLIC
        modeling_foundation
        modeling_math
        modeling_geometry
)

target_link_libraries(modeling_commands
    PUBLIC
        modeling_document
        modeling_mesh_ops
)
```

Do not paper over bad boundaries by adding one giant include path or one giant `modeling_all` library that every target links against.

---

## 5. Header and source file rules

### 5.1 Header principles

Headers are interfaces. They should be self-contained and directly include what they use.

Good:

```cpp
#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "mesh/core/mesh_ids.hpp"
#include "math/vec3.hpp"

namespace modeling::mesh {

class Polyhedron {
public:
    VertexId add_vertex(const Vec3& position);
    std::span<const VertexRecord> vertices() const noexcept;

private:
    std::vector<VertexRecord> vertices_;
};

} // namespace modeling::mesh
```

Bad:

```cpp
#pragma once

namespace modeling::mesh {

class Polyhedron {
public:
    VertexId add_vertex(const Vec3& position); // Where are VertexId and Vec3 declared?
};

}
```

### 5.2 Include order

For `.cpp` files:

```cpp
#include "mesh/core/polyhedron.hpp" // matching header first

#include <algorithm>
#include <cassert>
#include <vector>

#include "geometry/normals.hpp"
#include "mesh/core/mesh_validation.hpp"
```

For `.hpp` files:

```cpp
#pragma once

#include <cstdint>
#include <vector>

#include "foundation/result.hpp"
#include "math/vec3.hpp"
```

Project convention:

1. Matching header first in `.cpp`.
2. C++ standard library headers.
3. C/platform headers if needed.
4. Third-party headers.
5. Project headers.
6. Sort alphabetically within groups.

The important practical reason for putting the matching header first in a `.cpp` is that it catches missing includes in the header immediately.

### 5.3 Forward declarations

Use forward declarations only when they reduce coupling without hiding important dependencies.

Good use:

```cpp
class MeshObserver;

class Polyhedron {
public:
    void set_observer(MeshObserver* observer);
};
```

Do not forward declare:

- standard library types
- third-party types unless the library provides official forward headers
- value members
- base classes
- types used by inline functions
- templates you do not own

Prefer direct includes when in doubt. Hidden dependencies hurt refactoring tools and coding agents.

### 5.4 Class section order

Use:

```cpp
class Example {
public:
    // constructors / destructor
    // main API
    // queries
    // accessors

protected:
    // extension hooks, rarely used

private:
    // helper methods
    // data members
    // private constants
};
```

Public first makes the header read like documentation. Private data-first classes are acceptable for tiny implementation-only structs in `.cpp` files.

### 5.5 Constants

Use the smallest meaningful scope.

Good:

```cpp
namespace {
constexpr float kDefaultSnapEpsilon = 1e-5f;
}
```

Good:

```cpp
class Polyhedron {
public:
    static constexpr uint32_t kInvalidIndex = UINT32_MAX;
};
```

Avoid a giant global `constants.hpp`. It becomes a dependency magnet.

---

## 6. Half-edge mesh architecture

### 6.1 Store records, not object graphs

Avoid this:

```cpp
struct Vertex {
    std::vector<std::shared_ptr<Edge>> edges;
};

struct Edge {
    std::shared_ptr<Vertex> a;
    std::shared_ptr<Vertex> b;
};
```

Prefer records in contiguous storage:

```cpp
struct VertexRecord {
    HalfedgeId outgoing;
    Vec3 position;
    uint32_t flags = 0;
};

struct HalfedgeRecord {
    VertexId origin;
    HalfedgeId opposite;
    HalfedgeId next;
    HalfedgeId prev;
    FaceId face;
    EdgeId edge;
};

struct EdgeRecord {
    HalfedgeId halfedge;
};

struct FaceRecord {
    HalfedgeId halfedge;
};
```

This is easier to validate, serialize, diff, benchmark, and undo.

### 6.2 Use strong IDs

Do not pass raw integers everywhere.

Bad:

```cpp
void collapse_edge(uint32_t edge);
```

Good:

```cpp
Result<CollapseEdgeResult, MeshEditError> collapse_edge(Polyhedron& mesh, EdgeId edge);
```

This prevents accidentally passing a `VertexId` where an `EdgeId` is expected.

### 6.3 Use generation counters for deletion safety

If IDs survive outside the mesh, use generation counters:

```cpp
template <typename Tag>
struct Id {
    uint32_t index = kInvalidIndex;
    uint32_t generation = 0;
};
```

When a record is deleted and its slot reused, increment the generation. This lets the mesh reject stale IDs.

### 6.4 Choose a boundary representation once

Common choices:

1. **Invalid face for boundary halfedges**
2. **Explicit boundary faces**
3. **Separate boundary loop structure**

Pick one, document it, and enforce it in validation. Do not mix styles.

For an editor, invalid-face boundary halfedges are often straightforward, but explicit boundary loops can simplify some traversals. The important thing is consistency.

### 6.5 Separate topology, geometry, and attributes

A clean mesh usually has:

```text
topology records:
  vertices, halfedges, edges, faces

geometry attributes:
  positions
  normals
  UVs

custom attributes:
  per-vertex colors
  per-face material IDs
  creases
  bevel weights
  selection masks
```

Do not hardcode every possible attribute into the core record type. Keep core topology small and stable.

Possible design:

```cpp
class MeshAttributes {
public:
    AttributeId create_vertex_attribute(std::string name, AttributeFormat format);
    std::span<std::byte> vertex_attribute_data(AttributeId id);
};
```

Do not build a fully dynamic attribute system too early. Start with the attributes you actually need, but leave a clear path for extension.

### 6.6 Make operations transactional

Topology operations should follow this shape:

```text
1. Validate input IDs.
2. Check preconditions.
3. Collect affected local neighborhood.
4. Prepare new records.
5. Mutate topology in a small, obvious block.
6. Update attributes.
7. Update deletion/free lists.
8. Validate affected region in debug.
9. Return structured result.
```

Avoid operations that partially mutate the mesh and then fail halfway with no way to recover.

### 6.7 Mesh operation skeleton

```cpp
Result<SplitEdgeResult, MeshEditError> split_edge(Polyhedron& mesh, EdgeId edge) {
    if (!mesh.is_valid(edge)) {
        return unexpected(MeshEditError::invalid_edge(edge));
    }

    const auto neighborhood = collect_split_edge_neighborhood(mesh, edge);
    if (!neighborhood.can_split()) {
        return unexpected(MeshEditError::non_manifold_edge(edge));
    }

    MeshEditTransaction txn(mesh);

    const VertexId new_vertex = txn.create_vertex();
    // Rewire halfedges.
    // Interpolate attributes.
    // Mark affected faces dirty.

    auto result = SplitEdgeResult{
        .new_vertex = new_vertex,
        .affected_faces = neighborhood.faces(),
    };

    txn.commit();

    MODELING_DEBUG_VALIDATE(mesh, result.affected_faces);

    return result;
}
```

Even if `MeshEditTransaction` starts simple, the shape helps prevent half-mutated failures.

---

## 7. Document model

### 7.1 Document owns editor state

A document can contain:

```cpp
class Document {
public:
    ObjectId create_mesh_object(std::string name, Polyhedron mesh);
    MeshObject& mesh_object(ObjectId id);
    const Selection& selection() const noexcept;

    DocumentChangeSet take_pending_changes();

private:
    ObjectStore objects_;
    Selection selection_;
    MaterialLibrary materials_;
    DocumentChangeQueue changes_;
    bool dirty_ = false;
};
```

The document should centralize:

- object creation/deletion
- selection ownership
- material assignment
- naming
- layer membership
- dirty state
- change events
- undo integration points

### 7.2 Keep selection separate from mesh

Selection is editor state, not core topology. Store selection in the document:

```cpp
class Selection {
public:
    void select_vertex(ObjectId object, VertexId vertex);
    void select_edge(ObjectId object, EdgeId edge);
    void clear();

private:
    std::vector<ComponentRef> selected_components_;
};
```

When topology operations delete or create elements, commands should update selection using operation results.

### 7.3 Use explicit document events

Document events should be specific enough to update UI and render caches efficiently:

```cpp
enum class DocumentChangeKind {
    object_created,
    object_deleted,
    mesh_topology_changed,
    mesh_geometry_changed,
    selection_changed,
    material_changed,
    transform_changed,
};
```

Avoid one vague `document_changed` event for everything. It forces every listener to refresh too much.

---

## 8. Command architecture and undo/redo

### 8.1 Use commands for user-visible edits

Commands represent operations the user thinks of as one action:

- Split edge
- Extrude face
- Move selection
- Delete selected
- Assign material
- Import mesh
- Rename object

The command boundary is the undo boundary.

### 8.2 Command interface

```cpp
class ICommand {
public:
    virtual ~ICommand() = default;

    virtual std::string_view name() const noexcept = 0;
    virtual CommandResult execute(Document& document) = 0;
    virtual CommandResult undo(Document& document) = 0;

    virtual bool can_merge_with(const ICommand& next) const noexcept {
        return false;
    }

    virtual CommandResult merge_with(std::unique_ptr<ICommand> next) {
        return CommandResult::not_mergeable();
    }
};
```

Use merging for drag actions:

- move vertex continuously
- transform selection
- brush sculpting
- numeric slider changes

Do not create 2,000 undo entries while the user drags the mouse.

### 8.3 Undo storage strategy

Start simple, then optimize.

Phase 1:

- store before/after snapshots for small edits
- easiest to get correct
- good for MVP and tests

Phase 2:

- store mesh patches for topology operations
- store transform deltas for transforms
- store selection snapshots for selection changes

Phase 3:

- compress repeated edits
- support command serialization if needed
- support macro commands

Do not build an advanced persistent undo database before the editor exists.

### 8.4 Mesh patches

A mesh patch can store:

- created records
- deleted records
- modified records before/after
- attribute changes
- selection before/after
- dirty ranges

Keep the patch format close to the mesh storage format. Avoid a separate abstract undo language unless there is a proven need.

---

## 9. Tool architecture

### 9.1 Tools are state machines

A tool should have explicit states:

```cpp
enum class TransformToolState {
    idle,
    hovering,
    dragging,
    numeric_input,
};
```

Tool input methods:

```cpp
class ITool {
public:
    virtual void on_mouse_down(const PointerEvent& event) = 0;
    virtual void on_mouse_move(const PointerEvent& event) = 0;
    virtual void on_mouse_up(const PointerEvent& event) = 0;
    virtual void on_key_down(const KeyEvent& event) = 0;
    virtual void draw_overlay(ToolOverlayRenderer& overlay) const = 0;
};
```

### 9.2 Tools create commands

Bad:

```cpp
void ExtrudeTool::on_mouse_down(...) {
    document.mesh().extrude_face(active_face_);
}
```

Good:

```cpp
void ExtrudeTool::commit() {
    command_bus_.execute(std::make_unique<ExtrudeFaceCommand>(object_, face_, distance_));
}
```

### 9.3 Tool previews are not document truth

Previews can be:

- overlay draw data
- temporary render geometry
- ghost meshes
- cached projected positions

Do not insert preview topology into the real mesh unless you have a very explicit preview transaction model.

---

## 10. UI architecture

Detailed Qt Widgets guidance lives in [ui.md](ui.md).

Keep these app-level boundaries in force:

- UI captures user intent, presents document state, and adapts Qt Widgets concepts to commands, tools, render snapshots, and app services.
- UI may depend on application services, documents, commands, tools, and render integration.
- Lower layers must not include Qt Widgets headers or call UI services.
- Product-specific panels and workflows belong in UI modules, not in mesh, document, or command code.

---

## 11. Rendering architecture

### 11.1. Renderer consumes render data, not editor internals

Use an intermediate render snapshot:

```cpp
struct MeshRenderSnapshot {
    ObjectId object;
    std::span<const Vec3> positions;
    std::span<const uint32_t> indices;
    MaterialId material;
    Transform transform;
    MeshRenderDirtyFlags dirty_flags;
};
```

The renderer should not need to know about halfedge connectivity.

### 11.2. Dirty flags

Use precise dirty flags:

```cpp
enum class MeshDirtyFlag : uint32_t {
    topology = 1 << 0,
    positions = 1 << 1,
    normals = 1 << 2,
    uvs = 1 << 3,
    materials = 1 << 4,
    selection = 1 << 5,
};
```

Performance trap: rebuilding every GPU buffer after every selection change.

### 11.3. Snapshot threading

Recommended rule:

> The document mutates on the main/editor thread. Background jobs and render code consume immutable snapshots.

This avoids many locks. If you later add background remeshing or heavy imports, run them on copies or immutable inputs, then apply results through commands.

---

## 12. File I/O architecture

### 12.1. Importers produce documents or document fragments

```cpp
struct ImportResult {
    std::vector<MeshObject> objects;
    std::vector<ImportDiagnostic> diagnostics;
};

class IMeshImporter {
public:
    virtual ~IMeshImporter() = default;
    virtual bool can_import(const FilePath& path) const = 0;
    virtual Result<ImportResult, ImportError> import_file(const FilePath& path) = 0;
};
```

### 12.2. Keep UI out of importers

Bad:

```cpp
if (failed) {
    QMessageBox::warning(...);
}
```

Good:

```cpp
return unexpected(ImportError{
    .message = "OBJ file references missing material library",
    .line = 42,
});
```

The UI decides how to display the error.

### 12.3. Deterministic export

Where possible, export deterministically:

- stable object order
- stable material order
- stable float formatting
- stable line endings

This makes golden tests useful.

---

## 13. OOP patterns to use

### 13.1. Command pattern

Use for undoable actions.

Best for:

- editing operations
- transforms
- selection changes
- import/delete/duplicate
- material assignment

Avoid using commands for tiny internal implementation details. The command should represent a user-visible action.

### 13.2. Strategy pattern

Use for algorithms that genuinely vary.

Examples:

```cpp
class INormalGenerationStrategy {
public:
    virtual ~INormalGenerationStrategy() = default;
    virtual void compute(Polyhedron& mesh) = 0;
};
```

But do not create strategies before you have at least two real strategies or a clear testing need.

### 13.3. Observer pattern, carefully

Use document events to notify UI/render caches.

Good:

```cpp
document_events.push(MeshTopologyChanged{object_id, affected_faces});
```

Bad:

```cpp
global_event_bus.publish("something changed");
```

Avoid stringly typed events. Avoid events that mutate state in surprising order. Events should notify; they should not be the main way business logic happens.

### 13.4. Facade pattern

Use a small facade to expose app-level services:

```cpp
class EditorContext {
public:
    Document& document();
    CommandHistory& commands();
    ToolManager& tools();
};
```

Do not turn the facade into a god object. It should route to services, not implement everything.

### 13.5. Factory pattern

Use for boundary object or plugin creation:

```cpp
std::unique_ptr<IRenderer> create_renderer(GraphicsDevicePolicy policy);
std::unique_ptr<IMeshImporter> create_importer(FileFormat format);
```

Keep factories near boundaries. Do not factory-ize every tiny object.

### 13.6. Pimpl pattern, sparingly

Use Pimpl to hide heavy third-party dependencies from public headers or stabilize ABI.

Good cases:

- renderer GPU/runtime internals
- platform window internals
- file dialog implementation
- heavy importer implementation

Bad cases:

- every small domain class
- `Vec3`
- `VertexId`
- simple commands

Pimpl adds allocation, indirection, and implementation friction. Use it where it pays rent.

### 13.7. Dependency injection without a framework

Use constructor injection:

```cpp
class ToolManager {
public:
    ToolManager(Document& document, CommandHistory& commands, Picker& picker);
};
```

Avoid a large dependency injection framework for a C++ desktop modeling app unless you already have a real problem it solves.

### 13.8. State pattern for complex tools

Use explicit states for tools with multi-step interactions:

- knife tool
- bevel tool
- curve drawing
- retopology
- transform with numeric input

Do not hide state transitions in random booleans.

### 13.9. Patterns to avoid early

Avoid these unless there is a proven need:

- plugin architecture before stable internal APIs
- generic reflection system
- custom runtime type system
- global event bus
- service locator
- entity-component-system for everything
- abstract factory for every type
- template metaprogramming framework
- scripting layer before core operations are stable
- multithreaded document mutation

---

## 14. Software architecture patterns to follow

### 14.1. Layered architecture

Use layers for compile-time and conceptual boundaries.

Pros:

- easy to understand
- easy for agents to navigate
- good compile-time separation
- simple tests

Cons:

- can become rigid if taken too far
- cross-cutting features need careful placement

Use layers as guardrails, not as bureaucracy.

### 14.2. Ports and adapters at external boundaries

External systems should be behind interfaces:

- filesystem
- renderer GPU/runtime layer
- windowing
- clipboard
- native dialogs
- asset database
- import/export formats

This makes the core testable and keeps platform code contained.

### 14.3. Data-oriented core

For mesh and rendering, prefer compact arrays and explicit data flow.

Do not use inheritance-heavy object graphs for millions of elements.

### 14.4. Modular monolith first

Start as one executable with multiple internal libraries.

Do not start with microservices, plugins, IPC, or dynamic module loading. A modeling app is already complex. Keep deployment and debugging simple until the product needs more.

### 14.5. ADRs for major decisions

Use Architecture Decision Records:

```text
docs/decisions/
  ADR-0001-half-edge-storage.md
  ADR-0002-command-history.md
  ADR-0003-render-snapshot-model.md
```

ADR template:

```md
# ADR-0001: Half-edge storage uses record arrays and generational IDs

## Status

Accepted

## Context

We need stable handles for selection, undo, and render cache invalidation.

## Decision

Mesh elements are stored in record arrays. Public references use generational IDs.

## Consequences

Pros:

- stale IDs can be detected
- storage is cache-friendly
- undo patches can refer to records

Cons:

- IDs require validation
- free-list reuse needs care
```

ADRs are excellent for AI agents because they prevent agents from “rediscovering” and reversing important decisions.

---

## 15. Performance traps to avoid

### 15.1. Pointer graph mesh

Trap:

```cpp
std::shared_ptr<Vertex>
std::shared_ptr<Edge>
std::shared_ptr<Face>
```

Why it hurts:

- poor cache locality
- unclear ownership
- cycles
- slow traversal
- hard serialization
- hard undo
- hard deletion
- noisy code

Better:

- arrays of records
- IDs / handles
- explicit free lists
- attributes in separate arrays when useful

### 15.2. Virtual calls in inner loops

Avoid virtual dispatch inside halfedge traversal, normal generation, render extraction, or spatial queries.

Bad:

```cpp
for (const auto& element : mesh.elements()) {
    element->compute_normal_virtual();
}
```

Better:

```cpp
compute_normals(mesh.positions(), mesh.indices(), normals);
```

### 15.3. Per-frame allocations

Avoid allocating in render, picking, hover updates, and hot UI paths.

Use:

- `reserve()`
- scratch buffers
- frame allocators if needed
- reusable vectors
- dirty ranges
- cached acceleration structures

Track allocations in performance builds if possible.

### 15.4. Rebuilding everything after small edits

A selection change should not rebuild topology buffers. A vertex move should not rebuild index buffers. A material rename should not rebuild the mesh.

Use dirty flags that distinguish:

- topology
- positions
- normals
- UVs
- selection overlay
- material assignment
- object transform

### 15.5. Accidental `O(n²)`

Common modeling app traps:

- for each selected vertex, scan all vertices
- for each face, search all edges
- rebuild adjacency from scratch repeatedly
- linearly search object names in hot loops
- recompute normals for the entire mesh after local edits
- update all bounding boxes after one object changes

Prefer local neighborhoods, maps, cached adjacency, and dirty sets.

### 15.6. Overusing `shared_ptr`

`shared_ptr` is not a default ownership model. It means shared ownership, atomic-ish control block overhead, and potential cycles.

Prefer:

- value members
- `std::unique_ptr`
- raw pointer or reference for non-owning access
- IDs for long-lived references
- spans for borrowed ranges

### 15.7. Hidden copies

Watch for accidental copies of:

- meshes
- documents
- large vectors
- render buffers
- attribute arrays
- command histories

Use `const&`, `std::span`, move semantics, and explicit clone functions.

### 15.8. Too many tiny heap objects

Millions of small heap allocations kill locality and complicate ownership.

Bad:

```cpp
std::vector<std::unique_ptr<Halfedge>>
```

Better:

```cpp
std::vector<HalfedgeRecord>
```

### 15.9. Premature multithreading

Do not start by locking the document everywhere.

Better approach:

- single-threaded document mutation
- immutable snapshots for render/background
- background jobs return results
- main thread applies results through commands

### 15.10. Premature GPU cleverness

Do not build an advanced render graph before the editor works.

Start with:

- clear render snapshot
- basic mesh buffer cache
- dirty uploads
- simple material system
- debug overlays

Optimize after benchmarks show where the cost is.

### 15.11. Debug validation in release hot paths

Validation is essential, but full validation can be expensive.

Use tiers:

- local validation after each topology op in debug
- full mesh validation in unit tests and debug commands
- optional heavy validation command in developer tools
- no full validation every frame in release

### 15.12. Cache invalidation bugs

Cache invalidation is one of the central problems in modeling apps.

Rules:

- every mutable system owns dirty flags
- every mutation emits a precise change
- caches are rebuilt from source of truth
- caches are never source of truth
- stale cache detection exists in debug

---

## 16. Testing strategy

### 16.1. Test pyramid

Use multiple test types:

```text
Many:
  unit tests for IDs, mesh records, geometry, operations

Some:
  command tests, document tests, importer/exporter tests

Few:
  UI integration tests, full app tests

Continuous:
  benchmarks and sanitizer builds
```

### 16.2. Mesh operation tests

Every topology operation needs tests for:

- simple valid case
- boundary case
- invalid ID
- non-manifold case
- degenerate geometry
- attribute preservation
- selection remapping
- undo/redo
- validation after operation

Example test shape:

```cpp
TEST(SplitEdge, SplitsInteriorEdgeAndPreservesInvariants) {
    Polyhedron mesh = make_quad_mesh();

    const EdgeId edge = find_edge(mesh, VertexId{0}, VertexId{1});
    const auto result = split_edge(mesh, edge);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(validate_mesh(mesh).ok());
    EXPECT_EQ(mesh.vertex_count(), 5);
}
```

### 16.3. Golden tests

Use small fixture files for operations:

```text
tests/golden/mesh_ops/
  split_edge_quad.input.mesh.json
  split_edge_quad.expected.mesh.json
```

Golden tests are especially helpful for coding agents because expected behavior is concrete.

### 16.4. Fuzz tests

Fuzz topology operations when possible:

- random valid meshes
- random operation sequences
- validate after every operation
- undo all operations and compare to original if supported

### 16.5. Regression tests for every bug

Every fixed bug should get a test. Add the bug ID or issue link in the test name/comment.

---

## 17. Tooling and quality gates

### 17.1. Required repo files

```text
.clang-format
.clang-tidy
CMakePresets.json
AGENTS.md
README.md
docs/architecture.md
docs/code_map.md
```

### 17.2. Formatting

Use `clang-format`. Do not debate formatting in PRs or agent prompts. The formatter decides.

### 17.3. Static analysis

Use `clang-tidy` for:

- naming conventions
- bug-prone checks
- modernize checks
- performance checks
- C++ Core Guidelines checks, selectively

Do not enable every check blindly. Use a profile that the team can keep green.

### 17.4. Sanitizers

Use sanitizer presets:

- AddressSanitizer for memory errors
- UndefinedBehaviorSanitizer for undefined behavior
- ThreadSanitizer later, when threading exists

### 17.5. CMake presets

Use shared presets so humans and agents run the same commands:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "dev",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/dev",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "MODELING_ENABLE_TESTS": "ON",
        "MODELING_ENABLE_WARNINGS_AS_ERRORS": "ON"
      }
    },
    {
      "name": "asan",
      "inherits": "dev",
      "binaryDir": "${sourceDir}/build/asan",
      "cacheVariables": {
        "MODELING_ENABLE_ASAN": "ON"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "dev",
      "configurePreset": "dev"
    },
    {
      "name": "asan",
      "configurePreset": "asan"
    }
  ],
  "testPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "output": {
        "outputOnFailure": true
      }
    }
  ]
}
```

### 17.6. Standard commands

Put these in `AGENTS.md`:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
cmake --preset asan
cmake --build --preset asan
ctest --preset dev
```

Adjust exact commands to your real presets.

---

## 18. Keeping the app clean without over-engineering

### 18.1. Every abstraction must pay rent

Before adding an abstraction, ask:

- Does this hide a real dependency?
- Does this protect an invariant?
- Does this make testing easier?
- Does this enable a real alternate implementation?
- Does this reduce repeated code?
- Would a plain function be clearer?

If not, do not add it.

### 18.2. The “two real users” rule

Do not generalize a system until at least two real call sites need it.

Examples:

- Do not create `IRemesher` until you have multiple remeshers or need a mock.
- Do not create dynamic plugins until static registration hurts.
- Do not create a generic property system until several panels need it.
- Do not create a scripting API until the command API is stable.

### 18.3. Prefer boring composition over inheritance

Bad:

```cpp
class EditableRenderableSelectableSerializableMeshObject
    : public IEditable
    , public IRenderable
    , public ISelectable
    , public ISerializable
{
};
```

Good:

```cpp
class MeshObject {
public:
    ObjectId id;
    Transform transform;
    Polyhedron mesh;
    MaterialAssignments materials;
};
```

Put behavior in services/functions when it does not need inheritance.

### 18.4. Avoid “manager soup”

Too many managers are a smell:

```text
MeshManager
ObjectManager
SelectionManager
RenderManager
ToolManager
CommandManager
PanelManager
FileManager
StateManager
```

Some managers are legitimate. But when everything is a manager, ownership is unclear.

Prefer names that say what the type owns or does:

```text
Document
ObjectStore
Selection
CommandHistory
ToolRegistry
RenderScene
MeshRenderCache
```

### 18.5. Keep files cohesive, not tiny for sport

A good file contains one cohesive idea.

Bad extremes:

- one 8,000-line `mesh.cpp`
- 300 files with one trivial method each

Recommended:

- one header/source pair per major type or operation
- group tiny related records in one header
- split algorithms by operation
- keep public API headers readable

### 18.6. Avoid speculative plugin systems

Plugin systems are expensive:

- ABI boundaries
- versioning
- crash isolation
- dependency loading
- security
- testing
- documentation

Start with internal interfaces and static registration. Add dynamic plugins after the internal API is stable.

### 18.7. Do not build a framework inside the app

The goal is a modeling app, not a universal engine.

Avoid:

- custom reflection framework
- custom object database
- custom dependency injection container
- custom scripting language
- generic node graph for every operation
- generalized event processing framework
- generalized serialization framework before formats are clear

Build the smallest thing that makes the next feature clean.

---

## 19. Agentic-AI-friendly codebase rules

The whole app will be made by agentic AI, so design the repository as if every future contributor has no memory and only the current files.

### 19.1. Add `AGENTS.md`

Create a root `AGENTS.md` that tells coding agents:

- what the project is
- how to configure
- how to build
- how to test
- code style
- architecture boundaries
- common mistakes to avoid
- when to update docs
- where to add tests
- what not to touch without explicit instruction

Keep it concise. Put long architecture details in `docs/architecture.md` and link to them.

Recommended root `AGENTS.md`:

````md
# AGENTS.md

## Project purpose

This is a C++ polygon modeling app. The mesh core uses a half-edge-style topology with stable generational IDs. The editor is command-driven so all user-visible document mutations can support undo/redo.

## Standard commands

Configure:

```sh
cmake --preset dev
```
````

Build:

```sh
cmake --build --preset dev
```

Test:

```sh
ctest --preset dev
```

Format:

```sh
clang-format -i <changed .hpp/.cpp files>
```

## Architecture rules

- `mesh` must not depend on `ui`, `crimson`, `tools`, or `commands`.
- `geometry` must not depend on `document`.
- `commands` may mutate documents; tools and UI should create commands instead of directly editing meshes.
- Renderer code consumes snapshots/cache data and must not become the source of truth.
- Public IDs are strong types such as `VertexId`, `EdgeId`, `FaceId`; do not pass raw indices across module boundaries.
- Add or update tests for every behavior change.
- Run mesh validation in tests after topology operations.

## Code style

- Classes/structs: `PascalCase`.
- Functions/variables: `snake_case`.
- Private members: trailing underscore.
- Constants: `kPascalCase`.
- Prefer simple C++20.
- Prefer RAII and value ownership.
- Avoid global mutable state and singletons.
- Avoid `shared_ptr` unless ownership is genuinely shared.

## Mesh operation rules

When adding a mesh operation:

1. Put the public declaration in `src/mesh/ops/<operation>.hpp`.
2. Put implementation in `src/mesh/ops/<operation>.cpp`.
3. Document preconditions and affected elements.
4. Validate IDs before mutation.
5. Update attributes and selection remapping if relevant.
6. Add unit tests under `tests/unit/mesh/`.
7. Add golden tests for non-trivial topology changes.
8. Ensure `validate_mesh(mesh)` passes after the operation.

## Do not

- Do not implement topology operations in UI callbacks.
- Do not add a new framework or dependency without explicit approval.
- Do not perform broad unrelated rewrites.
- Do not change architecture boundaries to fix a local compile error.
- Do not store raw pointers to mesh elements in commands, selection, UI state, or renderer caches.

````

### 19.2. Use nested `AGENTS.md` files for specialized modules

Example:

```text
src/mesh/AGENTS.md
src/crimson/AGENTS.md
src/ui/AGENTS.md
````

`src/mesh/AGENTS.md` can say:

```md
# Mesh module rules

- This module owns topology and mesh attributes.
- It must not include UI, renderer, tools, or commands headers.
- All topology mutations must preserve half-edge invariants.
- Add tests for boundary, degenerate, and invalid-ID cases.
- Prefer record arrays and IDs over pointer graphs.
```

This helps agents follow local rules without loading a massive root instruction file.

### 19.3. Maintain `docs/code_map.md`

Agents perform better when they can find the right files quickly.

Example:

```md
# Code map

## Mesh core

- `src/mesh/core/polyhedron.hpp`: public mesh data structure API.
- `src/mesh/core/mesh_records.hpp`: internal record types for vertices, halfedges, edges, faces.
- `src/mesh/core/mesh_validation.hpp`: invariant checks.

## Mesh operations

- `src/mesh/ops/split_edge.*`: splits one edge and interpolates attributes.
- `src/mesh/ops/collapse_edge.*`: collapses an edge with manifold checks.
- `src/mesh/ops/extrude_face.*`: creates side faces and updates selection.

## Commands

- `src/commands/command.hpp`: base command interface.
- `src/commands/command_history.*`: undo/redo stack.
- `src/commands/mesh_commands.*`: user-visible mesh commands.
```

Update this whenever you add major files.

### 19.4. Use task specs for agent work

Create `docs/tasks/TASK-template.md`:

```md
# TASK: <title>

## Goal

Describe the desired behavior in user terms.

## Relevant files

- `src/...`
- `tests/...`

## Architecture constraints

- Do not edit `...`
- Keep dependency direction unchanged.
- Use commands for document mutation.

## Acceptance criteria

- [ ] Behavior works.
- [ ] Unit tests added.
- [ ] Golden tests updated if topology changes.
- [ ] `ctest --preset dev` passes.
- [ ] Docs updated if public behavior changed.

## Non-goals

- Do not redesign unrelated systems.
- Do not add dependencies.
```

Agents should work from task specs, not vague prompts.

### 19.5. Write code for local reasoning

AI agents struggle with code that requires hidden context. Make code locally understandable:

- descriptive names
- explicit types at boundaries
- small cohesive functions
- direct includes
- no macro magic
- no hidden global registries
- comments for invariants and “why”
- tests near behavior
- clear file names

### 19.6. Prefer explicit errors over mysterious booleans

Bad:

```cpp
bool collapse_edge(Polyhedron& mesh, EdgeId edge);
```

Good:

```cpp
Result<CollapseEdgeResult, MeshEditError> collapse_edge(Polyhedron& mesh, EdgeId edge);
```

Structured errors help humans, tests, and agents.

### 19.7. Give agents executable guardrails

Documentation is useful, but tests and checks are better.

For every architectural rule, ask whether it can be enforced:

- dependency checks
- unit tests
- mesh validation
- clang-tidy
- formatting
- CMake target boundaries
- CI presets
- golden tests

### 19.8. Keep instructions short and durable

Do not put long essays in `AGENTS.md`. Agents have context limits and instruction files can be truncated or ignored by specific tools.

Put durable, always-on rules in `AGENTS.md`. Put detailed explanations in `docs/`.

### 19.9. Keep an agent mistake ledger

Create:

```text
docs/agent_mistakes.md
```

Example:

```md
# Agent mistake ledger

## 2026-07-01: Mesh operation added UI include

Problem:
An agent included `ui/viewport_panel.hpp` from `mesh/ops/extrude_face.cpp`.

Rule:
Mesh operations must never include UI headers. Use operation results and commands.

Prevention:
Architecture check now rejects `src/mesh/**` including `src/ui/**`.
```

This turns repeated failures into repo knowledge.

---

## 20. Naming and style rules

Use one naming convention and enforce it.

Recommended:

| Thing          | Rule                          | Example              |
| -------------- | ----------------------------- | -------------------- |
| Namespace      | `lower_snake_case`            | `modeling::mesh`     |
| Class / struct | `PascalCase`                  | `Polyhedron`       |
| Enum           | `PascalCase`                  | `SelectionMode`      |
| Enum value     | `PascalCase` or `kPascalCase` | `Vertex`, `Boundary` |
| Function       | `snake_case`                  | `split_edge()`       |
| Variable       | `snake_case`                  | `selected_faces`     |
| Private member | trailing `_`                  | `vertices_`          |
| Constant       | `kPascalCase`                 | `kInvalidIndex`      |
| Macro          | `ALL_CAPS`                    | `MODELING_ASSERT`    |
| File           | `snake_case`                  | `polyhedron.hpp` |

Avoid clever abbreviations. Use domain terms consistently:

```text
vertex
halfedge
edge
face
loop
boundary
selection
document
object
command
tool
snapshot
```

Do not alternate between `halfedge`, `half_edge`, and `halfEdge` in type names. Pick one spelling. I recommend:

```cpp
HalfedgeId      // type name
halfedge        // variable name
polyhedron  // file/module name, if you prefer readability in filenames
```

But consistency matters more than this exact choice.

---

## 21. Error handling policy

Use clear error handling by layer.

### 21.1. Core mesh operations

Use `Result<T, MeshEditError>` for expected failures:

- invalid ID
- non-manifold operation
- boundary not supported
- degenerate geometry
- attribute mismatch

Do not throw exceptions for normal edit rejection.

### 21.2. I/O

Use structured errors with diagnostics:

```cpp
struct ImportDiagnostic {
    ImportSeverity severity;
    std::string message;
    std::optional<uint32_t> line;
    std::optional<uint32_t> column;
};
```

### 21.3. Programmer errors

Use assertions for broken invariants and impossible states:

```cpp
MODELING_ASSERT(mesh.is_valid(edge));
```

Assertions are not user-facing error handling.

### 21.4. UI

The UI translates errors into user messages. It should not invent low-level diagnostics.

---

## 22. Memory and ownership policy

### 22.1. Ownership vocabulary

Use these conventions:

| Situation                            | Preferred type                       |
| ------------------------------------ | ------------------------------------ |
| exclusive ownership                  | `std::unique_ptr<T>`                 |
| shared ownership                     | `std::shared_ptr<T>`, only when real |
| non-owning required reference        | `T&`                                 |
| non-owning optional reference        | `T*`                                 |
| long-lived reference to mesh element | strong ID                            |
| borrowed contiguous data             | `std::span<T>`                       |
| owned list                           | `std::vector<T>`                     |

### 22.2. RAII

Every resource should have a clear owner:

- files
- GPU buffers
- windows
- command transactions
- temporary mesh edits
- locks
- imported assets

Do not expose manual `init()` / `shutdown()` pairs unless unavoidable. Prefer constructors/destructors and small RAII wrappers.

### 22.3. Avoid ownership ambiguity

Bad:

```cpp
class Tool {
    Document* document_; // Does this own? Can it be null?
};
```

Better:

```cpp
class Tool {
public:
    explicit Tool(Document& document);

private:
    Document& document_; // non-owning, required
};
```

If it can be null:

```cpp
Document* active_document_ = nullptr; // non-owning, optional
```

Document this in the member comment if not obvious.

---

## 23. Concurrency policy

Start simple:

> Document mutation is single-threaded. Background work uses immutable inputs and returns results to be applied by commands.

Allowed background tasks:

- import parsing
- export writing from snapshot
- normal generation from snapshot
- remeshing copy of mesh
- BVH construction from snapshot
- thumbnail generation

Avoid:

- renderer mutating document
- background operation hand-editing live mesh
- locks sprinkled across mesh records
- shared mutable selection state
- command history accessed from worker threads

When a background job completes, apply its result on the main/editor thread.

---

## 24. Serialization policy

Separate internal storage from file format.

Do not expose internal halfedge records directly as the public saved file format unless you are sure the storage will remain stable.

Use a project file schema that can evolve:

```text
project version
objects
meshes
materials
transforms
selection, optionally
metadata
```

Rules:

- include schema version
- support migration
- write deterministic output when possible
- do not serialize raw memory
- do not serialize raw pointers
- include unit tests for old versions

---

## 25. Logging and diagnostics

Use structured logs for developer diagnostics:

```cpp
LOG_DEBUG("split_edge: object={}, edge={}", object_id, edge_id);
LOG_WARN("import: missing normal data, generating normals");
LOG_ERROR("mesh validation failed: {}", report.summary());
```

Do not spam logs in hot loops. Provide debug commands to inspect:

- selected element IDs
- mesh validation report
- dirty flags
- render cache state
- command history
- active tool state

These tools help both humans and agents debug.

---

## 26. Practical development sequence

Build the app in this order:

### Phase 1: Core skeleton

- CMake targets
- formatting/static-analysis setup
- `AGENTS.md`
- `docs/architecture.md`
- math types
- strong IDs
- result/error type
- basic tests

### Phase 2: Mesh core

- half-edge records
- create/delete primitives
- validation
- simple mesh fixtures
- traversal helpers
- unit tests

### Phase 3: Mesh operations

- split edge
- flip edge
- collapse edge
- delete face
- extrude face
- attribute handling
- golden tests

### Phase 4: Document and commands

- document object store
- selection
- command interface
- command history
- undo/redo tests
- mesh commands

### Phase 5: Minimal UI and viewport

- viewport
- selection tool
- transform tool
- render snapshot
- mesh render cache
- simple overlays

### Phase 6: I/O

- import OBJ or simple internal format
- export simple format
- diagnostics
- deterministic tests

### Phase 7: Performance and polish

- benchmarks
- dirty-buffer uploads
- spatial picking acceleration
- operation profiling
- sanitizer CI
- more tools

Avoid starting with advanced UI, plugins, or rendering before the model and commands are solid.

---

## 27. Code review checklist

Use this for humans and agents.

```md
## Architecture

- [ ] Does this preserve dependency direction?
- [ ] Is document mutation command-driven?
- [ ] Is UI/render code kept out of mesh/core algorithms?
- [ ] Is ownership clear?
- [ ] Are IDs used instead of raw mesh pointers across systems?

## Correctness

- [ ] Are preconditions checked?
- [ ] Are errors structured?
- [ ] Are invariants validated in tests/debug?
- [ ] Does undo/redo work if this is user-visible?
- [ ] Are boundary and degenerate cases tested?

## Performance

- [ ] Any accidental large copies?
- [ ] Any per-frame allocations?
- [ ] Any `O(n²)` scans on large meshes?
- [ ] Any unnecessary full-buffer rebuilds?
- [ ] Any virtual dispatch in hot loops?

## Maintainability

- [ ] Is the public API understandable from the header?
- [ ] Are names consistent with the style guide?
- [ ] Are files cohesive?
- [ ] Is the abstraction necessary?
- [ ] Did docs/code map need updating?

## Agent safety

- [ ] Did the change stay within requested files/scope?
- [ ] Were tests added or updated?
- [ ] Were standard build/test commands run?
- [ ] Did the agent avoid broad unrelated rewrites?
```

---

## 28. “Do this, not that” summary

| Avoid                                 | Prefer                                                       |
| ------------------------------------- | ------------------------------------------------------------ |
| mesh elements as `shared_ptr` graph   | arrays of records + strong IDs                               |
| UI directly edits topology            | UI creates commands                                          |
| renderer reads mutable mesh internals | renderer consumes snapshots/caches                           |
| giant `MeshManager` god object        | `Polyhedron`, `Selection`, `CommandHistory`, `RenderScene` |
| global event bus                      | typed document change queue                                  |
| one vague `changed` flag              | precise dirty flags                                          |
| abstract factory everywhere           | concrete types until variation exists                        |
| dynamic plugins on day one            | static registration                                          |
| raw integers for IDs                  | `VertexId`, `EdgeId`, `FaceId`                               |
| hidden transitive includes            | direct self-contained headers                                |
| comments only for invariants          | executable validation                                        |
| “AI, just figure it out”              | task specs + `AGENTS.md` + tests                             |

---

## 29. Final architecture promise

The project should feel like this:

- A new mesh operation has an obvious place.
- A new command has an obvious place.
- A new tool has an obvious place.
- A new Crimson renderer feature has an obvious place.
- A new importer has an obvious place.
- A failing invariant points to a small local area.
- A coding agent can read `AGENTS.md`, `docs/code_map.md`, and the target header, then make a safe change.
- No system needs secret knowledge from another system.
- Public APIs are narrow.
- Data flow is visible.
- Performance decisions are measurable.
- Over-engineering is rejected unless it solves a current problem.

That is the architecture to protect.

---

## 30. References and external anchors

These are not rules to copy blindly, but they support the recommendations in this guide.

- C++ Core Guidelines: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines
  - RAII and ownership guidance.
  - Self-contained headers.
  - Explicit, strongly typed interfaces.
- Google C++ Style Guide: https://google.github.io/styleguide/cppguide.html
  - Forward declaration tradeoffs.
  - Self-contained headers.
  - Include discipline.
- LLVM Coding Standards: https://llvm.org/docs/CodingStandards.html
  - Include ordering and self-contained header guidance.
- Clang-format documentation: https://clang.llvm.org/docs/ClangFormatStyleOptions.html
  - Project-local `.clang-format`.
- Clang-tidy identifier naming check: https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html
  - Enforce naming conventions.
- Clang AddressSanitizer documentation: https://clang.llvm.org/docs/AddressSanitizer.html
  - Detect memory errors such as out-of-bounds and use-after-free.
- CMake presets documentation: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
  - Shared configure/build/test presets.
- AGENTS.md: https://agents.md/
  - A predictable instruction file for coding agents.
- OpenAI Codex AGENTS.md guide: https://developers.openai.com/codex/guides/agents-md
  - Project and nested instruction files for coding agents.

UI-specific Qt references live in [ui.md](ui.md#2-ui-references-and-external-anchors).
