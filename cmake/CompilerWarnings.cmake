function(ballistics_set_warnings target_name)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Wdouble-promotion
            -Wformat=2
        )
    endif()
endfunction()

function(ballistics_reject_unsafe_math_flags)
    set(_ballistics_flag_sets
        "${CMAKE_C_FLAGS}"
        "${CMAKE_C_FLAGS_DEBUG}"
        "${CMAKE_C_FLAGS_RELEASE}"
        "${CMAKE_C_FLAGS_RELWITHDEBINFO}"
        "${CMAKE_C_FLAGS_MINSIZEREL}"
    )
    foreach(_flags IN LISTS _ballistics_flag_sets)
        if(_flags MATCHES "(^|[ ;])(-ffast-math|-Ofast|-funsafe-math-optimizations)([ ;]|$)")
            message(FATAL_ERROR "Unsafe floating-point option is forbidden: ${_flags}")
        endif()
    endforeach()
endfunction()
