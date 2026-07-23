add_executable(ballistics_cli
    main.c
    cli_options.c
    file_byte_sink.c
)

target_compile_features(ballistics_cli PRIVATE c_std_17)
set_target_properties(ballistics_cli PROPERTIES C_EXTENSIONS OFF)
target_link_libraries(ballistics_cli PRIVATE ballistics::core)
target_include_directories(ballistics_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
ballistics_set_warnings(ballistics_cli)
ballistics_enable_sanitizers(ballistics_cli)
ballistics_enable_coverage(ballistics_cli)
