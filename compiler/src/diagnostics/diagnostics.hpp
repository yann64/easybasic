#pragma once

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace easybasic {

/// A location in original PureBasic source. `fileId` indexes into
/// DiagnosticEngine's file registry (see registerFile()), so a diagnostic
/// raised anywhere - including inside an IncludeFile'd file, or inside code
/// reached only after macro/CompilerIf expansion - still reports its true
/// originating file and line, not just the top-level input file's.
struct SourceLoc {
    int line = 0;
    int column = 0;
    int fileId = 0;
};

/// Warning never stops compilation (checked via hasErrors(), not a separate
/// warning count) - the pipeline only has these two severities.
enum class Severity {
    Error,
    Warning,
};

/// One reported problem, ready to be formatted by DiagnosticEngine::printAll().
struct Diagnostic {
    Severity severity;
    SourceLoc loc;
    std::string message;
};

/// Accumulates diagnostics across an entire compile (preprocessor through
/// codegen) rather than having each stage report independently. This lets
/// every pipeline stage keep running after an error - so a single `pbcxx`
/// invocation can surface multiple real problems at once - while each stage
/// still checks hasErrors() before handing its result to the next one.
class DiagnosticEngine {
public:
    void error(SourceLoc loc, std::string message);
    void warning(SourceLoc loc, std::string message);

    /// True once at least one error() has been recorded.
    bool hasErrors() const { return errorCount_ > 0; }
    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }

    /// Registers a file path, returning a stable id for use in SourceLoc.
    /// Registering the same path again returns the same id.
    int registerFile(const std::string& path);

    /// The path passed to registerFile() for `fileId`, or "<unknown>" for an
    /// out-of-range id.
    const std::string& fileName(int fileId) const;

    /// Writes every recorded diagnostic to `os`, one per line, in report order.
    void printAll(std::ostream& os) const;

private:
    std::vector<Diagnostic> diagnostics_;
    std::vector<std::string> fileNames_;
    std::unordered_map<std::string, int> fileIds_;
    int errorCount_ = 0;
};

} // namespace easybasic
