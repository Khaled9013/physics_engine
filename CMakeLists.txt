cmake_minimum_required(VERSION 3.20)
project(ballistics_sim VERSION 0.1.0 LANGUAGES C)

option(BALLISTICS_BUILD_SHARED "Build ballistics_core as a shared library" OFF)
option(BALLISTICS_BUILD_TESTS "Build test targets" ON)
option(BALLISTICS_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(BALLISTICS_ENABLE_COVERAGE "Enable GCC/Clang coverage instrumentation" OFF)
option(BALLISTICS_DEBUG_ENABLED "Compile debug logging support" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)
include(CodeCoverage)
include(PortSelection)

ballistics_reject_unsafe_math_flags()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(BALLISTICS_BUILD_SHARED)
    set(BALLISTICS_LIBRARY_TYPE SHARED)
else()
    set(BALLISTICS_LIBRARY_TYPE STATIC)
endif()

ballistics_select_port(BALLISTICS_PORT_SOURCE)

add_library(ballistics_core ${BALLISTICS_LIBRARY_TYPE}
    src/core/status.c
    src/debug/ballistics_debug.c
    ${BALLISTICS_PORT_SOURCE}
)
add_library(ballistics::core ALIAS ballistics_core)

target_compile_features(ballistics_core PUBLIC c_std_17)
set_target_properties(ballistics_core PROPERTIES
    C_EXTENSIONS OFF
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(ballistics_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(ballistics_core
    PRIVATE
        $<$<BOOL:${BALLISTICS_BUILD_SHARED}>:BALLISTICS_BUILD_SHARED>
        BALLISTICS_DEBUG_ENABLED=$<BOOL:${BALLISTICS_DEBUG_ENABLED}>
        $<$<CONFIG:Release>:BALLISTICS_RELEASE_LOGGING=1>
)
target_compile_options(ballistics_core PRIVATE $<$<CONFIG:Release>:-O2>)
ballistics_set_warnings(ballistics_core)
ballistics_enable_sanitizers(ballistics_core)
ballistics_enable_coverage(ballistics_core)

# These named skeleton targets are replaced by real executable/test targets at their
# implementation stages, while preserving the required top-level target names.
add_custom_target(ballistics_cli)
if(BALLISTICS_BUILD_TESTS)
    enable_testing()
    add_custom_target(ballistics_tests)
endif()
