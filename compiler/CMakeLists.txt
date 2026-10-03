# pbcxx_frontend: Preprocessor+Lexer+Parser+Diagnostics only, deliberately
# split out from the rest of the compiler from day one (mirrors eBasic's own
# ebasic_frontend) so a future docgen/LSP tool can parse real PB source the
# exact same way `pbcxx` itself does, without linking Sema/Codegen at all.
# (CompilerIf/DataSection ended up living in Sema instead - see their own
# doc comments; Macro's own token-level textual substitution is the one
# piece that genuinely has to run before the Parser, so it's the first real
# preprocessor/ source.)
add_library(pbcxx_frontend STATIC
    src/diagnostics/diagnostics.cpp
    src/lexer/lexer.cpp
    src/preprocessor/macro_expander.cpp
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
