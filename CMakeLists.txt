# +-----------------------------------------+
# |              License: MIT               |
# +-----------------------------------------+
# | Copyright (c) 2024                      |
# | Author: Gleb Trufanov (aka Glebchansky) |
# +-----------------------------------------+

cmake_minimum_required(VERSION 3.21)

project(ISS-Driver-Drowsiness-Detector)

include(cmake/CheckSystem.cmake)

set(MAIN_TARGET DDDetector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/src/ui)

check_system()

set(EXECUTABLE_DESTINATION_PATH ${CMAKE_BINARY_DIR})

if (UNIX)
    set(EXECUTABLE_DESTINATION_PATH ${CMAKE_BINARY_DIR}/bin)
endif ()

message(STATUS "Set the output directory for the executable to " ${EXECUTABLE_DESTINATION_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXECUTABLE_DESTINATION_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXECUTABLE_DESTINATION_PATH})

set(RESOURCE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resource)
set(RESOURCE_BINARY_DIR ${CMAKE_BINARY_DIR}/resource)

# Project sources
set(TS_FILES ${RESOURCE_SOURCE_DIR}/translation/${PROJECT_NAME}_ru_RU.ts)
set(RC_FILES ${RESOURCE_SOURCE_DIR}/resource.qrc ${RESOURCE_SOURCE_DIR}/executable_icon_resource.rc)

set(PROJECT_SOURCES
    ${RC_FILES}
    ${CMAKE_SOURCE_DIR}/src/ui/mainwindow.ui
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/Camera.cpp
    ${CMAKE_SOURCE_DIR}/src/ImageRecognizer.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/TextBrowserLogger.cpp
)
#####################################################################################

# Find packages
find_package(Qt6 6.3 REQUIRED COMPONENTS
    LinguistTools
    Widgets
    Multimedia
)

find_package(OpenCV 4.5.4 REQUIRED) # 4.5.4 is the minimum version
#####################################################################################

add_executable(${MAIN_TARGET} ${PROJECT_SOURCES})

# Copying the neural network model and warning sound to the folder with the executable file
# It only works during CMake configuration (build generation), not during the actual build
file(COPY ${RESOURCE_SOURCE_DIR}/neural_network_model DESTINATION ${RESOURCE_BINARY_DIR})
file(COPY ${RESOURCE_SOURCE_DIR}/sound DESTINATION ${RESOURCE_BINARY_DIR})

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${RESOURCE_BINARY_DIR}/translation)
qt6_add_translations(${MAIN_TARGET} TS_FILES ${TS_FILES})

target_include_directories(${MAIN_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/utils
    ${OpenCV_INCLUDE_DIRS}
)

if (UNIX)
    set(RPATH_LIBRARY_DIRECTORIES Qt6 opencv cuda)

    foreach (RPATH_DIRECTORY ${RPATH_LIBRARY_DIRECTORIES})
        list(APPEND RPATH_LIST "$ORIGIN/../lib64/${RPATH_DIRECTORY}")
    endforeach ()

    list(JOIN RPATH_LIST ":" RPATH_LIST)

    # Lots of problems with escaping $ORIGIN in CMake (target_link_options especially), but thanks https://stackoverflow.com/a/75790542
    set_target_properties(${MAIN_TARGET} PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath,'${RPATH_LIST}'")
endif ()

target_link_libraries(${MAIN_TARGET} PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    ${OpenCV_LIBS}
)
