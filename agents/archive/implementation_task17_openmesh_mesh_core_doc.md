# Implementation Plan: Task #17 OpenMesh Behind Quader Mesh Core

## A19 Corrected-Objective Addendum (2026-07-05)

This addendum is authoritative for continuing task #17 after the A18 software-authority review. The corrected objective is not to merely add OpenMesh as a dependency or prepare APIs indefinitely. Task #17 must swap Quader's current vector-backed topology storage to a private OpenMesh `PolyMesh` backend while preserving Quader-owned public IDs, generations, validation diagnostics, attributes, and document/command/I/O APIs.

Completed preparatory work:

- A14 completed Slices 0/1 as preparation only: OpenMesh 11.0.0 is pinned through FetchContent, `modeling_mesh_core` is a compiled static target, OpenMesh is linked privately through `OpenMeshCore`, placeholder mesh-core translation units exist, and include-leakage guardrails are present.
- A16 completed Slice 2 as preparation only: public mutable mesh record accessors were removed, Quader-owned query/mutation APIs were added, traversal/tests/I/O were moved to public APIs, and validation corruption tests are isolated behind test-only hooks.
- A14 and A16 do not complete #17. #17 remains incomplete until the actual topology storage swap, Quader ID/handle mapping, deletion/generation/compaction policy, validation rewrite, and focused tests are implemented and reviewed.

Next authorized implementation begins at Slice 3: Private OpenMesh Storage And Mapping Tables. The executor must replace `Polyhedron`'s authoritative `std::vector<VertexRecord>`, `std::vector<HalfedgeRecord>`, `std::vector<EdgeRecord>`, and `std::vector<FaceRecord>` topology with private OpenMesh storage. A short compile-preserving adapter is allowed only inside the storage-swap work and must not survive final authority review.

Concrete storage-swap requirements for the next implementation:

- Add private OpenMesh storage under `src/mesh/core/detail/` using `OpenMesh::PolyMesh_ArrayKernelT<detail::QuaderOpenMeshTraits>`.
- Add Quader-owned ID-to-handle and handle-to-ID maps for vertices, halfedges, edges, and faces. Each map owns stable Quader slots, alive/deleted state, generation, free-list reuse, current OpenMesh handle, and reverse lookup.
- Deletion marks Quader IDs stale immediately, increments generation while skipping generation `0`, clears reverse mappings for deleted backend handles, and releases Quader slots for later reuse independently of OpenMesh garbage collection.
- Add explicit `compact_storage()` using OpenMesh garbage collection with handle tracking. It must preserve all live Quader IDs, generations, and attributes, rebuild reverse maps, refresh edge mappings from representative halfedges, and validate after compaction.
- Rewrite validation to check both OpenMesh connectivity and Quader mapping consistency. Diagnostics must describe Quader concepts first.
- Keep OpenMesh out of public Quader headers and out of document, command, I/O, UI, and Crimson contracts.
- Add/maintain behavior tests for stale IDs, delete/reuse generations, face rejection without partial mutation, mapping corruption, compaction, copy/move semantics, and I/O invalid-mesh rejection.
- Do not weaken the verification commands in this plan.

Authority:

- Project-board task: `#17 [priority:high][type:enhancement][area:mesh] Add OpenMesh behind Quader mesh types`.
- Active master audit: `agents/plans/audit_20260704_full_architecture_master.md`, especially Batch 5 "Mesh core and validation" and the global boundary decisions.
- Required planning assignment: A2, software architecture only. No code implementation is authorized by this plan file alone.
- Tests policy applies: `agents/tests-policy.md` was read before writing the test strategy.

This plan revises the original Batch 5 storage assumption: Quader keeps its own public mesh contracts and stable generational IDs, but the private half-edge connectivity backend becomes OpenMesh.

## Current Workspace Findings

The implementation agent should not repeat broad discovery. The important current facts are:

- `modeling_mesh_core` is now a compiled `STATIC` CMake target in `CMakeLists.txt`.
- OpenMesh 11.0.0 is already pinned through FetchContent with SHA256 `9d22e65bdd6a125ac2043350a019ec4346ea83922cafdf47e125a03c16f6fa07`.
- `modeling_mesh_core` already links `OpenMeshCore` privately. OpenMesh is not linked directly by document, commands, I/O, UI, Crimson, or the app executable.
- `src/mesh/core/polyhedron.cpp` and `src/mesh/core/mesh_validation.cpp` exist, but `polyhedron.cpp` is currently only a placeholder include.
- `tools/check_architecture.py` already rejects `OpenMesh/` includes outside `src/mesh/core/detail/`, `src/mesh/core/polyhedron.cpp`, and `src/mesh/core/mesh_validation.cpp`.
- The actual authoritative topology storage is still vector-backed and still lives directly in `src/mesh/core/polyhedron.hpp`:
  - `std::vector<VertexRecord> vertices_`
  - `std::vector<HalfedgeRecord> halfedges_`
  - `std::vector<EdgeRecord> edges_`
  - `std::vector<FaceRecord> faces_`
  - free-list vectors for each element type
  - `MeshAttributes attributes_`
- Public mutable storage accessors have already been removed. Production code no longer calls `vertex_records`, `halfedge_records`, `edge_records`, `face_records`, or mutable `*_record()` accessors.
- `src/mesh/core/mesh_traversal.hpp` already uses public mesh query methods.
- `src/mesh/core/mesh_validation.cpp` has moved validation implementation out of the public header, but it still validates the current private vector records through `Polyhedron` friendship. It does not yet validate OpenMesh connectivity or Quader ID/handle maps.
- `tests/unit/mesh/mesh_corruption_fixtures.hpp` currently corrupts private vector records through `QUADER_MESH_CORE_TEST_HOOKS`. This is acceptable only as completed Slice 2 preparation. The helper must be replaced or retargeted to OpenMesh/mapping corruption once the private storage layer exists.
- `tests/unit/io/io_document_fragment_tests.cpp` and `tests/unit/io/io_import_export_service_tests.cpp` now use the test-only corruption fixture instead of public mutable record access.
- `Document`, `ObjectStore`, import fragments, and commands store `quader::mesh::Polyhedron` by value. This is acceptable; `Polyhedron` must remain a Quader-owned value type with move support and preferably deep-copy support.
- `Selection` stores only `ObjectId` plus `VertexId`/`EdgeId`/`FaceId`. That contract already matches the stable public ID requirement.
- #14 GoogleTest and #16 GLM work are implemented and software-authority approved on the board, though they are still active entries awaiting user validation. #17 must use current GoogleTest/CTest infrastructure and continue targeting Quader public `Vec3`; it must not expose glm or OpenMesh publicly.
- The board marks #17 stale until this amended plan is integrated. After PM/root integration, implementation should proceed from the storage-swap slice, not from dependency/API preparation.

## External Dependency References

OpenMesh should be pinned to the current official 11.0.0 source release unless PM or the user explicitly authorizes a different version.

Checked references:

- Official OpenMesh overview and 11.0 release note: https://www.graphics.rwth-aachen.de/software/openmesh/
- Official OpenMesh download page: https://www.graphics.rwth-aachen.de/software/openmesh/download/
- Official OpenMesh CMake usage page, showing `find_package(OpenMesh)` and targets `OpenMeshCore` / `OpenMeshTools`: https://www.graphics.rwth-aachen.de/media/openmesh_static/Documentations/OpenMesh-Doc-Latest/a06342.html
- Official OpenMesh license page: https://www.graphics.rwth-aachen.de/software/openmesh/license/
- Official OpenMesh ArrayKernel garbage-collection documentation, noting that OpenMesh deletion marks elements and garbage collection invalidates handles unless tracked handles are passed for update: https://www.graphics.rwth-aachen.de/media/openmesh_static/Documentations/OpenMesh-8.1-Documentation/a02113.html

Dependency decision:

- Use the official OpenMesh 11.0.0 source archive, not an unpinned branch.
- Prefer the official `tar.bz2` source archive:
  - `https://www.graphics.rwth-aachen.de/media/openmesh_static/Releases/11.0/OpenMesh-11.0.0.tar.bz2`
  - Expected SHA256 from current package metadata: `9d22e65bdd6a125ac2043350a019ec4346ea83922cafdf47e125a03c16f6fa07`
- Use `FetchContent_Declare(openmesh URL ... URL_HASH SHA256=...)`.
- Do not use the OpenMesh git repository for this task; the official download page says recursive clone is required for git, which adds unnecessary submodule drift.
- Disable OpenMesh apps before `FetchContent_MakeAvailable(openmesh)`. Start with OpenMesh's known `BUILD_APPS=OFF` cache option, then verify configure output.
- Link only `OpenMeshCore` unless implementation proves `OpenMeshTools` is required. Task #17 does not require OpenMesh I/O, decimation, hole filling, or tools.
- Keep the OpenMesh dependency private to `modeling_mesh_core`.

If configure does not expose `OpenMeshCore` as a target after the pinned archive is fetched, stop and return a Workaround/Deviation Report. Do not wrap an untracked local installation, do not silently switch to an unpinned mirror, and do not expose OpenMesh as a public dependency.

## Architecture Decisions

### AD-17-01: OpenMesh Is A Private Backend

Public Quader contracts remain:

- `quader::mesh::Polyhedron`
- `VertexId`
- `HalfedgeId`
- `EdgeId`
- `FaceId`
- `AttributeId`
- `MeshError`, `MeshValidationReport`, traversal helper APIs
- document/command APIs that take or return `Polyhedron`

OpenMesh types must not appear in:

- public `src/mesh/core/*.hpp` headers, except private files under `src/mesh/core/detail/`
- `src/document/**`
- `src/commands/**`
- `src/io/**`
- `src/ui/**`
- `src/crimson/**`
- tests outside explicitly named mesh-core private validation tests

Forbidden public leaks include:

- `OpenMesh::VertexHandle`
- `OpenMesh::HalfedgeHandle`
- `OpenMesh::EdgeHandle`
- `OpenMesh::FaceHandle`
- `OpenMesh::PolyMesh_ArrayKernelT`
- OpenMesh traits
- OpenMesh status/property handle types
- OpenMesh vector types

### AD-17-02: Use PolyMesh, Not TriMesh

Use `OpenMesh::PolyMesh_ArrayKernelT<detail::QuaderOpenMeshTraits>` because Quader is a polygon modeling app and current `create_face(std::span<const VertexId>)` accepts arbitrary polygon faces with at least three vertices.

The private OpenMesh point type can be `OpenMesh::Vec3f` for this task. Convert at the backend boundary:

- `quader::math::Vec3` to `OpenMesh::Vec3f`
- `OpenMesh::Vec3f` to `quader::math::Vec3`

Do not add glm conversion logic in #17. If #16 lands first, the conversion still targets the Quader public `Vec3` contract.

### AD-17-03: Stable Quader IDs Are Separate From OpenMesh Handles

Do not alias Quader ID index to OpenMesh handle index as an architectural contract.

Each element domain must have a Quader-owned mapping table:

- vertices: `VertexId <-> OpenMesh::VertexHandle`
- halfedges: `HalfedgeId <-> OpenMesh::HalfedgeHandle`
- edges: `EdgeId <-> OpenMesh::EdgeHandle`, with a representative `HalfedgeId` for compaction repair
- faces: `FaceId <-> OpenMesh::FaceHandle`

The mapping table owns:

- stable Quader slot index
- generation
- alive/deleted state
- current OpenMesh handle
- reverse lookup from OpenMesh handle index to Quader slot index

The public ID generation remains Quader-owned. OpenMesh handle status is not sufficient because stale IDs must reject even if a backend slot index gets reused.

### AD-17-04: No Dual Mesh Truth

The final implementation must not keep both the current record vectors and OpenMesh as authoritative topology.

Allowed retained data:

- Quader ID mapping slots
- Quader mesh attributes, indexed by Quader ID slot
- small cached counts if validated against mapping/backend updates
- optional diagnostic snapshots produced on demand

Not allowed in the final implementation:

- mirrored `VertexRecord`/`HalfedgeRecord`/`EdgeRecord`/`FaceRecord` arrays that must be kept synchronized with OpenMesh
- a legacy vector-backed mesh updated alongside OpenMesh
- OpenMesh handles stored in document, command, selection, UI, I/O, or renderer state

A short migration adapter is allowed only inside this task if it is necessary to keep the tree compiling between slices. It must be removed before authority review. If the adapter cannot be removed without changing scope, stop and return a Workaround/Deviation Report.

### AD-17-05: Public Access Becomes Query/Mutation APIs Or Value Views

Replace public mutable record access with explicit APIs:

Query APIs should return IDs, values, or immutable value views. Mutation APIs should express the intended edit.

Required public query/mutation surface:

```cpp
class Polyhedron final {
public:
    Polyhedron();
    ~Polyhedron();
    Polyhedron(const Polyhedron&);
    Polyhedron& operator=(const Polyhedron&);
    Polyhedron(Polyhedron&&) noexcept;
    Polyhedron& operator=(Polyhedron&&) noexcept;

    [[nodiscard]] VertexId create_vertex(quader::math::Vec3 position);
    [[nodiscard]] quader::foundation::Result<void, MeshError> delete_vertex(VertexId vertex);
    [[nodiscard]] quader::foundation::Result<FaceId, MeshError> create_face(std::span<const VertexId> vertices);
    [[nodiscard]] quader::foundation::Result<void, MeshError> delete_face(FaceId face);

    [[nodiscard]] bool is_valid(VertexId id) const noexcept;
    [[nodiscard]] bool is_valid(HalfedgeId id) const noexcept;
    [[nodiscard]] bool is_valid(EdgeId id) const noexcept;
    [[nodiscard]] bool is_valid(FaceId id) const noexcept;

    [[nodiscard]] std::size_t vertex_count() const noexcept;
    [[nodiscard]] std::size_t halfedge_count() const noexcept;
    [[nodiscard]] std::size_t edge_count() const noexcept;
    [[nodiscard]] std::size_t face_count() const noexcept;

    [[nodiscard]] std::vector<VertexId> vertex_ids() const;
    [[nodiscard]] std::vector<HalfedgeId> halfedge_ids() const;
    [[nodiscard]] std::vector<EdgeId> edge_ids() const;
    [[nodiscard]] std::vector<FaceId> face_ids() const;

    [[nodiscard]] quader::foundation::Result<quader::math::Vec3, MeshError> vertex_position(VertexId vertex) const;
    [[nodiscard]] quader::foundation::Result<void, MeshError> set_vertex_position(VertexId vertex, quader::math::Vec3 position);

    [[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> vertex_outgoing_halfedge(VertexId vertex) const;
    [[nodiscard]] quader::foundation::Result<VertexId, MeshError> halfedge_origin(HalfedgeId halfedge) const;
    [[nodiscard]] quader::foundation::Result<VertexId, MeshError> halfedge_target(HalfedgeId halfedge) const;
    [[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> opposite_halfedge(HalfedgeId halfedge) const;
    [[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> next_halfedge(HalfedgeId halfedge) const;
    [[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> previous_halfedge(HalfedgeId halfedge) const;
    [[nodiscard]] quader::foundation::Result<EdgeId, MeshError> halfedge_edge(HalfedgeId halfedge) const;
    [[nodiscard]] quader::foundation::Result<FaceId, MeshError> halfedge_face(HalfedgeId halfedge) const;
    [[nodiscard]] quader::foundation::Result<bool, MeshError> is_boundary_halfedge(HalfedgeId halfedge) const;

    [[nodiscard]] quader::foundation::Result<std::vector<HalfedgeId>, MeshError> face_halfedges(FaceId face) const;
    [[nodiscard]] quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError> edge_halfedges(EdgeId edge) const;

    [[nodiscard]] quader::foundation::Result<MeshCompactionReport, MeshError> compact_storage();

    [[nodiscard]] MeshAttributes& attributes() noexcept;
    [[nodiscard]] const MeshAttributes& attributes() const noexcept;
};
```

The exact names can be adjusted to fit local style, but the final API must not expose mutable record spans or OpenMesh types.

Optional immutable value views:

```cpp
struct VertexView {
    VertexId id;
    quader::math::Vec3 position;
    HalfedgeId outgoing_halfedge;
};

struct HalfedgeView {
    HalfedgeId id;
    bool boundary = false;
    VertexId origin;
    VertexId target;
    HalfedgeId opposite;
    HalfedgeId next;
    HalfedgeId prev;
    FaceId face;
    EdgeId edge;
};

struct EdgeView {
    EdgeId id;
    std::array<HalfedgeId, 2> halfedges;
};

struct FaceView {
    FaceId id;
    std::vector<HalfedgeId> halfedges;
};
```

If value views are added, they are snapshots. Mutating a view must not mutate the mesh.

## Expected Files And Classes

### CMake

Current status:

- OpenMesh FetchContent, the private `OpenMeshCore` link, and the compiled `modeling_mesh_core` target are already complete from A14.
- The storage-swap worker should edit `CMakeLists.txt` only to add new mesh-core detail source/header files and any narrowly required target-local options.
- The existing public module dependencies must remain:
  - `PUBLIC modeling_foundation`
  - `PUBLIC modeling_math`
  - `PUBLIC modeling_geometry`
- OpenMesh must remain private:
  - `PRIVATE OpenMeshCore`
- Do not link OpenMesh to `modeling_document`, `modeling_commands`, `modeling_ui`, `crimson`, or the app executable directly.

Planned CMake shape:

```cmake
set(QUADER_OPENMESH_VERSION 11.0.0)
set(QUADER_OPENMESH_SHA256 9d22e65bdd6a125ac2043350a019ec4346ea83922cafdf47e125a03c16f6fa07)

set(BUILD_APPS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    openmesh
    URL "https://www.graphics.rwth-aachen.de/media/openmesh_static/Releases/11.0/OpenMesh-11.0.0.tar.bz2"
    URL_HASH "SHA256=${QUADER_OPENMESH_SHA256}"
)
FetchContent_MakeAvailable(openmesh)

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
    src/mesh/core/detail/openmesh_storage.hpp
    src/mesh/core/detail/openmesh_id_map.hpp
    src/mesh/core/detail/openmesh_traits.hpp
)
```

The exact OpenMesh cache options may need minor adjustment after configure. If the implementation needs options beyond disabling apps and tests, record them in the final report and update docs if they affect developer setup.

### Mesh Core Public Files

`src/mesh/core/mesh_ids.hpp`

- Keep current ID typedefs unchanged.
- Do not add OpenMesh includes.

`src/mesh/core/mesh_error.hpp`

- Keep existing errors.
- Add errors as needed:
  - `NonManifoldTopology`
  - `BackendRejectedFace`
  - `BackendMappingCorrupt`
  - `CompactionFailed`
  - `InvalidGeometry`
- Diagnostics must describe Quader concepts, not OpenMesh internals.

`src/mesh/core/mesh_attributes.hpp`

- Keep `MeshAttributes` as Quader-owned data.
- Continue indexing attributes by Quader ID slot, not OpenMesh handle index.
- Add domain resize/reset helpers if needed:
  - `ensure_domain_slot(AttributeDomain domain, IdIndex slot)`
  - `domain_slot_count(AttributeDomain domain) const`
- Do not store OpenMesh property handles in `MeshAttributes` for this task.

`src/mesh/core/polyhedron.hpp`

- Convert from header-only implementation to declarations plus a private storage pointer.
- Public header may include `mesh_attributes.hpp`, `mesh_error.hpp`, `mesh_ids.hpp`, math, result, span/vector/array/memory.
- Public header must not include `mesh_records.hpp` or any OpenMesh header after the migration is complete.
- Preserve value semantics:
  - move construct/assign
  - deep copy construct/assign, unless implementation proves no current caller needs copy; if copy is removed, stop for architect review because it changes the document/import value contract.

`src/mesh/core/mesh_records.hpp`

- Preferred final state: remove from public use and replace with `mesh_element_views.hpp` if immutable views are needed.
- If deleting the file causes too much churn, leave it as a compatibility include that contains only immutable view structs and no mutable storage records. Do not keep `alive`, `generation`, or backend linkage as public mutable fields.

`src/mesh/core/mesh_traversal.hpp`

- Rewrite to use public query methods only.
- `face_vertices`, `face_edges`, and `find_halfedge` must not read record spans.
- `find_halfedge` should use backend-supported traversal through `Polyhedron` methods, not scan internal records from outside the class.

`src/mesh/core/mesh_validation.hpp` and new `.cpp`

- Keep the public validation report API.
- Move validation implementation to `.cpp` so it can inspect private backend/mapping detail without leaking detail headers through public headers.

### Mesh Core Private Files

Add `src/mesh/core/detail/openmesh_traits.hpp`:

- Defines `QuaderOpenMeshTraits`.
- Uses OpenMesh point type with float precision for now.
- Requests only the traits needed by Quader and OpenMesh deletion/validation.
- Keep it private. Only mesh-core `.cpp` or detail headers include it.

Add `src/mesh/core/detail/openmesh_id_map.hpp`:

Responsibilities:

- Generic ID slot lifecycle for one element domain.
- Store generation, alive state, and backend handle/reference.
- Convert valid Quader ID to backend handle.
- Convert backend handle to valid Quader ID.
- Allocate a new Quader ID for a new backend handle.
- Mark a Quader ID deleted, bump generation, and release the slot for reuse.
- Rebuild reverse mappings after OpenMesh garbage collection.

Suggested shape:

```cpp
namespace quader::mesh::detail {

template <class PublicId, class OpenMeshHandle>
class OpenMeshIdMap final {
public:
    [[nodiscard]] PublicId create(OpenMeshHandle handle);
    [[nodiscard]] bool is_valid(PublicId id) const noexcept;
    [[nodiscard]] OpenMeshHandle handle(PublicId id) const;
    [[nodiscard]] PublicId id_for_handle(OpenMeshHandle handle) const noexcept;
    [[nodiscard]] void erase(PublicId id);
    [[nodiscard]] std::vector<PublicId> live_ids() const;
    [[nodiscard]] std::size_t live_count() const noexcept;
    void rebuild_reverse_from_live_slots();
};

}
```

Do not instantiate it in public headers if that requires OpenMesh includes to leak. Keep instantiations in `.cpp` or detail-only headers.

Add `src/mesh/core/detail/openmesh_storage.hpp/.cpp`:

Responsibilities:

- Own `using BackendMesh = OpenMesh::PolyMesh_ArrayKernelT<QuaderOpenMeshTraits>;`
- Own the backend mesh.
- Request required OpenMesh status attributes in constructor:
  - vertex status
  - halfedge status if OpenMesh exposes it for the selected type
  - edge status
  - face status
- Own four mapping tables.
- Own all conversion helpers.
- Implement low-level operations used by `Polyhedron`.
- Implement private validation helpers.

Suggested shape:

```cpp
namespace quader::mesh::detail {

class OpenMeshStorage final {
public:
    OpenMeshStorage();
    OpenMeshStorage(const OpenMeshStorage&);
    OpenMeshStorage& operator=(const OpenMeshStorage&);
    OpenMeshStorage(OpenMeshStorage&&) noexcept;
    OpenMeshStorage& operator=(OpenMeshStorage&&) noexcept;
    ~OpenMeshStorage();

    [[nodiscard]] VertexId create_vertex(quader::math::Vec3 position);
    [[nodiscard]] Result<void, MeshError> delete_vertex(VertexId vertex);
    [[nodiscard]] Result<FaceId, MeshError> create_face(std::span<const VertexId> vertices);
    [[nodiscard]] Result<void, MeshError> delete_face(FaceId face);
    [[nodiscard]] Result<void, MeshError> set_vertex_position(VertexId vertex, quader::math::Vec3 position);

    [[nodiscard]] bool is_valid(VertexId id) const noexcept;
    [[nodiscard]] bool is_valid(HalfedgeId id) const noexcept;
    [[nodiscard]] bool is_valid(EdgeId id) const noexcept;
    [[nodiscard]] bool is_valid(FaceId id) const noexcept;

    // Query helpers return Quader IDs and values only.
};

}
```

### Document, Commands, I/O

Expected code changes should be minimal if `Polyhedron` remains the public value type:

- `src/document/scene_object.hpp`: no OpenMesh include; still includes `mesh/core/polyhedron.hpp`.
- `src/document/document.cpp`: keep validation gate through `quader::mesh::validate_mesh`.
- `src/document/selection.hpp`: no change to public component references.
- `src/commands/document_commands.*`: continue moving `Polyhedron` by value.
- `src/io/imported_document.hpp`: continue using `Polyhedron` by value; no OpenMesh includes.
- `src/io/document_fragment_validator.cpp`: keep validation call.
- I/O tests that currently corrupt `vertex_records()` must migrate to new test helpers.

## ID Mapping Policy

### Creation

Vertex creation:

1. Validate finite position.
2. Convert `quader::math::Vec3` to backend point.
3. Call OpenMesh `add_vertex`.
4. Allocate a Quader `VertexId` slot for the returned handle.
5. Resize/reset vertex attribute domain slot for the Quader ID index.
6. Return `VertexId`.

Face creation:

1. Validate at least three vertices.
2. Validate all `VertexId`s are live.
3. Validate positions are finite.
4. Validate non-degenerate polygon area using existing Quader geometry helper.
5. Convert vertices to backend vertex handles.
6. Call OpenMesh `add_face`.
7. If OpenMesh returns invalid face handle, return `MeshErrorCode::BackendRejectedFace` or `NonManifoldTopology`.
8. Allocate or retrieve a Quader `FaceId` for the face handle.
9. Traverse the face halfedges through OpenMesh and allocate or retrieve `HalfedgeId` and `EdgeId` mappings for every backend halfedge and edge touched by the new face.
10. Ensure boundary/opposite halfedges also have `HalfedgeId`s.
11. Resize/reset attribute domains for every newly allocated Quader halfedge, edge, and face slot.
12. Run cheap local validation in debug/test builds if available.

Do not assume a face creates only new edges. If OpenMesh reuses an edge/halfedge, preserve the existing Quader ID mapping.

### Deletion

Public deletion semantics:

- Deleting an invalid ID returns a structured error.
- Deleting a referenced vertex remains rejected unless and until a higher-level topology operation explicitly supports removing incident topology.
- Deleting a face invalidates that `FaceId` and any backend-only halfedges/edges that OpenMesh deletes as a result.
- Stale IDs must be rejected immediately after deletion.
- Attribute slots for deleted IDs may remain allocated, but must reset to defaults when the Quader slot is reused.

Backend deletion semantics:

- Use OpenMesh status/deletion APIs; OpenMesh normally marks deleted elements before garbage collection.
- Quader mapping marks affected IDs dead immediately and increments their generations immediately.
- Quader free lists can reuse dead Quader slots for later new elements, independent of OpenMesh garbage collection.
- Reverse backend-handle mappings for deleted backend handles must be cleared immediately.

Generation policy:

- Generation starts at `1`.
- Invalid IDs use invalid index and generation `0` as current `GenerationalId` already does.
- On deletion, increment generation and skip `0` on wrap.
- Reuse the same Quader slot index only with the incremented generation.
- Never expose OpenMesh handle index/generation as Quader generation.

### Garbage Collection And Compaction

Default runtime policy:

- Do not call OpenMesh garbage collection automatically inside ordinary `create_*` or `delete_*` APIs.
- Deletion marks elements in OpenMesh and invalidates Quader IDs immediately.
- Compaction is explicit through `Polyhedron::compact_storage()` or an internal equivalent called only at controlled boundaries.

Required compaction behavior:

- Preserve every live Quader ID and generation.
- Update every live mapping slot to the new OpenMesh handle after compaction.
- Rebuild every reverse lookup table.
- Keep attributes indexed by Quader ID slot; compaction must not reorder attribute values by backend handle.
- Return `MeshCompactionReport` with removed element counts and live counts.
- Run validation after compaction.

Implementation detail:

- Use OpenMesh garbage collection with handle tracking for vertex, halfedge, and face handles.
- For edges, do not rely solely on `OpenMesh::EdgeHandle` surviving garbage collection. Track each live edge's representative halfedge before compaction, update halfedge handles through OpenMesh, then refresh each edge's backend handle from the updated representative halfedge.
- If OpenMesh 11.0.0 cannot update enough handles to preserve all four Quader ID maps, stop and return a Workaround/Deviation Report. Do not implement compaction by invalidating live Quader IDs.

## Validation Policy

Validation must cover two layers:

1. OpenMesh connectivity/topology.
2. Quader ID mapping and lifecycle.

### OpenMesh Connectivity Checks

At minimum, `validate_mesh` must verify:

- all live backend vertices have finite positions
- all live backend faces have at least three vertices
- all live backend faces have non-degenerate area according to existing Quader geometry rules
- face halfedge cycles close within a bounded number of steps
- halfedge opposite links are reciprocal
- next/previous links are reciprocal for non-boundary face cycles where OpenMesh exposes previous halfedges
- halfedge origin and target vertices resolve to live Quader `VertexId`s
- each live edge resolves to two opposite halfedges
- boundary halfedges are represented consistently through the Quader query API

If OpenMesh already guarantees some connectivity invariants internally, Quader validation should still check the invariants that feed Quader public behavior and diagnostics.

### Quader ID Mapping Checks

At minimum, `validate_mesh` must verify:

- every live Quader vertex slot maps to a valid, live OpenMesh vertex handle
- every live Quader halfedge slot maps to a valid, live OpenMesh halfedge handle
- every live Quader edge slot maps to a valid, live OpenMesh edge handle or a valid representative halfedge from which the edge handle can be derived
- every live Quader face slot maps to a valid, live OpenMesh face handle
- reverse mapping from backend handle index returns the same live Quader ID
- no deleted Quader slot appears in reverse mappings
- no live backend element lacks a Quader ID mapping
- public `*_count()` values match mapping live counts and backend live counts, excluding backend-deleted elements awaiting compaction
- stale IDs are rejected after deletion and after compaction
- attribute domain slot counts can address every live Quader ID slot

Validation diagnostics should mention Quader terms first. Example: "mesh edge id map is missing backend handle for live EdgeId index 12", not only "OpenMesh EdgeHandle invalid".

## Migration Slices

Each implementation slice below is a coherent checkpoint. For C++/CMake/runtime work, the executor must run `cmake --build --preset qt-mingw-debug` after each coherent slice and before starting another independent code task. Before review, run the full verification commands listed later.

Executor must set a Codex `/goal` before editing, naming task #17 and stating completion requires implementation, verification, documentation where required, and PM/authority handoff.

Current continuation state after A19:

- Slices 0, 1, and 2 are completed preparatory work only.
- They do not satisfy the corrected objective and must not be used as authority to move #17 to review.
- The next implementation assignment starts at Slice 3 and must perform the actual storage swap to private OpenMesh topology.

### Slice 0: Start Gate And Conflict Revalidation

Status after A19: completed as preparation by A13/A18/PM revalidation, then refreshed by this plan amendment. Future executors must still perform the normal board freshness/start gate before editing.

No code changes yet.

Actions:

- Read this plan, task #17, `agents/tests-policy.md`, `agents/workflow.md`, `agents/task_board.md`, and the current board metadata.
- Confirm PM has cleared #17 implementation to start.
- Confirm #14 GoogleTest work is either complete/integrated or PM explicitly authorizes #17 to modify CMake/tests concurrently.
- Confirm #16 glm work is not being implemented in overlapping files at the same time. If #16 is active, coordinate order through PM.
- Confirm `CMakeLists.txt` current test harness state because #14 appears to have changed current files while still marked active.

Stop point:

- If #14 or #16 is still actively modifying `CMakeLists.txt`, `src/math/*`, `src/geometry/*`, or shared tests, stop and ask PM for sequencing before editing.

### Slice 1: OpenMesh Dependency And Compiled Mesh Core Target

Status after A19: completed as preparation by A14 and approved by A15. Do not redo this slice unless the storage-swap implementation discovers a real build integration gap.

Files:

- `CMakeLists.txt`
- possibly `dev-changelog.md` if dependency/build notes are durable
- possibly `AGENTS.md` or README only if commands or prerequisites change; ordinary FetchContent dependency does not require command changes.

Actions:

- Already completed: pinned OpenMesh FetchContent.
- Already completed: `modeling_mesh_core` is a `STATIC` target.
- Already completed: placeholder `.cpp` files without changing public behavior:
  - `src/mesh/core/polyhedron.cpp`
  - `src/mesh/core/mesh_validation.cpp`
- Already completed: OpenMesh is linked privately to `modeling_mesh_core`.
- Already completed: current consumers link through `modeling_mesh_core`.
- Already completed: architecture check rules for OpenMesh includes:
  - reject `OpenMesh/` includes outside `src/mesh/core/detail/` and the approved mesh core `.cpp` files.
  - public headers outside detail must not include OpenMesh.

Expected tests:

- Existing tests only.
- No test of OpenMesh itself.

Verification:

- `cmake --preset qt-mingw-debug`
- `cmake --build --preset qt-mingw-debug`
- `ctest --preset qt-mingw-debug`
- `cmake --build --preset qt-mingw-debug --target check_architecture`

Stop point:

- If OpenMesh configure requires unpinned git, local installs, broad global CMake options that break Qt/bgfx/GTest, or public include propagation, stop and return a Workaround/Deviation Report.

### Slice 2: Public Mesh API Narrowing

Status after A19: completed as preparation by A16. This slice narrowed the public API but intentionally did not migrate storage. The storage-swap worker must preserve the public query/mutation contracts added here.

Files:

- `src/mesh/core/polyhedron.hpp`
- `src/mesh/core/mesh_traversal.hpp`
- `src/mesh/core/mesh_records.hpp` or new `src/mesh/core/mesh_element_views.hpp`
- `tests/unit/mesh/mesh_core_tests.cpp`
- `tests/unit/mesh/mesh_fixtures.hpp`
- I/O tests that exercise validation-negative imported mesh cases

Actions:

- Already completed: new query/mutation APIs were added while the old vector-backed storage still existed.
- Already completed: `mesh_traversal.hpp` uses query APIs, not records.
- Already completed: ordinary tests use query APIs:
  - boundary assertions use `is_boundary_halfedge`, `halfedge_face`, `opposite_halfedge`, `next_halfedge`, `previous_halfedge`
  - edge lookup uses `halfedge_edge`
  - position checks use `vertex_position`
- Already completed: ordinary production caller use of mutable record spans/accessors was removed.
- Already completed as a temporary test strategy: validation-negative tests use a targeted test-only corruption/probe mechanism instead of public mutable records.

Preferred test-only mechanism:

- `tests/unit/mesh/mesh_corruption_fixtures.hpp`
- It may include mesh-core private detail only after Slice 3, or for Slice 2 it may build invalid meshes through a controlled friend test hook guarded by a compile definition on `mesh_core_tests` only.
- This helper is justified only for validation internals that cannot be reached through public APIs.
- Do not make the corruption helper available to document, command, UI, renderer, or production code.

Stop point:

- At the end of Slice 2, no production code outside `Polyhedron`/validation may call `vertex_records`, `halfedge_records`, `edge_records`, `face_records`, or mutable `*_record()` accessors.
- If a production caller truly needs direct storage mutation, stop and ask for plan revision.

Verification:

- `cmake --build --preset qt-mingw-debug`
- `ctest --preset qt-mingw-debug`
- `cmake --build --preset qt-mingw-debug --target check_architecture`

### Slice 3: Private OpenMesh Storage And Mapping Tables

Status after A19: next authorized implementation slice. This is the corrected-objective storage swap, not more API preparation.

Files:

- `src/mesh/core/detail/openmesh_traits.hpp`
- `src/mesh/core/detail/openmesh_id_map.hpp`
- `src/mesh/core/detail/openmesh_storage.hpp`
- `src/mesh/core/detail/openmesh_storage.cpp`
- `src/mesh/core/polyhedron.hpp`
- `src/mesh/core/polyhedron.cpp`
- `src/mesh/core/mesh_error.hpp`

Actions:

- Add private OpenMesh storage and make it the only authoritative topology storage.
- Remove `Polyhedron`'s direct authoritative `std::vector<VertexRecord>`, `std::vector<HalfedgeRecord>`, `std::vector<EdgeRecord>`, and `std::vector<FaceRecord>` topology from the public header.
- Add mapping tables for vertex, halfedge, edge, and face.
- Each mapping table must own stable Quader slots, alive/deleted state, generation, free-list reuse, current OpenMesh handle, and reverse backend-handle lookup.
- Implement conversion helpers.
- Implement `Polyhedron` constructors/destructor/copy/move.
- Implement:
  - `create_vertex`
  - `delete_vertex`
  - `create_face`
  - `delete_face`
  - all `is_valid`
  - all counts
  - ID enumeration
  - public traversal query methods
  - vertex position getter/setter
- Keep `MeshAttributes` indexed by Quader ID slot.
- Ensure creating/deleting/reusing vertices resets attribute values exactly as current behavior does.
- Deletion must mark Quader IDs stale immediately, bump generation, clear deleted backend reverse mappings, and release Quader slots for later reuse independent of backend garbage collection.
- Do not expose OpenMesh headers from public headers.

Edge cases to handle:

- invalid ID input
- non-finite vertex positions
- face with fewer than three vertices
- face containing invalid/stale vertex IDs
- duplicate vertices in one face, if OpenMesh rejects or geometry validation rejects it
- degenerate face area
- OpenMesh face rejection due non-manifold or duplicate face
- boundary halfedge queries
- deletion of referenced vertex
- stale IDs after delete/reuse
- copy/move of meshes with mappings and attributes

Required tests in this slice:

- stale IDs reject after deletion
- deleting then reusing a vertex slot changes generation
- face creation rejects invalid, duplicate, non-finite, or degenerate vertices without partial topology mutation
- copy/move preserves valid IDs, counts, attributes, topology, and validation
- OpenMesh and private detail remain absent from public headers and from document/command/I/O/UI/Crimson contracts

Verification:

- `cmake --build --preset qt-mingw-debug`
- `ctest --preset qt-mingw-debug`
- `cmake --build --preset qt-mingw-debug --target check_architecture`

Stop point:

- If current behavior cannot be preserved without keeping old record vectors as mirrored truth, stop and return a Workaround/Deviation Report.

### Slice 4: Validation Rewrite

Files:

- `src/mesh/core/mesh_validation.hpp`
- `src/mesh/core/mesh_validation.cpp`
- `tests/unit/mesh/mesh_core_tests.cpp`
- `tests/unit/mesh/mesh_corruption_fixtures.hpp` if used
- `tests/unit/io/io_document_fragment_tests.cpp`
- `tests/unit/io/io_import_export_service_tests.cpp`

Actions:

- Keep validation implementation in `.cpp`.
- Validate OpenMesh connectivity and Quader mapping consistency.
- Preserve current public report shape where practical.
- Add specific mapping validation issue codes:
  - `MissingBackendHandle`
  - `MissingQuaderId`
  - `ReverseMappingMismatch`
  - `DeletedElementMapped`
  - `AttributeDomainMismatch`
- Update document and I/O validation tests to assert semantic rejection of invalid imported meshes.
- For tests that need impossible corrupt internal state, use the test-only corruption helper and state in test names that they validate mapping/connectivity guardrails.

Required tests:

- valid single triangle validates
- boundary halfedge representation validates
- stale deleted IDs reject through public API
- missing or broken reverse mapping reports a mapping issue
- backend connectivity corruption reports a connectivity issue
- non-finite vertex position reports validation issue
- degenerate face creation is rejected before backend mutation
- import fragment with invalid mesh reports validation errors without returning a partial document

Verification:

- `cmake --build --preset qt-mingw-debug`
- `ctest --preset qt-mingw-debug`
- `cmake --build --preset qt-mingw-debug --target check_architecture`

### Slice 5: Explicit Compaction

Files:

- `src/mesh/core/polyhedron.hpp`
- `src/mesh/core/polyhedron.cpp`
- `src/mesh/core/detail/openmesh_storage.*`
- `tests/unit/mesh/mesh_core_tests.cpp`

Actions:

- Add `MeshCompactionReport`.
- Implement explicit `compact_storage()`.
- Use OpenMesh handle-tracking garbage collection.
- Preserve all live Quader IDs and generations.
- Preserve all live attributes by Quader ID slot across compaction.
- Refresh edge mappings through representative halfedges.
- Rebuild reverse maps from live slots.
- Run validation after compaction and fail if mapping is inconsistent.

Required tests:

- delete a face, compact, and assert:
  - deleted IDs remain invalid
  - live vertex IDs remain valid with same generation
  - counts are correct
  - validation passes
- create additional geometry after compaction and assert new IDs do not collide with live IDs
- attributes remain associated with Quader IDs across compaction
- stale IDs from before compaction remain invalid

Stop point:

- If OpenMesh edge or halfedge remapping cannot be made reliable, do not ship a fake compaction method. Stop for architect review. It is acceptable to keep compaction uncalled until fixed only if PM/architect explicitly revises #17 scope; it is not acceptable to silently drop the policy.

Verification:

- `cmake --build --preset qt-mingw-debug`
- `ctest --preset qt-mingw-debug`
- `cmake --build --preset qt-mingw-debug --target check_architecture`

### Slice 6: Caller Cleanup And Architecture Lock

Files:

- `src/document/*` only if API changes require minor updates
- `src/commands/*` only if API changes require minor updates
- `src/io/*` and I/O tests if record access was present
- `tools/check_architecture.py`
- `tests/unit/document/*`
- `tests/unit/commands/*`
- `tests/unit/mesh/*`

Actions:

- Remove public mutable record APIs completely.
- Remove or neutralize old `mesh_records.hpp` mutable storage contract.
- Ensure no production caller includes mesh private detail.
- Ensure no production caller includes OpenMesh.
- Ensure no tests outside mesh-core private tests include OpenMesh or private detail.
- Ensure document/command semantic tests pass unchanged or with only helper updates.
- Add architecture check coverage for OpenMesh include boundaries.

Required grep checks:

```powershell
rg -n "OpenMesh|OpenMeshCore|OpenMeshTools|PolyMesh|TriMesh" src tests CMakeLists.txt
rg -n "vertex_records\\(|halfedge_records\\(|edge_records\\(|face_records\\(|vertex_record\\(|halfedge_record\\(|edge_record\\(|face_record\\(" src tests
```

Expected result:

- `OpenMesh` appears in `CMakeLists.txt`, `src/mesh/core/detail/*`, mesh-core `.cpp`, and mesh-core private tests only.
- mutable record APIs do not appear in production code.
- remaining test-only references are intentionally limited and named as validation corruption fixtures.

Verification:

- `cmake --build --preset qt-mingw-debug`
- `ctest --preset qt-mingw-debug`
- `cmake --build --preset qt-mingw-debug --target check_architecture`

### Slice 7: Documentation And Final Verification

Files:

- `dev-changelog.md`
- `AGENTS.md` or README only if build/test commands or developer prerequisites changed.

Actions:

- Add a `dev-changelog.md` entry for:
  - pinned OpenMesh dependency
  - mesh core backend replacement
  - public record access removal
  - validation/mapping checks
- Do not update user-facing `changelog.md` unless the user or PM explicitly includes release work. This is internal architecture work.
- Do not bump `VERSION` or `DEV_VERSION`.
- Do not create a dev-build archive unless separately requested.

Final verification before architect review:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
python tools/project_board.py validate
```

Run deploy only if user-visible runtime behavior changed:

```powershell
cmake --build --preset qt-mingw-debug --target deploy
```

## Test Strategy

Tests must protect behavior and invariants, not OpenMesh implementation details.

### Mesh Unit Tests

Update `tests/unit/mesh/mesh_core_tests.cpp` and fixtures.

Required behavior tests:

- IDs default invalid.
- invalid IDs are rejected.
- single triangle has:
  - 3 vertices
  - 3 edges
  - 6 halfedges if Quader still exposes boundary halfedge IDs
  - 1 face
  - valid boundary halfedge queries
  - valid face vertex order
- delete face invalidates face, edge, and halfedge IDs affected by deletion.
- deleting a referenced vertex is rejected.
- deleting then reusing a vertex slot changes generation.
- stale IDs fail after deletion and after compaction.
- attribute default values initialize and reset on reused Quader slots.
- vertex position set rejects non-finite values and does not mutate.
- face creation rejects degenerate polygons and invalid vertices without partial topology.
- compaction preserves live Quader IDs and validation.

Mapping-specific tests:

- all live backend elements have Quader IDs after face creation.
- reverse mappings are consistent.
- no deleted backend element maps to a live Quader ID.
- copy/move preserves valid IDs, counts, attributes, and validation.

Connectivity validation tests:

- validation accepts valid topology.
- validation reports broken OpenMesh connectivity using a private corruption fixture.
- validation reports broken Quader mapping using a private corruption fixture.

### Document Tests

Update `tests/unit/document/document_tests.cpp` and `document_test_helpers.hpp` only as needed.

Required behavior remains:

- document accepts a valid triangle mesh object.
- document rejects invalid mesh objects through validation.
- selection validates `VertexId`, `EdgeId`, and `FaceId`.
- delete object prunes selection.
- dirty flags/events still distinguish mesh topology and geometry changes.
- semantic state helpers use public mesh queries/counts only.

### Command Tests

Update `tests/unit/commands/command_tests.cpp` only as needed.

Required behavior remains:

- create mesh object execute/undo/redo preserves semantic document state.
- delete object undo/redo restores object and selection.
- selection commands still use stable Quader IDs.
- rejected commands do not mutate document or command history.

### I/O Validation Tests

Even though #17 is mesh-core owned, current I/O tests participate in validation of imported mesh payloads. Preserve and update:

- `tests/unit/io/io_document_fragment_tests.cpp`
- `tests/unit/io/io_import_export_service_tests.cpp`

Required behavior:

- import validation rejects an invalid mesh payload.
- build document from import rejects invalid import without partial document.
- tests must not use public mutable record spans.
- invalid imported mesh fixtures must use the mesh-core test-only corruption fixture or public invalid-input APIs, not production record access.

Use a mesh test fixture helper that intentionally creates an invalid mesh only for validation tests. Keep it small and named by behavior.

### Architecture Checks

Extend `tools/check_architecture.py`:

- reject `OpenMesh/` includes outside approved mesh-core private paths
- reject `glm/` only if #16 owns that change; do not add glm checks in #17 unless #16 has already landed and PM confirms ownership
- keep existing Qt and graphics-runtime checks unchanged

Add or update tests for architecture check only if the tool has existing test coverage. If not, the required verification is direct `check_architecture` target plus `ctest`, matching current project practice.

### Sanitizers And Fuzzing

No sanitizer or fuzz target is currently configured in this repository. Do not invent a new sanitizer/fuzz infrastructure inside #17 unless PM expands scope.

Manual substitute:

- Run the full unit suite through CTest.
- Add deterministic edge-case unit tests for invalid IDs, stale IDs, degenerate faces, mapping corruption, and compaction.
- Record in final report that sanitizer/fuzz coverage is not configured.

## Sequencing Relative To #14 And #16

### #14 GoogleTest

#17 implementation should start after #14 is integrated or after PM confirms no file conflict.

Reason:

- #17 will touch `CMakeLists.txt`.
- #17 will substantially edit `tests/unit/mesh/*`, `tests/unit/document/*`, `tests/unit/commands/*`, and current I/O tests.
- #14 owns test harness conversion and CTest/GTest registration.

If #14 remains active when #17 is assigned, the #17 executor must stop before editing and ask PM whether to wait, rebase against #14 output, or receive a conflict-free assignment.

### #16 glm

#17 can be implemented before or after #16 if boundaries are preserved.

Rules:

- #17 must continue to use `quader::math::Vec3` in public APIs.
- #17 private OpenMesh conversion helpers must not include or mention glm.
- If #16 lands first, use the resulting Quader `Vec3` API and keep OpenMesh conversion in mesh-core private implementation.
- If #17 lands first, #16 later must treat OpenMesh conversion as a mesh-core private consumer of Quader math, not as a reason to expose glm to mesh public APIs.

If #16 is concurrently editing `src/math/*`, `src/geometry/*`, or mesh math call sites, PM must sequence the work before #17 implementation starts.

## Edge Cases And Required Handling

- Empty mesh validates if no corrupt mapping exists.
- Invalid default IDs are rejected in every public query/mutation.
- Deleted IDs with old generation are rejected.
- Reused Quader slot with new generation is accepted.
- Face creation rejects duplicate or invalid vertices before partial mutation where practical.
- If OpenMesh partially mutates before returning an invalid face, rollback or rebuild to the pre-call state. If rollback cannot be guaranteed, stop for plan revision.
- Non-manifold or duplicate face rejection returns structured `MeshError`.
- Boundary halfedges must be queryable without exposing OpenMesh boundary internals.
- Deleting a face must not leave selected components valid by accident; document selection validation should reject stale component IDs after a mesh topology change if checked.
- Mesh copy preserves public IDs, generations, attributes, topology, and validation.
- Mesh move leaves destination valid and source destructible.
- Compaction preserves live IDs and attributes.
- Attributes remain indexed by Quader slot, not backend handle.
- OpenMesh status attributes must be available before deletion or compaction APIs are used.

## Final Authority Review Handoff

Before requesting software-architect review, the implementation report must include:

- changed files
- confirmation that OpenMesh is pinned to 11.0.0 with URL hash
- confirmation that OpenMesh is linked only behind `modeling_mesh_core`
- confirmation that no OpenMesh type appears in public Quader contracts
- confirmation that public mutable record spans/accessors are removed or replaced
- mapping table design actually implemented
- deletion/generation/compaction behavior summary
- test list and results
- build/check command results
- any known deviations

The implementation must return to the software architect after code work. If approved, the main/root agent should move this plan from `agents/plans` to `agents/archive`; do not delete it automatically.

## PM/Authority Stop Points

Stop and route through PM plus software architect if any of these occurs:

- OpenMesh 11.0.0 cannot be fetched and built from a pinned source archive.
- Implementer wants to use system-installed OpenMesh, an unpinned git branch, or a third-party mirror.
- OpenMesh target names differ and require a broader dependency strategy.
- Public headers need to expose OpenMesh types to compile.
- Stable Quader IDs cannot be preserved across delete/reuse.
- Compaction cannot preserve live Quader IDs.
- Implementation needs to keep old vector records as mirrored truth beyond a short migration adapter.
- Document, commands, I/O, UI, or Crimson need direct OpenMesh handles.
- #14 or #16 active work conflicts with #17 CMake/test/math/geometry edits.
- Required verification cannot be run.
