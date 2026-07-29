# Build configuration for resid-1.0
# This builds the reSID library from the VICE submodule
# 
# The VICE submodule is cloned to plugin/Source/3rdparty/resid-1.0/vice-git/
# This CMakeLists.txt is in the main project and references VICE source files.

# Path to VICE submodule source directory
set(RESID_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vice-git/vice/src/resid)

# Configuration variables for siddefs.h.in
set(RESID_INLINING 1)
set(RESID_INLINE "inline")
set(RESID_BRANCH_HINTS 0)
set(HAVE_BOOL 1)
set(HAVE_BUILTIN_EXPECT 1)
set(HAVE_LOG1P 1)
set(NEW_8580_FILTER 0)

# Configure siddefs.h from siddefs.h.in
set(RESID_SIDDEFS_H ${CMAKE_CURRENT_BINARY_DIR}/resid-1.0/generated/siddefs.h)
configure_file(
    ${RESID_SOURCE_DIR}/siddefs.h.in
    ${RESID_SIDDEFS_H}
    @ONLY
)

# Generate waveform header files from .dat files
set(RESID_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/resid-1.0/generated)

# Create the output directory if it doesn't exist
file(MAKE_DIRECTORY ${RESID_GENERATED_DIR})

# Copy source files to build directory and apply patch
# This keeps the submodule clean and avoids modifying it directly
# We need to create the vice/src/resid directory structure for the patch to apply correctly
set(RESID_BUILD_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/resid-1.0/build-src/vice/src/resid)
file(MAKE_DIRECTORY ${RESID_BUILD_SRC_DIR})

# List of source files to copy
set(RESID_SOURCE_FILES
    sid.cc
    voice.cc
    wave.cc
    envelope.cc
    filter.cc
    extfilt.cc
    pot.cc
    dac.cc
    version.cc
    filter8580new.cc
)

# List of header files to copy
set(RESID_HEADER_FILES
    sid.h
    voice.h
    wave.h
    envelope.h
    filter.h
    extfilt.h
    pot.h
    dac.h
    resid-config.h
    spline.h
    filter8580new.h
)

# Copy all source files to build directory
foreach(SRC_FILE ${RESID_SOURCE_FILES})
    configure_file(${RESID_SOURCE_DIR}/${SRC_FILE} ${RESID_BUILD_SRC_DIR}/${SRC_FILE} COPYONLY)
endforeach()

# Copy all header files to build directory
foreach(HDR_FILE ${RESID_HEADER_FILES})
    configure_file(${RESID_SOURCE_DIR}/${HDR_FILE} ${RESID_BUILD_SRC_DIR}/${HDR_FILE} COPYONLY)
endforeach()

# Copy waveform data files
set(WAVE_DAT_FILES
    wave6581__ST.dat
    wave6581_P_T.dat
    wave6581_PS_.dat
    wave6581_PST.dat
    wave8580__ST.dat
    wave8580_P_T.dat
    wave8580_PS_.dat
    wave8580_PST.dat
)

foreach(WAVE_FILE ${WAVE_DAT_FILES})
    configure_file(${RESID_SOURCE_DIR}/${WAVE_FILE} ${RESID_BUILD_SRC_DIR}/${WAVE_FILE} COPYONLY)
endforeach()

# Copy helper script
configure_file(${RESID_SOURCE_DIR}/samp2src.pl ${RESID_BUILD_SRC_DIR}/samp2src.pl COPYONLY)

# Apply the regToCutoff patch for resid-0.16 compatibility
set(PATCH_FILE ${CMAKE_CURRENT_SOURCE_DIR}/regtocutoff.patch)
set(WORK_DIR ${CMAKE_CURRENT_BINARY_DIR}/resid-1.0/build-src/vice/src)
if(EXISTS ${PATCH_FILE})
    message(STATUS "Applying regToCutoff patch to resid-1.0 build sources")
    execute_process(
        COMMAND patch -p1
        WORKING_DIRECTORY ${WORK_DIR}
        RESULT_VARIABLE PATCH_RESULT
        OUTPUT_VARIABLE PATCH_OUTPUT
        ERROR_VARIABLE PATCH_ERROR
        INPUT_FILE ${PATCH_FILE}
    )
    if(NOT PATCH_RESULT EQUAL 0)
        message(WARNING "Failed to apply regToCutoff patch:\n${PATCH_ERROR}")
    else()
        message(STATUS "Successfully applied regToCutoff patch")
    endif()
else()
    message(WARNING "Patch file not found: ${PATCH_FILE}")
endif()

# Generate .h files from .dat files using samp2src.pl
foreach(WAVE_FILE ${WAVE_DAT_FILES})
    set(DAT_FILE ${RESID_BUILD_SRC_DIR}/${WAVE_FILE})
    set(H_FILE ${RESID_GENERATED_DIR}/${WAVE_FILE})
    string(REPLACE ".dat" ".h" H_FILE ${H_FILE})
    string(REPLACE ".dat" "" FILE_BASENAME ${WAVE_FILE})
    
    add_custom_command(
        OUTPUT ${H_FILE}
        COMMAND perl ${RESID_BUILD_SRC_DIR}/samp2src.pl ${FILE_BASENAME} ${DAT_FILE} ${H_FILE}
        DEPENDS ${DAT_FILE} ${RESID_BUILD_SRC_DIR}/samp2src.pl
        COMMENT "Converting ${WAVE_FILE} to header format"
    )
    
    list(APPEND GENERATED_HEADERS ${H_FILE})
endforeach()

# Define the resid-1.0 library as a static library using patched sources
add_library(resid-1.0 STATIC
    ${RESID_BUILD_SRC_DIR}/sid.cc
    ${RESID_BUILD_SRC_DIR}/voice.cc
    ${RESID_BUILD_SRC_DIR}/wave.cc
    ${RESID_BUILD_SRC_DIR}/envelope.cc
    ${RESID_BUILD_SRC_DIR}/filter.cc
    ${RESID_BUILD_SRC_DIR}/extfilt.cc
    ${RESID_BUILD_SRC_DIR}/pot.cc
    ${RESID_BUILD_SRC_DIR}/dac.cc
    ${RESID_BUILD_SRC_DIR}/version.cc
    ${RESID_BUILD_SRC_DIR}/filter8580new.cc
    ${RESID_BUILD_SRC_DIR}/resid-config.h
    ${GENERATED_HEADERS}
)

# Set generated headers as source group
source_group(TREE ${RESID_GENERATED_DIR} PREFIX Generated FILES ${GENERATED_HEADERS})

# Add include directories pointing to the build source directory
target_include_directories(resid-1.0 PUBLIC
    ${RESID_BUILD_SRC_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/resid-1.0/generated
)

# Set C++ standard
target_compile_features(resid-1.0 PUBLIC cxx_std_17)

# Enable position-independent code for shared library compatibility
set_target_properties(resid-1.0 PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(resid-1.0 PRIVATE
    VERSION="1.0.0"
    RESID_INLINING=1
)