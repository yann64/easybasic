add_executable(easybasic_unit_tests
    lexer_test.cpp
    sema_test.cpp
    runtime_pbstring_test.cpp
    ${CMAKE_SOURCE_DIR}/compiler/src/sema/sema.cpp
)
target_link_libraries(easybasic_unit_tests PRIVATE
    Catch2::Catch2WithMain
    pbcxx_frontend
    easybasic_runtime
)

include(Catch)
catch_discover_tests(easybasic_unit_tests)
