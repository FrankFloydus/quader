#include "ui/models/property_descriptor.hpp"

#include "document/document.hpp"
#include "document/scene_object.hpp"
#include "document/selection.hpp"

#include <QString>

#include <cstddef>

namespace quader::ui {
namespace {

[[nodiscard]] PropertyDescriptor
make_descriptor(quader::document::ObjectId object, PropertyField field,
		QString label, PropertyValueKind kind, QVariant value,
		bool editable = false, QString tooltip = {}) {
	return PropertyDescriptor{
		PropertyPath{ object, field },
		std::move(label),
		kind,
		std::move(value),
		{},
		{},
		3,
		editable,
		std::move(tooltip),
	};
}

[[nodiscard]] QVariant count_value(std::size_t value) {
	return QVariant::fromValue<qulonglong>(static_cast<qulonglong>(value));
}

void append_transform_descriptors(
		std::vector<PropertyDescriptor> &descriptors,
		quader::document::ObjectId object,
		const quader::document::Transform &transform) {
	descriptors.push_back(
			make_descriptor(object, PropertyField::TranslationX,
					QStringLiteral("Translation X"), PropertyValueKind::Float,
					static_cast<double>(transform.translation.x), true));
	descriptors.push_back(
			make_descriptor(object, PropertyField::TranslationY,
					QStringLiteral("Translation Y"), PropertyValueKind::Float,
					static_cast<double>(transform.translation.y), true));
	descriptors.push_back(
			make_descriptor(object, PropertyField::TranslationZ,
					QStringLiteral("Translation Z"), PropertyValueKind::Float,
					static_cast<double>(transform.translation.z), true));

	descriptors.push_back(
			make_descriptor(object, PropertyField::RotationX,
					QStringLiteral("Rotation X"), PropertyValueKind::Float,
					static_cast<double>(transform.rotation_euler.x), true));
	descriptors.push_back(
			make_descriptor(object, PropertyField::RotationY,
					QStringLiteral("Rotation Y"), PropertyValueKind::Float,
					static_cast<double>(transform.rotation_euler.y), true));
	descriptors.push_back(
			make_descriptor(object, PropertyField::RotationZ,
					QStringLiteral("Rotation Z"), PropertyValueKind::Float,
					static_cast<double>(transform.rotation_euler.z), true));

	descriptors.push_back(make_descriptor(
			object, PropertyField::ScaleX, QStringLiteral("Scale X"),
			PropertyValueKind::Float, static_cast<double>(transform.scale.x), true));
	descriptors.push_back(make_descriptor(
			object, PropertyField::ScaleY, QStringLiteral("Scale Y"),
			PropertyValueKind::Float, static_cast<double>(transform.scale.y), true));
	descriptors.push_back(make_descriptor(
			object, PropertyField::ScaleZ, QStringLiteral("Scale Z"),
			PropertyValueKind::Float, static_cast<double>(transform.scale.z), true));
}

[[nodiscard]] PropertyDescriptor make_selection_summary(const QString &value) {
	return make_descriptor(
			quader::document::ObjectId::invalid(), PropertyField::SelectionSummary,
			QStringLiteral("Selection"), PropertyValueKind::ReadOnlyText,
			value, false);
}

} // namespace

std::vector<PropertyDescriptor>
build_property_descriptors(const quader::document::Document &document) {
	const auto &selection = document.selection();
	if (selection.empty()) {
		return {};
	}

	if (!selection.selected_components().empty()) {
		return { make_selection_summary(
				QStringLiteral("%1 component(s) selected")
						.arg(static_cast<qsizetype>(
								selection.selected_components().size()))) };
	}

	if (selection.selected_objects().size() != 1U) {
		return { make_selection_summary(
				QStringLiteral("%1 object(s) selected")
						.arg(static_cast<qsizetype>(selection.selected_objects().size()))) };
	}

	const auto kObjectId = selection.selected_objects().front();
	const auto *object = document.find_mesh_object(kObjectId);
	if (object == nullptr) {
		return { make_selection_summary(
				QStringLiteral("Selected object is no longer available")) };
	}

	std::vector<PropertyDescriptor> descriptors;
	descriptors.reserve(14U);
	descriptors.push_back(make_descriptor(
			kObjectId, PropertyField::ObjectName, QStringLiteral("Name"),
			PropertyValueKind::String, QString::fromStdString(object->name), true,
			QStringLiteral("Object name")));
	descriptors.push_back(make_descriptor(
			kObjectId, PropertyField::ObjectKind, QStringLiteral("Kind"),
			PropertyValueKind::ReadOnlyText, QStringLiteral("Mesh")));
	descriptors.push_back(make_descriptor(
			kObjectId, PropertyField::VertexCount, QStringLiteral("Vertices"),
			PropertyValueKind::Int, count_value(object->mesh.vertex_count())));
	descriptors.push_back(make_descriptor(
			kObjectId, PropertyField::EdgeCount, QStringLiteral("Edges"),
			PropertyValueKind::Int, count_value(object->mesh.edge_count())));
	descriptors.push_back(make_descriptor(
			kObjectId, PropertyField::FaceCount, QStringLiteral("Faces"),
			PropertyValueKind::Int, count_value(object->mesh.face_count())));

	append_transform_descriptors(descriptors, kObjectId, object->transform);
	return descriptors;
}

} // namespace quader::ui
