cmake_minimum_required(VERSION 3.12)

project(CKImGui VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED NO)
set(CMAKE_CXX_EXTENSIONS YES)

# Add path for custom modules
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if (NOT WIN32)
    message(FATAL_ERROR "Only support Windows.")
endif ()

# Use relative paths
if (WIN32)
    set(CMAKE_USE_RELATIVE_PATHS TRUE)
    set(CMAKE_SUPPRESS_REGENERATION TRUE)
endif ()

# Use folders to organize targets in an IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as no build type was specified")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type (Debug/Release)" FORCE)
endif ()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "The root directory of the installation" FORCE)
    message(STATUS "Setting default install directory to ${CMAKE_INSTALL_PREFIX} as no install directory was specified")
endif ()

# Disable msvc unsafe warnings
if (MSVC)
    add_compile_definitions(
            $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
            $<$<C_COMPILER_ID:MSVC>:_CRT_NONSTDC_NO_WARNINGS>
    )
endif ()

# Generate a CompilationDatabase (compile_commands.json)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

find_package(VirtoolsSDK REQUIRED)

configure_file(${CMAKE_CURRENT_LIST_DIR}/ImGui.rc.in ${CMAKE_CURRENT_LIST_DIR}/ImGui.rc)

set(IMGUI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

set(IMGUI_HEADERS
        ${IMGUI_SOURCE_DIR}/imgui.h
        ${IMGUI_SOURCE_DIR}/imgui_internal.h
        ${IMGUI_SOURCE_DIR}/imstb_rectpack.h
        ${IMGUI_SOURCE_DIR}/imstb_textedit.h
        ${IMGUI_SOURCE_DIR}/imstb_truetype.h
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_win32.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_ck2.h
        )

set(IMGUI_SOURCES
        ${IMGUI_SOURCE_DIR}/imgui.cpp
        ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
        ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
        ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
        ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_ck2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Handler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ImGui.rc
        )

add_library(ImGui SHARED)
target_sources(ImGui PRIVATE ${IMGUI_SOURCES} ${IMGUI_HEADERS})
target_include_directories(ImGui PUBLIC
        $<BUILD_INTERFACE:${IMGUI_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )
target_link_libraries(ImGui PRIVATE CK2 VxMath)
target_compile_definitions(ImGui PRIVATE IMGUI_EXPORT)
set_target_properties(ImGui PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
        )

add_custom_command(
        TARGET ImGui PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h" "${IMGUI_SOURCE_DIR}"
        VERBATIM
)

if (NOT SKIP_INSTALL_ALL)
    configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/CMake/ImGuiConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/ImGuiConfig.cmake"
            INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ImGui"
            )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/ImGuiConfigVersion.cmake"
            COMPATIBILITY SameMinorVersion
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/ImGuiConfig.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ImGui"
            )

    install(TARGETS ImGui
            EXPORT ImGuiTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )

    install(EXPORT ImGuiTargets
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ImGui")

    if (NOT SKIP_INSTALL_HEADERS)
        install(FILES
                ${IMGUI_SOURCE_DIR}/imconfig.h
                ${IMGUI_SOURCE_DIR}/imgui.h
                ${IMGUI_SOURCE_DIR}/imgui_internal.h
                ${IMGUI_SOURCE_DIR}/imstb_textedit.h
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif ()
endif ()
	
