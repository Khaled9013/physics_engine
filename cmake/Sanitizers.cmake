function(ballistics_enable_sanitizers target_name)
    if(NOT BALLISTICS_ENABLE_SANITIZERS)
        return()
    endif()
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${target_name} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
    else()
        message(WARNING "Sanitizers requested but unsupported by ${CMAKE_C_COMPILER_ID}")
    endif()
endfunction()
