cmake_minimum_required(VERSION 3.12)

project(ImGuiManager VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED NO)
set(CMAKE_CXX_EXTENSIONS YES)

# Add path for custom modules
list(INSERT CMAKE_MODULE_PATH 0
        "${CMAKE_CURRENT_SOURCE_DIR}/CMake"
        "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules"
        )

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Use folders to organize targets in an IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

if (NOT WIN32)
    message(FATAL_ERROR "Only support Windows.")
endif ()

# Use relative paths
if (WIN32)
    set(CMAKE_USE_RELATIVE_PATHS TRUE)
    set(CMAKE_SUPPRESS_REGENERATION TRUE)
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as no build type was specified")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type (Debug/Release)" FORCE)
endif ()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "The root directory of the installation" FORCE)
    message(STATUS "Setting default install directory to ${CMAKE_INSTALL_PREFIX} as no install directory was specified")
endif ()

# Generate a CompilationDatabase (compile_commands.json)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(VIRTOOLS_SDK_PATH "${VIRTOOLS_SDK_PATH}" CACHE PATH "Path to the Virtools SDK")
set(VIRTOOLS_SDK_FETCH_FROM_GIT "${VIRTOOLS_SDK_FETCH_FROM_GIT}" CACHE BOOL "Set to ON to fetch copy of SDK from git if not otherwise locatable")
set(VIRTOOLS_SDK_FETCH_FROM_GIT_PATH "${VIRTOOLS_SDK_FETCH_FROM_GIT_PATH}" CACHE FILEPATH "location to download SDK")

if (NOT VIRTOOLS_SDK_PATH)
    if (NOT VIRTOOLS_SDK_FETCH_FROM_GIT)
        message(FATAL_ERROR "Virtools SDK location was not specified. Please set VIRTOOLS_SDK_PATH or set VIRTOOLS_SDK_FETCH_FROM_GIT to on to fetch from git.")
    else ()
        include(FetchContent)
        set(FETCHCONTENT_BASE_DIR_SAVE ${FETCHCONTENT_BASE_DIR})
        if (VIRTOOLS_SDK_FETCH_FROM_GIT_PATH)
            get_filename_component(FETCHCONTENT_BASE_DIR "${VIRTOOLS_SDK_FETCH_FROM_GIT_PATH}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
        endif ()
        FetchContent_Declare(
                Virtools_SDK
                GIT_REPOSITORY https://github.com/doyaGu/Virtools-SDK-2.1.git
                GIT_TAG main
        )
        if (NOT Virtools_SDK)
            message("Downloading Virtools SDK")
            FetchContent_Populate(Virtools_SDK)
            set(VIRTOOLS_SDK_PATH "${virtools_sdk_SOURCE_DIR}" CACHE PATH "Path to the Virtools SDK" FORCE)
        endif ()
        set(FETCHCONTENT_BASE_DIR ${FETCHCONTENT_BASE_DIR_SAVE})
    endif ()
endif ()

find_package(VirtoolsSDK REQUIRED HINTS ${VIRTOOLS_SDK_PATH})

configure_file(ImGuiManager.rc.in "${CMAKE_CURRENT_LIST_DIR}/ImGuiManager.rc")

add_compile_definitions(
        $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
        $<$<C_COMPILER_ID:MSVC>:_CRT_NONSTDC_NO_WARNINGS>
        $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE>
        $<$<C_COMPILER_ID:MSVC>:_CRT_NONSTDC_NO_DEPRECATE>
)

# ImGui
add_library(ImGui SHARED
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_ck2.cpp
        imconfig.h
        imgui/imgui.h
        imgui/imgui_internal.h
        imgui/imstb_rectpack.h
        imgui/imstb_textedit.h
        imgui/imstb_truetype.h
        imgui/backends/imgui_impl_ck2.h
        ImGui.rc
        )
target_include_directories(ImGui PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )
target_link_libraries(ImGui PRIVATE CK2 VxMath)
target_compile_definitions(ImGui PRIVATE VX_API)

# ImGuiManager
add_library(ImGuiManager SHARED ImGuiManager.cpp ImGuiManager.h ImGuiManager.rc)
target_link_libraries(ImGuiManager PRIVATE ImGui CK2 VxMath)
target_compile_definitions(ImGui PRIVATE VX_API)

# CKImGui
add_library(CKImGui STATIC CKImGui.cpp CKImGui.h)
target_link_libraries(CKImGui PRIVATE CK2 VxMath)
target_compile_definitions(CKImGui PRIVATE VX_API)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/CMake/ImGuiManagerConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ImGuiManagerConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ImGuiManager"
        )

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/ImGuiManagerConfigVersion.cmake"
        COMPATIBILITY SameMinorVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ImGuiManagerConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ImGuiManager"
        )

install(TARGETS ImGui ImGuiManager CKImGui
        EXPORT ImGuiManagerTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )

install(EXPORT "ImGuiManagerTargets"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ImGuiManager")

install(FILES
        imconfig.h
        imgui/imgui.h
        ImGuiManager.h
        CKImGui.h
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")