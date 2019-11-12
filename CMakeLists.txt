cmake_minimum_required(VERSION 3.1)
project(dlsynth VERSION 0.4.0
                DESCRIPTION "DLS Synth library"
                LANGUAGES C CXX)

add_library(dlsynth "")
add_library(libdmusic::dlsynth ALIAS dlsynth)

include(GenerateExportHeader)
generate_export_header(dlsynth)

set(DLSYNTH_ENABLE_SIMD "OFF" CACHE STRING "The SIMD instruction set to use")
option(DLSYNTH_BIGENDIAN "Whether the target architecture is big endian" OFF)

target_sources(dlsynth
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/Decoders/FloatDecoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Decoders/LogarithmicDecoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Decoders/MsAdpcmDecoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Decoders/PcmDecoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Synth/DisableDenormals.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Synth/EG.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Synth/LFO.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Synth/ModMatrix.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Synth/Synthesizer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Synth/Transform.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Synth/Voice.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Synth/VoiceMessage.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Articulator.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/DecoderTable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/dlsynth.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Error.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/ExpressionParser.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Info.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Instrument.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Region.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Sound.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Wave.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Wavesample.cpp
)

target_compile_features(dlsynth PRIVATE cxx_std_17)
target_include_directories(dlsynth
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

if(DLSYNTH_BIGENDIAN)
    target_compile_definitions(dlsynth
        PRIVATE
            DLSYNTH_BIGENDIAN=1
    )
endif()

if(DLSYNTH_ENABLE_SIMD STREQUAL "SSE")
    target_compile_definitions(dlsynth PRIVATE DLSYNTH_USE_SSE=1)
    if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL "4")
        target_compile_options(dlsynth PUBLIC /arch:SSE)
    endif()

    if(NOT MSVC)
        target_compile_options(dlsynth PUBLIC -msse)
    endif()
elseif(DLSYNTH_ENABLE_SIMD STREQUAL "SSE2")
    target_compile_definitions(dlsynth PRIVATE DLSYNTH_USE_SSE=2)
    if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL "4")
        target_compile_options(dlsynth PUBLIC /arch:SSE2)
    endif()

    if(NOT MSVC)
        target_compile_options(dlsynth PUBLIC -msse2)
    endif()
elseif(DLSYNTH_ENABLE_SIMD STREQUAL "SSE3")
    target_compile_definitions(dlsynth PRIVATE DLSYNTH_USE_SSE=3)
    if(MSVC)
        target_compile_options(dlsynth PUBLIC /arch:SSE3)
    endif()

    if(NOT MSVC)
        target_compile_options(dlsynth PUBLIC -msse3)
    endif()
endif()

find_program(GIT_EXECUTABLE "git")
if(NOT GIT_EXECUTABLE STREQUAL "GIT_NOTFOUND" AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse -q HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" status --porcelain
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_EDITS
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT GIT_EDITS STREQUAL "")
        set(GIT_COMMIT "${GIT_COMMIT}-dirty")
    endif()

    target_compile_options(dlsynth PRIVATE -DDLSYNTH_COMMIT="${GIT_COMMIT}")
endif()

find_package(riffcpp 2.2 CONFIG REQUIRED)

target_link_libraries(dlsynth PRIVATE libdmusic::riffcpp)

include(GNUInstallDirs)

# install library files needed for linking
install(
    TARGETS dlsynth
    EXPORT dlsynth-targets
    COMPONENT runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# install the public header files
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT devel
)

install(
    FILES ${PROJECT_BINARY_DIR}/dlsynth_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT devel
)

# installs the *cmake files in share directory
install(
    EXPORT dlsynth-targets
    FILE dlsynth-targets.cmake
    NAMESPACE libdmusic::
    DESTINATION share/dlsynth
    COMPONENT devel
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/cmake/dlsynth-config-version.cmake
    VERSION ${DLSYNTH_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/dlsynth-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/dlsynth-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dlsynth
)

install(
    FILES
        ${CMAKE_BINARY_DIR}/cmake/dlsynth-config.cmake
        ${CMAKE_BINARY_DIR}/cmake/dlsynth-config-version.cmake
    DESTINATION share/dlsynth
    COMPONENT devel
)

#Register package in user's package registry
export(PACKAGE dlsynth)

# Packaging support
set(CPACK_PACKAGE_VENDOR "Francesco Bertolaccini")
set(CPACK_PACKAGE_CONTACT "francesco@bertolaccini.dev")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DLS Synth library")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "libdlsynth")
set(CPACK_DEBIAN_DEVEL_PACKAGE_NAME "libdlsynth-dev")
set(CPACK_DEBIAN_RUNTIME_FILE_NAME libdlsynth_${CPACK_PACKAGE_VERSION}.deb)
set(CPACK_DEBIAN_DEVEL_FILE_NAME libdlsynth-dev_${CPACK_PACKAGE_VERSION}.deb)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.14), libstdc++6 (>= 5.2), libgcc1 (>= 1:3.0), libriffcpp (>= 2.2), libriffcpp (<< 2.3)")
set(CPACK_DEBIAN_DEVEL_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libdlsynth (= ${CPACK_PACKAGE_VERSION})")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)

cpack_add_component(
    runtime
    DISPLAY_NAME "Runtime library"
    REQUIRED
)

cpack_add_component(
    devel
    DISPLAY_NAME "Header files"
    DESCRIPTION "Required if you need to compile programs that depend on this library"
    DEPENDS runtime
)

#include(CTest)
#add_subdirectory(tests)
