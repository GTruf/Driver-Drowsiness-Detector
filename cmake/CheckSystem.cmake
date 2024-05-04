function(check_system)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        if (WIN32)
            message(STATUS "Windows operating system detected")

            if (NOT MSVC)
                message(FATAL_ERROR "The project is designed to work with the MSVC compiler when compiling on Windows operating system")
            endif ()

            message(STATUS "Disabling the console at program start-up")
            add_link_options(/SUBSYSTEM:windows /ENTRY:mainCRTStartup) # To hide the console window at program start-up on Windows
        elseif (UNIX AND NOT APPLE)
            message(STATUS "Linux kernel operating system detected")

            if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                message(FATAL_ERROR "The project is designed to work with the GNU compiler (GCC) when compiling on Linux operating system")
            endif ()
        else ()
            message(FATAL_ERROR "At this point, the project is only designed to run on Windows or Linux operating systems")
        endif ()
    else ()
        message(FATAL_ERROR "The project is designed to work with an operating system that supports 64-bit extensions")
    endif ()
endfunction()