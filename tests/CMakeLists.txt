add_library(ballistics_unity STATIC vendor/unity/unity.c)
target_compile_features(ballistics_unity PRIVATE c_std_17)
target_include_directories(ballistics_unity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor/unity)
target_compile_definitions(ballistics_unity PUBLIC UNITY_INCLUDE_DOUBLE)
ballistics_enable_sanitizers(ballistics_unity)
ballistics_enable_coverage(ballistics_unity)

add_executable(ballistics_tests
    unit/test_main.c
    unit/test_vector3.c
    unit/test_equations.c
    unit/test_gravity_model.c
    unit/test_drag_model.c
    unit/test_registry_config_status.c
    unit/test_euler_integrator.c
    unit/test_rk4_integrator.c
)
target_compile_features(ballistics_tests PRIVATE c_std_17)
set_target_properties(ballistics_tests PROPERTIES C_EXTENSIONS OFF)
target_link_libraries(ballistics_tests PRIVATE ballistics::core ballistics_unity)
ballistics_set_warnings(ballistics_tests)
ballistics_enable_sanitizers(ballistics_tests)
ballistics_enable_coverage(ballistics_tests)
add_test(NAME ballistics_unit_tests COMMAND ballistics_tests)
