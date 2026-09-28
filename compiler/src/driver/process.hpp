#pragma once

#include <string>
#include <vector>

namespace easybasic {

/// Result of running a child process to completion.
struct ProcessResult {
    int exitCode = -1;
    bool launched = false; ///< False if the process could not even be started.
};

/// Runs `executable` with `args` (not including argv[0]) to completion,
/// inheriting this process's stdio. POSIX today (fork/execvp/waitpid);
/// gains a Windows CreateProcess path when the windows-mingw preset (M6)
/// needs it - kept as a single seam now so that addition doesn't ripple
/// through the rest of the driver.
ProcessResult runProcess(const std::string& executable, const std::vector<std::string>& args);

} // namespace easybasic
