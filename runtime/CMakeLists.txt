# Header-only for now (PBString/PBArray/PBList/PBMap are all templates or
# small inline wrappers) - becomes a compiled target once non-template
# runtime support code needs a .cpp of its own.
add_library(easybasic_runtime INTERFACE)
target_include_directories(easybasic_runtime INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(easybasic_runtime INTERFACE cxx_std_20)
# threadlib.hpp (M7a) needs a real pthread/Win32-threads backend for
# std::thread/std::mutex/std::counting_semaphore - CMake's standard
# "find and link the right threading library for this platform" module.
find_package(Threads REQUIRED)
target_link_libraries(easybasic_runtime INTERFACE Threads::Threads)
