#include "diagnostics.hpp"

namespace easybasic {

void DiagnosticEngine::error(SourceLoc loc, std::string message) {
    diagnostics_.push_back(Diagnostic{Severity::Error, loc, std::move(message)});
    ++errorCount_;
}

void DiagnosticEngine::warning(SourceLoc loc, std::string message) {
    diagnostics_.push_back(Diagnostic{Severity::Warning, loc, std::move(message)});
}

int DiagnosticEngine::registerFile(const std::string& path) {
    auto it = fileIds_.find(path);
    if (it != fileIds_.end()) {
        return it->second;
    }
    int id = static_cast<int>(fileNames_.size());
    fileNames_.push_back(path);
    fileIds_.emplace(path, id);
    return id;
}

const std::string& DiagnosticEngine::fileName(int fileId) const {
    static const std::string unknown = "<unknown>";
    if (fileId < 0 || static_cast<std::size_t>(fileId) >= fileNames_.size()) {
        return unknown;
    }
    return fileNames_[static_cast<std::size_t>(fileId)];
}

void DiagnosticEngine::printAll(std::ostream& os) const {
    for (const auto& diag : diagnostics_) {
        os << fileName(diag.loc.fileId) << ':' << diag.loc.line << ':' << diag.loc.column
           << ": " << (diag.severity == Severity::Error ? "error" : "warning") << ": "
           << diag.message << '\n';
    }
}

} // namespace easybasic
