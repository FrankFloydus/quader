#include "crimson/diagnostics/diagnostic_ring.hpp"

#include <algorithm>
#include <utility>

namespace crimson {

DiagnosticRing::DiagnosticRing(std::size_t capacity)
    : capacity_(capacity)
{
    diagnostics_.reserve(capacity_);
}

void DiagnosticRing::push(RendererDiagnostic diagnostic)
{
    if (capacity_ == 0) {
        return;
    }

    if (diagnostics_.size() < capacity_) {
        diagnostics_.push_back(std::move(diagnostic));
        return;
    }

    diagnostics_[next_index_] = std::move(diagnostic);
    next_index_ = (next_index_ + 1) % capacity_;
    wrapped_ = true;
}

std::vector<RendererDiagnostic> DiagnosticRing::snapshot() const
{
    if (!wrapped_) {
        return diagnostics_;
    }

    std::vector<RendererDiagnostic> ordered;
    ordered.reserve(diagnostics_.size());
    for (std::size_t offset = 0; offset < diagnostics_.size(); ++offset) {
        ordered.push_back(diagnostics_[(next_index_ + offset) % diagnostics_.size()]);
    }
    return ordered;
}

void DiagnosticRing::clear() noexcept
{
    diagnostics_.clear();
    next_index_ = 0;
    wrapped_ = false;
}

DiagnosticRing make_diagnostic_ring_from_recent(
    const std::vector<RendererDiagnostic>& diagnostics,
    std::size_t capacity)
{
    DiagnosticRing ring(capacity);
    const std::size_t first = diagnostics.size() > capacity ? diagnostics.size() - capacity : 0;
    for (std::size_t index = first; index < diagnostics.size(); ++index) {
        ring.push(diagnostics[index]);
    }
    return ring;
}

} // namespace crimson
