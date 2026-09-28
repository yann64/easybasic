#pragma once

#include <iostream>
#include <string>

namespace easybasic::runtime {

/// Emits one `Debug` line, matching real PureBasic's debugger console
/// format exactly (oracle-verified byte-for-byte via `pbcompilerc -d`:
/// `"[Debugger]  " + value + '\n'`, two spaces after the closing bracket).
///
/// Only called from generated code when `pbcxx` was invoked in debug mode
/// (mirroring `pbcompilerc -d`) - a plain build emits no call at all for a
/// `Debug` statement, exactly like a plain (non-`-d`) `pbcompilerc` build
/// strips it to nothing. See docs/architecture/roadmap.md's M0 notes.
inline void debugPrint(const std::string& text) {
    std::cout << "[Debugger]  " << text << '\n';
}

} // namespace easybasic::runtime
