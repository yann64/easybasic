#include "process.hpp"

#ifdef _WIN32
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <vector>

namespace easybasic {

#ifdef _WIN32

ProcessResult runProcess(const std::string& executable, const std::vector<std::string>& args) {
    // Minimal stopgap for the windows-mingw preset; a fuller CreateProcess-
    // based implementation (proper argument quoting, pipe redirection) is
    // M6's job once Windows CI actually exercises this path.
    std::vector<const char*> argv;
    argv.push_back(executable.c_str());
    for (const auto& a : args) {
        argv.push_back(a.c_str());
    }
    argv.push_back(nullptr);
    int rc = static_cast<int>(_spawnvp(_P_WAIT, executable.c_str(), argv.data()));
    if (rc == -1) {
        return ProcessResult{-1, false};
    }
    return ProcessResult{rc, true};
}

#else

ProcessResult runProcess(const std::string& executable, const std::vector<std::string>& args) {
    // execvp's signature (char *const argv[]) predates `const` in C and is
    // documented to never actually modify these strings - the const_cast is
    // required to call it at all, not a real constness violation.
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(executable.c_str())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    for (const auto& a : args) {
        argv.push_back(const_cast<char*>(a.c_str())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) {
        return ProcessResult{-1, false};
    }
    if (pid == 0) {
        execvp(executable.c_str(), argv.data());
        _exit(127); // execvp only returns on failure
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return ProcessResult{-1, true};
    }
    if (WIFEXITED(status)) {
        return ProcessResult{WEXITSTATUS(status), true};
    }
    return ProcessResult{-1, true};
}

#endif

} // namespace easybasic
