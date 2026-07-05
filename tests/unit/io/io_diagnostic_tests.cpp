#include "foundation/logging.hpp"
#include "io/io_diagnostic.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

TEST(IoDiagnostic, IoDiagnosticsPreserveSourceLocationPathAndSubject) {
	quader::io::IoDiagnosticList diagnostics;
	diagnostics.add(quader::io::IoDiagnostic{
			quader::io::IoSeverity::Warning,
			quader::io::IoDiagnosticCode::UnsupportedFeaturePreserved,
			"preserved unsupported extension",
			std::filesystem::path{ "scene.gltf" },
			quader::io::SourceLocation{ 12U, 7U, "/materials/0/extensions" },
			"MaterialA",
	});
	diagnostics.add_error(quader::io::IoDiagnosticCode::ParseFailed, "invalid token");

	EXPECT_FALSE(diagnostics.empty());
	EXPECT_TRUE(diagnostics.has_errors());
	EXPECT_EQ(diagnostics.size(), 2U);
	EXPECT_EQ(diagnostics[0].severity, quader::io::IoSeverity::Warning);
	EXPECT_EQ(diagnostics[0].code, quader::io::IoDiagnosticCode::UnsupportedFeaturePreserved);
	EXPECT_EQ(diagnostics[0].source_path, std::filesystem::path{ "scene.gltf" });
	EXPECT_EQ(diagnostics[0].location.line, 12U);
	EXPECT_EQ(diagnostics[0].location.column, 7U);
	EXPECT_EQ(diagnostics[0].location.json_pointer, std::string("/materials/0/extensions"));
	EXPECT_EQ(diagnostics[0].subject, std::string("MaterialA"));
	EXPECT_EQ(diagnostics[1].severity, quader::io::IoSeverity::Error);
}

TEST(IoDiagnostic, IoDiagnosticsConvertToFoundationDiagnostics) {
	quader::io::IoDiagnosticList diagnostics;
	diagnostics.add(quader::io::IoSeverity::Info,
			quader::io::IoDiagnosticCode::Unknown,
			"loaded optional metadata");
	diagnostics.add(quader::io::IoSeverity::Warning,
			quader::io::IoDiagnosticCode::UnsupportedFeaturePreserved,
			"preserved extension");
	diagnostics.add_error(quader::io::IoDiagnosticCode::ValidationFailed, "invalid mesh");

	const auto kConverted = quader::io::to_foundation_diagnostics(diagnostics);
	EXPECT_EQ(kConverted.size(), 3U);
	EXPECT_TRUE(kConverted.has_errors());
	EXPECT_EQ(kConverted[0].severity, quader::foundation::Severity::Info);
	EXPECT_EQ(kConverted[1].severity, quader::foundation::Severity::Warning);
	EXPECT_EQ(kConverted[2].severity, quader::foundation::Severity::Error);
	EXPECT_EQ(kConverted[2].message, std::string("invalid mesh"));
}

} // namespace
