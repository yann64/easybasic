# pbcxx_frontend: Preprocessor+Lexer+Parser+Diagnostics only, deliberately
# split out from the rest of the compiler from day one (mirrors eBasic's own
# ebasic_frontend) so a future docgen/LSP tool can parse real PB source the
# exact same way `pbcxx` itself does, without linking Sema/Codegen at all.
# (Preprocessor has no sources yet - CompilerIf/Macro/DataSection land in M5.)
add_library(pbcxx_frontend STATIC
    src/diagnostics/diagnostics.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
)
target_include_directories(pbcxx_frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(pbcxx_frontend PUBLIC cxx_std_20)

add_executable(pbcxx
    src/sema/sema.cpp
    src/codegen/codegen.cpp
    src/driver/main.cpp
    src/driver/process.cpp
)
target_link_libraries(pbcxx PRIVATE pbcxx_frontend easybasic_runtime)
target_include_directories(pbcxx PRIVATE ${CMAKE_BINARY_DIR}/generated)
