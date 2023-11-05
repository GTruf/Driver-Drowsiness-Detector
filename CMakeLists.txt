# +-----------------------------------------+
# |              License: MIT               |
# +-----------------------------------------+
# | Copyright (c) 2023                      |
# | Author: Gleb Trufanov (aka Glebchansky) |
# +-----------------------------------------+

cmake_minimum_required(VERSION 3.21)

project(ISS-Driver-Drowsiness-Detector)

set(MAIN_TARGET DDDetector)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/src/ui)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (WIN32)
        message(STATUS "Windows operating system detected")

        if (NOT MSVC)
            message(FATAL_ERROR "The project is designed to work with the MSVC compiler when compiling on a Windows operating system")
        endif ()

        message(STATUS "Disabling the console at program start-up")
        add_link_options(/SUBSYSTEM:windows /ENTRY:mainCRTStartup) # To hide the console window at program start-up on Windows
    elseif (UNIX AND NOT APPLE)
        message(STATUS "Linux kernel operating system detected")
    else ()
        message(FATAL_ERROR "At this point, the project is only designed to run on Windows or Linux operating systems")
    endif ()
else ()
    message(FATAL_ERROR "The project is designed to work with an operating system that supports 64-bit extensions")
endif ()

set(RESOURCE_CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resource)
set(RESOURCE_CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/resource)

message(STATUS "Set the output directory for the executable to " ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})

# Project sources
set(TS_FILES ${RESOURCE_CMAKE_SOURCE_DIR}/translation/${PROJECT_NAME}_ru_RU.ts)
set(RC_FILES ${RESOURCE_CMAKE_SOURCE_DIR}/resource.qrc ${RESOURCE_CMAKE_SOURCE_DIR}/executable_icon_resource.rc)

set(PROJECT_SOURCES
    ${RC_FILES}
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/MainWindow.h
    ${CMAKE_SOURCE_DIR}/src/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/Camera.h
    ${CMAKE_SOURCE_DIR}/src/Camera.cpp
    ${CMAKE_SOURCE_DIR}/src/ImageRecognizer.h
    ${CMAKE_SOURCE_DIR}/src/ImageRecognizer.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.h
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/RecognitionFrameCounter.h
    ${CMAKE_SOURCE_DIR}/src/utils/TextBrowserLogger.h
    ${CMAKE_SOURCE_DIR}/src/utils/TextBrowserLogger.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/mainwindow.ui
)
#####################################################################################

# Find packages
find_package(Qt6 COMPONENTS REQUIRED
    LinguistTools
    Widgets
    Multimedia
)

find_package(OpenCV 4.5.4 REQUIRED) # 4.5.4 is the minimum version
#####################################################################################

# Uncomment this to create a QM file based on the TS file.
# A deprecated but working way to update .ts files based on new translations in the source code
# qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

add_executable(${MAIN_TARGET} ${PROJECT_SOURCES})

# Copying the neural network model and warning sound to the folder with the executable file
# It only works when running cmake command, not during actual build time
file(COPY ${RESOURCE_CMAKE_SOURCE_DIR}/neural_network_model DESTINATION ${RESOURCE_CMAKE_BINARY_DIR})
file(COPY ${RESOURCE_CMAKE_SOURCE_DIR}/sound DESTINATION ${RESOURCE_CMAKE_BINARY_DIR})

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${RESOURCE_CMAKE_BINARY_DIR}/translation)
qt6_add_translations(${MAIN_TARGET} TS_FILES ${TS_FILES})

target_include_directories(${MAIN_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/utils
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(${MAIN_TARGET} PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    ${OpenCV_LIBS}
)
