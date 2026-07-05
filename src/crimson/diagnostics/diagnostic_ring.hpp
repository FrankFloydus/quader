#pragma once

#include "crimson/renderer_diagnostics.hpp"

#include <cstddef>
#include <vector>

namespace crimson {

class DiagnosticRing final {
public:
    explicit DiagnosticRing(std::size_t capacity = 256);

    void push(RendererDiagnostic diagnostic);
    [[nodiscard]] std::vector<RendererDiagnostic> snapshot() const;
    void clear() noexcept;

private:
    std::vector<RendererDiagnostic> diagnostics_;
    std::size_t capacity_ = 0;
    std::size_t next_index_ = 0;
    bool wrapped_ = false;
};

[[nodiscard]] DiagnosticRing make_diagnostic_ring_from_recent(
    const std::vector<RendererDiagnostic>& diagnostics,
    std::size_t capacity = 256);

} // namespace crimson
