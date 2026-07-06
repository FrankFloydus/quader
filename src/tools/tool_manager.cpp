/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "tools/tool_manager.hpp"

#include "commands/selection_commands.hpp"
#include "document/selection.hpp"
#include "mesh/core/polyhedron.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace quader::tools {
namespace {

[[nodiscard]] quader::document::ObjectId object_id_from_encoded_hit(std::uint64_t encoded) noexcept {
	return quader::document::ObjectId{
		static_cast<quader::foundation::IdIndex>(encoded & 0xffffffffULL),
		static_cast<quader::foundation::IdGeneration>((encoded >> 32U) & 0xffffffffULL),
	};
}

[[nodiscard]] quader::document::ObjectId object_id_from_hit(const SurfaceHit &hit) noexcept {
	if (hit.document_object_id.is_valid()) {
		return hit.document_object_id;
	}
	return object_id_from_encoded_hit(hit.object_id);
}

[[nodiscard]] bool tool_uses_selection_fallback(ToolId id) noexcept {
	return id == ToolId::Move || id == ToolId::Rotate || id == ToolId::Scale;
}

[[nodiscard]] std::optional<quader::mesh::FaceId> face_id_from_index(
		const quader::mesh::Polyhedron &mesh,
		std::uint32_t index) {
	const auto kFaces = mesh.face_ids();
	const auto kIt = std::find_if(kFaces.begin(), kFaces.end(), [index](quader::mesh::FaceId face) {
		return face.index() == index;
	});
	if (kIt == kFaces.end()) {
		return std::nullopt;
	}
	return *kIt;
}

[[nodiscard]] std::optional<quader::mesh::EdgeId> edge_id_from_index(
		const quader::mesh::Polyhedron &mesh,
		std::uint32_t index) {
	const auto kEdges = mesh.edge_ids();
	const auto kIt = std::find_if(kEdges.begin(), kEdges.end(), [index](quader::mesh::EdgeId edge) {
		return edge.index() == index;
	});
	if (kIt == kEdges.end()) {
		return std::nullopt;
	}
	return *kIt;
}

[[nodiscard]] std::optional<quader::mesh::VertexId> vertex_id_from_index(
		const quader::mesh::Polyhedron &mesh,
		std::uint32_t index) {
	const auto kVertices = mesh.vertex_ids();
	const auto kIt = std::find_if(kVertices.begin(), kVertices.end(), [index](quader::mesh::VertexId vertex) {
		return vertex.index() == index;
	});
	if (kIt == kVertices.end()) {
		return std::nullopt;
	}
	return *kIt;
}

[[nodiscard]] quader::document::SelectionMode document_selection_mode_for(SelectionMode mode) noexcept {
	switch (mode) {
		case SelectionMode::Object:
			return quader::document::SelectionMode::Object;
		case SelectionMode::Vertex:
			return quader::document::SelectionMode::Vertex;
		case SelectionMode::Edge:
			return quader::document::SelectionMode::Edge;
		case SelectionMode::Face:
			return quader::document::SelectionMode::Face;
	}
	return quader::document::SelectionMode::Object;
}

[[nodiscard]] quader::document::ComponentKind component_kind_for(SelectionMode mode) noexcept {
	switch (mode) {
		case SelectionMode::Vertex:
			return quader::document::ComponentKind::Vertex;
		case SelectionMode::Edge:
			return quader::document::ComponentKind::Edge;
		case SelectionMode::Object:
		case SelectionMode::Face:
			return quader::document::ComponentKind::Face;
	}
	return quader::document::ComponentKind::Face;
}

[[nodiscard]] std::optional<quader::document::ComponentRef> component_ref_from_hit(
		const quader::document::ObjectId object_id,
		const quader::mesh::Polyhedron &mesh,
		const SurfaceHit &hit,
		SelectionMode mode) {
	switch (mode) {
		case SelectionMode::Vertex: {
			if (hit.kind != SurfaceHitKind::Vertex) {
				return std::nullopt;
			}
			if (const auto *vertex = std::get_if<quader::mesh::VertexId>(&hit.component)) {
				return quader::document::ComponentRef{ object_id, *vertex };
			}
			const auto kVertex = vertex_id_from_index(mesh, hit.component_index);
			if (!kVertex.has_value()) {
				return std::nullopt;
			}
			return quader::document::ComponentRef{ object_id, *kVertex };
		}
		case SelectionMode::Edge: {
			if (hit.kind != SurfaceHitKind::Edge) {
				return std::nullopt;
			}
			if (const auto *edge = std::get_if<quader::mesh::EdgeId>(&hit.component)) {
				return quader::document::ComponentRef{ object_id, *edge };
			}
			const auto kEdge = edge_id_from_index(mesh, hit.component_index);
			if (!kEdge.has_value()) {
				return std::nullopt;
			}
			return quader::document::ComponentRef{ object_id, *kEdge };
		}
		case SelectionMode::Face: {
			if (hit.kind != SurfaceHitKind::Face) {
				return std::nullopt;
			}
			if (const auto *face = std::get_if<quader::mesh::FaceId>(&hit.component)) {
				return quader::document::ComponentRef{ object_id, *face };
			}
			const auto kFace = face_id_from_index(mesh, hit.component_index);
			if (!kFace.has_value()) {
				return std::nullopt;
			}
			return quader::document::ComponentRef{ object_id, *kFace };
		}
		case SelectionMode::Object:
			return std::nullopt;
	}
	return std::nullopt;
}

[[nodiscard]] bool remove_object(std::vector<quader::document::ObjectId> &objects,
		quader::document::ObjectId object) {
	const auto kOldSize = objects.size();
	objects.erase(std::remove(objects.begin(), objects.end(), object), objects.end());
	return objects.size() != kOldSize;
}

[[nodiscard]] bool remove_component(std::vector<quader::document::ComponentRef> &components,
		const quader::document::ComponentRef &component) {
	const auto kOldSize = components.size();
	components.erase(std::remove(components.begin(), components.end(), component), components.end());
	return components.size() != kOldSize;
}

[[nodiscard]] bool same_hover_hit(const SurfaceHit &left, const SurfaceHit &right) noexcept {
	return left.kind == right.kind &&
			left.object_id == right.object_id &&
			left.component_index == right.component_index &&
			left.document_object_id == right.document_object_id &&
			left.component == right.component;
}

} // namespace

ToolManager::ToolManager(ToolContext context) : context_(std::move(context)) {
}

bool ToolManager::register_tool(std::unique_ptr<ITool> tool) {
	if (!tool) {
		return false;
	}

	const auto kId = tool->id();
	if (tools_.contains(kId)) {
		return false;
	}

	tools_.emplace(kId, std::move(tool));
	return true;
}

bool ToolManager::has_tool(ToolId id) const noexcept {
	return tools_.contains(id);
}

std::optional<ToolId> ToolManager::active_tool_id() const noexcept {
	if (active_tool_ == nullptr) {
		return std::nullopt;
	}

	return active_tool_->id();
}

ITool *ToolManager::active_tool() noexcept {
	return active_tool_;
}

const ITool *ToolManager::active_tool() const noexcept {
	return active_tool_;
}

SelectionMode ToolManager::selection_mode() const noexcept {
	return selection_mode_;
}

std::optional<SurfaceHit> ToolManager::selection_hover() const {
	return selection_hover_;
}

bool ToolManager::selection_hover_suppresses_selected() const noexcept {
	return selection_hover_suppresses_selected_;
}

bool ToolManager::clear_selection_hover() {
	return clear_selection_hover_state();
}

bool ToolManager::set_selection_mode(SelectionMode mode) noexcept {
	if (selection_mode_ == mode) {
		return false;
	}

	selection_mode_ = mode;
	(void)clear_selection_hover_state();
	return true;
}

bool ToolManager::set_active_tool(ToolId id) {
	const auto kIterator = tools_.find(id);
	if (kIterator == tools_.end()) {
		return false;
	}

	auto *next_tool = kIterator->second.get();
	if (active_tool_ == next_tool) {
		return true;
	}

	(void)clear_selection_hover_state();
	if (active_tool_ != nullptr) {
		active_tool_->deactivate(context_);
	}

	active_tool_ = next_tool;
	active_tool_->activate(context_);
	notify_active_tool_changed();
	return true;
}

void ToolManager::clear_active_tool() {
	if (active_tool_ == nullptr) {
		return;
	}

	(void)clear_selection_hover_state();
	active_tool_->deactivate(context_);
	active_tool_ = nullptr;
	notify_active_tool_changed();
}

bool ToolManager::cancel_active_tool() {
	if (active_tool_ == nullptr) {
		return false;
	}

	active_tool_->cancel(context_);
	return true;
}

void ToolManager::set_after_active_tool_changed(std::function<void()> callback) {
	after_active_tool_changed_ = std::move(callback);
}

bool ToolManager::dispatch_pointer_event(const PointerEvent &event) {
	if (active_tool_ == nullptr) {
		return false;
	}

	const ToolId kHandledTool = active_tool_->id();
	if (active_tool_->on_pointer_event(event, context_)) {
		apply_tool_completion_request(kHandledTool);
		return true;
	}

	if (active_tool_->id() != ToolId::Select && !tool_uses_selection_fallback(active_tool_->id())) {
		return false;
	}

	return handle_select_pointer_event(event);
}

bool ToolManager::dispatch_key_event(const KeyEvent &event) {
	if (active_tool_ == nullptr) {
		return false;
	}

	const ToolId kHandledTool = active_tool_->id();
	if (active_tool_->on_key_event(event, context_)) {
		apply_tool_completion_request(kHandledTool);
		return true;
	}

	return false;
}

ToolPreview ToolManager::preview() const {
	if (active_tool_ == nullptr) {
		return {};
	}

	return active_tool_->preview();
}

ToolContext &ToolManager::context() noexcept {
	return context_;
}

const ToolContext &ToolManager::context() const noexcept {
	return context_;
}

bool ToolManager::handle_select_pointer_event(const PointerEvent &event) {
	if (event.navigation_active) {
		return clear_selection_hover();
	}

	if (event.phase == PointerPhase::Hover) {
		if (!event.surface_hit) {
			return clear_selection_hover();
		}

		const quader::document::ObjectId kHoverObject = object_id_from_hit(*event.surface_hit);
		if (!context_.document().validate_object_id(kHoverObject)) {
			return clear_selection_hover();
		}

		return set_selection_hover(*event.surface_hit,
				selected_modifier_hover_for_hit(*event.surface_hit, event.modifiers));
	}

	if (event.phase == PointerPhase::Press && event.button == PointerButton::Left && event.pressed && !event.surface_hit) {
		const bool kHoverCleared = clear_selection_hover_state();
		if (event.modifiers.shift || event.modifiers.control || context_.document().selection().empty()) {
			return kHoverCleared;
		}

		quader::document::Selection selection;
		if (selection_mode_ == SelectionMode::Object) {
			auto selected = selection.set_objects({});
			if (!selected) {
				return false;
			}
		} else {
			auto selected = selection.set_component_selection(document_selection_mode_for(selection_mode_), {}, {});
			if (!selected) {
				return false;
			}
		}

		const auto kResult = context_.execute_command(
				std::make_unique<quader::commands::SetSelectionCommand>(std::move(selection)));
		return kResult.is_applied();
	}

	if (event.phase != PointerPhase::Press || event.button != PointerButton::Left || !event.pressed || !event.surface_hit) {
		return false;
	}

	const quader::document::ObjectId kObject = object_id_from_hit(*event.surface_hit);
	if (!context_.document().validate_object_id(kObject)) {
		return false;
	}

	const bool kAdditive = event.modifiers.shift || event.modifiers.control;
	quader::document::Selection selection;

	if (selection_mode_ != SelectionMode::Object) {
		const auto *object = context_.document().find_mesh_object(kObject);
		if (object == nullptr) {
			return false;
		}

		const std::optional<quader::document::ComponentRef> kComponent =
				component_ref_from_hit(kObject, object->mesh, *event.surface_hit, selection_mode_);

		const quader::document::SelectionMode kDocumentMode = document_selection_mode_for(selection_mode_);
		const quader::document::ComponentKind kKind = component_kind_for(selection_mode_);
		std::vector<quader::document::ObjectId> targets;
		std::vector<quader::document::ComponentRef> components;
		if (kAdditive && context_.document().selection().mode() == kDocumentMode) {
			const auto kSelectedObjects = context_.document().selection().selected_objects();
			targets.assign(kSelectedObjects.begin(), kSelectedObjects.end());
			for (const auto &component : context_.document().selection().selected_components()) {
				if (quader::document::component_kind(component) == kKind) {
					components.push_back(component);
				}
			}
		}

		if (std::find(targets.begin(), targets.end(), kObject) == targets.end()) {
			targets.push_back(kObject);
		}

		if (kComponent.has_value()) {
			if (!kAdditive || !remove_component(components, *kComponent)) {
				components.push_back(*kComponent);
			}
		}

		auto selected = selection.set_component_selection(kDocumentMode, std::move(targets), std::move(components));
		if (!selected) {
			return false;
		}
	} else {
		std::vector<quader::document::ObjectId> objects;
		if (kAdditive) {
			const auto kSelectedObjects = context_.document().selection().selected_objects();
			objects.assign(kSelectedObjects.begin(), kSelectedObjects.end());
		}

		if (!kAdditive || !remove_object(objects, kObject)) {
			objects.push_back(kObject);
		}

		auto selected = selection.set_objects(std::move(objects));
		if (!selected) {
			return false;
		}
	}

	const bool kHoverInvalidated = suppress_selection_hover_after_click(*event.surface_hit);
	const auto kResult = context_.execute_command(
			std::make_unique<quader::commands::SetSelectionCommand>(std::move(selection)));
	return kResult.is_applied() || kHoverInvalidated;
}

bool ToolManager::clear_selection_hover_state() noexcept {
	const bool had_state = selection_hover_.has_value() ||
			selection_hover_suppresses_selected_ ||
			suppressed_selection_hover_.has_value();
	selection_hover_ = std::nullopt;
	selection_hover_suppresses_selected_ = false;
	suppressed_selection_hover_ = std::nullopt;
	return had_state;
}

bool ToolManager::clear_visible_selection_hover() noexcept {
	const bool had_state = selection_hover_.has_value() || selection_hover_suppresses_selected_;
	selection_hover_ = std::nullopt;
	selection_hover_suppresses_selected_ = false;
	return had_state;
}

bool ToolManager::suppress_selection_hover_after_click(const SurfaceHit &hit) {
	const bool had_state = selection_hover_.has_value() ||
			selection_hover_suppresses_selected_ ||
			!suppressed_selection_hover_.has_value() ||
			!same_hover_hit(*suppressed_selection_hover_, hit);
	selection_hover_ = std::nullopt;
	selection_hover_suppresses_selected_ = false;
	suppressed_selection_hover_ = hit;
	return had_state;
}

bool ToolManager::selected_modifier_hover_for_hit(
		const SurfaceHit &hit,
		const KeyboardModifiers &modifiers) const {
	if (!modifiers.shift && !modifiers.control) {
		return false;
	}

	const quader::document::ObjectId kObject = object_id_from_hit(hit);
	if (!context_.document().validate_object_id(kObject)) {
		return false;
	}

	const quader::document::Selection &selection = context_.document().selection();
	if (selection_mode_ == SelectionMode::Object) {
		return selection.mode() == quader::document::SelectionMode::Object &&
				std::find(selection.selected_objects().begin(),
						selection.selected_objects().end(),
						kObject) != selection.selected_objects().end();
	}

	const quader::document::SelectionMode kDocumentMode = document_selection_mode_for(selection_mode_);
	if (selection.mode() != kDocumentMode) {
		return false;
	}

	const auto *object = context_.document().find_mesh_object(kObject);
	if (object == nullptr) {
		return false;
	}

	const std::optional<quader::document::ComponentRef> kComponent =
			component_ref_from_hit(kObject, object->mesh, hit, selection_mode_);
	if (!kComponent.has_value()) {
		return false;
	}

	return std::find(selection.selected_components().begin(),
				   selection.selected_components().end(),
				   *kComponent) != selection.selected_components().end();
}

bool ToolManager::set_selection_hover(SurfaceHit hit, bool suppresses_selected) {
	if (suppressed_selection_hover_.has_value()) {
		if (same_hover_hit(*suppressed_selection_hover_, hit)) {
			if (!suppresses_selected) {
				return clear_visible_selection_hover();
			}
			suppressed_selection_hover_ = std::nullopt;
		} else {
			suppressed_selection_hover_ = std::nullopt;
		}
	}

	if (selection_hover_.has_value() &&
			same_hover_hit(*selection_hover_, hit) &&
			selection_hover_suppresses_selected_ == suppresses_selected) {
		return false;
	}

	selection_hover_ = hit;
	selection_hover_suppresses_selected_ = suppresses_selected;
	return true;
}

void ToolManager::apply_tool_completion_request(ToolId handled_tool) {
	if (active_tool_ == nullptr || active_tool_->id() != handled_tool) {
		return;
	}

	const ToolCompletionRequest kRequest = active_tool_->consume_completion_request();
	if (kRequest == ToolCompletionRequest::ReturnToSelect && handled_tool != ToolId::Select) {
		(void)set_selection_mode(SelectionMode::Object);
		if (has_tool(ToolId::Select)) {
			(void)set_active_tool(ToolId::Select);
		}
	}
}

void ToolManager::notify_active_tool_changed() {
	if (after_active_tool_changed_) {
		after_active_tool_changed_();
	}
}

} // namespace quader::tools
