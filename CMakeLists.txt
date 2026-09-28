cmake_minimum_required(VERSION 3.20)
project(easybasic VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Best-effort short git commit hash for `pbcxx --version`; degrades gracefully
# to empty when there's no .git (e.g. a source tarball) or no git binary.
set(EASYBASIC_GIT_HASH "")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE EASYBASIC_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _easybasic_git_rc
    )
    if(NOT _easybasic_git_rc EQUAL 0)
        set(EASYBASIC_GIT_HASH "")
    endif()
endif()
configure_file(
    ${CMAKE_SOURCE_DIR}/compiler/src/driver/version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/easybasic/version.hpp
    @ONLY
)

# -Wswitch (part of -Wall) is what would have caught a real M2 bug during
# development: Codegen's genStmt/genExpr switches silently did nothing for
# newly-added ast::StmtKind/ExprKind values before -Wall was added here,
# because an unmatched case with no `default:` is well-defined C++ (falls
# through to after the switch), not a compile error - only a warning flags
# it. Kept as a plain warning (not -Werror): CI's own clang-tidy/cppcheck
# jobs are the hard gates; this is a fast, always-on local safety net.
add_compile_options(-Wall -Wextra)

option(EASYBASIC_SANITIZE "Build with -fsanitize=address,undefined" OFF)
if(EASYBASIC_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

include(GNUInstallDirs)

add_subdirectory(runtime)
add_subdirectory(compiler)

include(CTest) # also calls enable_testing(); registers MEMORYCHECK_COMMAND etc.
                # for the nightly Valgrind workflow (.github/workflows/nightly.yml)
add_subdirectory(tests)

# Doxygen API docs: cmake --build <preset> --target docs
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/docs)
    file(MAKE_DIRECTORY ${DOXYGEN_OUT})
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E env
            "EASYBASIC_DOXYGEN_INPUT=${CMAKE_SOURCE_DIR}/compiler/src ${CMAKE_SOURCE_DIR}/frontend/src ${CMAKE_SOURCE_DIR}/runtime/include"
            "EASYBASIC_DOXYGEN_OUTPUT=${DOXYGEN_OUT}"
            ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating Doxygen API documentation into ${DOXYGEN_OUT}/html"
        VERBATIM
    )
endif()
