# Header-only for now (PBString/PBArray/PBList/PBMap are all templates or
# small inline wrappers) - becomes a compiled target once non-template
# runtime support code needs a .cpp of its own.
add_library(easybasic_runtime INTERFACE)
target_include_directories(easybasic_runtime INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(easybasic_runtime INTERFACE cxx_std_20)
