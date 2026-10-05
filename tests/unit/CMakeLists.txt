add_executable(easybasic_unit_tests
    lexer_test.cpp
    macro_expander_test.cpp
    sema_test.cpp
    runtime_pbstring_test.cpp
    runtime_stringlib_test.cpp
    runtime_mathlib_test.cpp
    runtime_memorylib_test.cpp
    runtime_filelib_test.cpp
    runtime_datelib_test.cpp
    runtime_datalib_test.cpp
    runtime_threadlib_test.cpp
    ${CMAKE_SOURCE_DIR}/compiler/src/sema/sema.cpp
)
target_link_libraries(easybasic_unit_tests PRIVATE
    Catch2::Catch2WithMain
    pbcxx_frontend
    easybasic_runtime
)

# runtime_guilib_test.cpp includes guilib.hpp directly (M7b), so it needs
# GTK3's dev headers/libs at *this test binary's own* build time - unlike
# pbcxx's own generated-program compilation, which only links GTK3 for a
# program that actually uses GUI commands (see main.cpp's own notes).
# GTK3 is deliberately treated as optional here too (not REQUIRED), the
# same "don't force it on platforms that don't have it yet" stance -
# Windows-MinGW CI doesn't install it yet and Haiku doesn't have
# gtk3_devel installed yet, and neither of those should lose the rest of
# the unit test suite over it. Linux CI (gcc/clang/sanitizers/
# static-analysis) does install libgtk-3-dev, so it gets full coverage.
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(GTK3 QUIET IMPORTED_TARGET gtk+-3.0)
endif()
if(GTK3_FOUND)
    target_sources(easybasic_unit_tests PRIVATE runtime_guilib_test.cpp)
    target_link_libraries(easybasic_unit_tests PRIVATE PkgConfig::GTK3)
else()
    message(STATUS "GTK3 not found via pkg-config - skipping runtime_guilib_test.cpp (M7b GUI unit tests)")
endif()

# runtime_systraylib_test.cpp includes systraylib.hpp (M7b's SysTrayIcon
# slice), which needs ayatana-appindicator3's own dev headers/libs at *this
# test binary's own* build time, the identical optional-dependency pattern
# GTK3 itself already has just above (and for the identical reason: a
# generated program only links it when the program actually calls a
# SysTray builtin - see main.cpp's own notes). A separate, narrower check
# than GTK3's own, since most GUI programs never touch SysTray at all.
if(PkgConfig_FOUND)
    pkg_check_modules(AYATANA_APPINDICATOR3 QUIET IMPORTED_TARGET ayatana-appindicator3-0.1)
endif()
if(AYATANA_APPINDICATOR3_FOUND)
    target_sources(easybasic_unit_tests PRIVATE runtime_systraylib_test.cpp)
    target_link_libraries(easybasic_unit_tests PRIVATE PkgConfig::AYATANA_APPINDICATOR3)
else()
    message(STATUS "ayatana-appindicator3 not found via pkg-config - skipping runtime_systraylib_test.cpp "
                    "(M7b SysTrayIcon unit tests)")
endif()

include(Catch)
catch_discover_tests(easybasic_unit_tests)
