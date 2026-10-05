include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_subdirectory(unit)

# --- Golden e2e tests: fixed expected.stdout, no external PureBasic install
# required. Discovers tests/e2e/<case>/{input.pb,expected.stdout} pairs.
# GUI (M7b+) cases are run via run_gui_case.sh instead of plain
# run_case.sh - it skips gracefully (SKIP_RETURN_CODE) rather than failing
# when this environment has no GTK3 dev files or no usable display, the
# same way e2e_diff cases skip when pbcompilerc isn't installed.
set(EASYBASIC_GUI_E2E_CASES gui_window_core gui_basic_gadgets gui_menu_statusbar gui_image gui_toolbar gui_systray gui_container gui_panel gui_splitter gui_scrollarea gui_listview_combobox gui_listicon_tree)
file(GLOB EASYBASIC_E2E_CASES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/e2e ${CMAKE_CURRENT_SOURCE_DIR}/e2e/*)
foreach(case ${EASYBASIC_E2E_CASES})
    set(case_dir ${CMAKE_CURRENT_SOURCE_DIR}/e2e/${case})
    if(EXISTS ${case_dir}/input.pb)
        if(case IN_LIST EASYBASIC_GUI_E2E_CASES)
            set(runner run_gui_case.sh)
        else()
            set(runner run_case.sh)
        endif()
        add_test(
            NAME e2e_${case}
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/e2e/${runner} $<TARGET_FILE:pbcxx> ${case_dir}
        )
        if(case IN_LIST EASYBASIC_GUI_E2E_CASES)
            set_tests_properties(e2e_${case} PROPERTIES SKIP_RETURN_CODE 125)
        endif()
    endif()
endforeach()

# --- Differential e2e tests against the real pbcompilerc, when installed
# (see scripts/diff_against_pbcompilerc.sh - skips gracefully otherwise).
file(GLOB EASYBASIC_E2E_DIFF_CASES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/e2e_diff ${CMAKE_CURRENT_SOURCE_DIR}/e2e_diff/*)
foreach(case ${EASYBASIC_E2E_DIFF_CASES})
    set(case_input ${CMAKE_CURRENT_SOURCE_DIR}/e2e_diff/${case}/input.pb)
    if(EXISTS ${case_input})
        add_test(
            NAME e2e_diff_${case}
            COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/diff_against_pbcompilerc.sh $<TARGET_FILE:pbcxx> ${case_input}
        )
    endif()
endforeach()
