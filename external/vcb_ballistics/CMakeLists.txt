add_library(vcb_ballistics STATIC
    source/vcb_ballistics.c
)
add_library(vcb::ballistics ALIAS vcb_ballistics)

target_compile_features(vcb_ballistics PUBLIC c_std_11)
set_target_properties(vcb_ballistics PROPERTIES C_EXTENSIONS OFF)
target_include_directories(vcb_ballistics PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
)
if(UNIX)
    target_link_libraries(vcb_ballistics PUBLIC m)
endif()
ballistics_set_warnings(vcb_ballistics)
ballistics_enable_sanitizers(vcb_ballistics)
ballistics_enable_coverage(vcb_ballistics)

if(BALLISTICS_BUILD_TESTS)
    function(vcb_configure_test target_name)
        target_compile_features(${target_name} PRIVATE c_std_11)
        set_target_properties(${target_name} PROPERTIES C_EXTENSIONS OFF)
        target_include_directories(${target_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/source
        )
        if(UNIX)
            target_link_libraries(${target_name} PRIVATE m)
        endif()
        ballistics_set_warnings(${target_name})
        ballistics_enable_sanitizers(${target_name})
        ballistics_enable_coverage(${target_name})
    endfunction()

    foreach(step_suffix IN ITEMS 025 0125 00625)
        add_library(vcb_ballistics_step_${step_suffix} OBJECT
            source/vcb_ballistics.c
        )
        vcb_configure_test(vcb_ballistics_step_${step_suffix})
    endforeach()
    target_compile_definitions(vcb_ballistics_step_025 PRIVATE
        vcb_ballistics_solve=vcb_ballistics_solve_step_025
        VCB_BALLISTICS_STEP_M=0.25f
    )
    target_compile_definitions(vcb_ballistics_step_0125 PRIVATE
        vcb_ballistics_solve=vcb_ballistics_solve_step_0125
        VCB_BALLISTICS_STEP_M=0.125f
    )
    target_compile_definitions(vcb_ballistics_step_00625 PRIVATE
        vcb_ballistics_solve=vcb_ballistics_solve_step_00625
        VCB_BALLISTICS_STEP_M=0.0625f
    )

    add_executable(vcb_ballistics_validator
        tests/ballistics/test_vcb_ballistics.c
    )
    target_link_libraries(vcb_ballistics_validator PRIVATE vcb::ballistics)
    vcb_configure_test(vcb_ballistics_validator)
    add_test(NAME vcb_ballistics_validator COMMAND vcb_ballistics_validator
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ballistics/reference_cases.csv
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ballistics/reference_cases_g7.csv
    )

    set(vcb_all_solver_objects
        $<TARGET_OBJECTS:vcb_ballistics_step_025>
        $<TARGET_OBJECTS:vcb_ballistics_step_0125>
        $<TARGET_OBJECTS:vcb_ballistics_step_00625>
    )

    add_executable(vcb_ballistics_review
        tests/ballistics-review/test_ballistics_review.c
        ${vcb_all_solver_objects}
    )
    target_link_libraries(vcb_ballistics_review PRIVATE vcb::ballistics)
    vcb_configure_test(vcb_ballistics_review)
    add_test(NAME vcb_ballistics_review COMMAND vcb_ballistics_review)

    foreach(test_name IN ITEMS g7_knot convergence_order)
        add_executable(vcb_ballistics_${test_name}
            tests/ballistics-review/test_${test_name}.c
            ${vcb_all_solver_objects}
        )
        target_link_libraries(vcb_ballistics_${test_name} PRIVATE vcb::ballistics)
        vcb_configure_test(vcb_ballistics_${test_name})
        add_test(NAME vcb_ballistics_${test_name}
            COMMAND vcb_ballistics_${test_name}
        )
    endforeach()

    add_executable(vcb_ballistics_zero_bisection
        tests/ballistics-review/test_zero_bisection.c
    )
    target_link_libraries(vcb_ballistics_zero_bisection PRIVATE vcb::ballistics)
    vcb_configure_test(vcb_ballistics_zero_bisection)
    add_test(NAME vcb_ballistics_zero_bisection
        COMMAND vcb_ballistics_zero_bisection
    )

    foreach(test_name IN ITEMS g1_thresholds g7_thresholds closed_form)
        add_executable(vcb_ballistics_${test_name}
            tests/ballistics-review/test_${test_name}.c
        )
        vcb_configure_test(vcb_ballistics_${test_name})
        add_test(NAME vcb_ballistics_${test_name}
            COMMAND vcb_ballistics_${test_name}
        )
    endforeach()

    add_executable(vcb_ballistics_benchmark
        tests/ballistics-review/benchmark_ballistics.c
    )
    target_link_libraries(vcb_ballistics_benchmark PRIVATE vcb::ballistics)
    vcb_configure_test(vcb_ballistics_benchmark)
endif()
