#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace quader::foundation {

enum class Severity {
    info,
    warning,
    error,
};

struct Diagnostic {
    Severity severity = Severity::info;
    std::string message;
};

class DiagnosticList final {
public:
    using container_type = std::vector<Diagnostic>;
    using const_iterator = container_type::const_iterator;

    void add(Diagnostic diagnostic)
    {
        diagnostics_.push_back(std::move(diagnostic));
    }

    void add(Severity severity, std::string message)
    {
        add(Diagnostic{severity, std::move(message)});
    }

    void clear() noexcept
    {
        diagnostics_.clear();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return diagnostics_.empty();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return diagnostics_.size();
    }

    [[nodiscard]] bool has_errors() const noexcept
    {
        for (const auto& diagnostic : diagnostics_) {
            if (diagnostic.severity == Severity::error) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] std::span<const Diagnostic> diagnostics() const noexcept
    {
        return diagnostics_;
    }

    [[nodiscard]] const Diagnostic& operator[](std::size_t index) const noexcept
    {
        return diagnostics_[index];
    }

    [[nodiscard]] const_iterator begin() const noexcept
    {
        return diagnostics_.begin();
    }

    [[nodiscard]] const_iterator end() const noexcept
    {
        return diagnostics_.end();
    }

private:
    container_type diagnostics_;
};

} // namespace quader::foundation
