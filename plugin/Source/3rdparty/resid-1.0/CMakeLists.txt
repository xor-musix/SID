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

# List of waveform data files
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

# Generate .h files from .dat files using samp2src.pl
foreach(WAVE_FILE ${WAVE_DAT_FILES})
    set(DAT_FILE ${RESID_SOURCE_DIR}/${WAVE_FILE})
    set(H_FILE ${RESID_GENERATED_DIR}/${WAVE_FILE})
    string(REPLACE ".dat" ".h" H_FILE ${H_FILE})
    string(REPLACE ".dat" "" FILE_BASENAME ${WAVE_FILE})
    
    add_custom_command(
        OUTPUT ${H_FILE}
        COMMAND perl ${RESID_SOURCE_DIR}/samp2src.pl ${FILE_BASENAME} ${DAT_FILE} ${H_FILE}
        DEPENDS ${DAT_FILE} ${RESID_SOURCE_DIR}/samp2src.pl
        COMMENT "Converting ${WAVE_FILE} to header format"
    )
    
    list(APPEND GENERATED_HEADERS ${H_FILE})
endforeach()

# Define the resid-1.0 library as a static library
add_library(resid-1.0 STATIC
    ${RESID_SOURCE_DIR}/sid.cc
    ${RESID_SOURCE_DIR}/voice.cc
    ${RESID_SOURCE_DIR}/wave.cc
    ${RESID_SOURCE_DIR}/envelope.cc
    ${RESID_SOURCE_DIR}/filter.cc
    ${RESID_SOURCE_DIR}/extfilt.cc
    ${RESID_SOURCE_DIR}/pot.cc
    ${RESID_SOURCE_DIR}/dac.cc
    ${RESID_SOURCE_DIR}/version.cc
    ${RESID_SOURCE_DIR}/filter8580new.cc
    ${GENERATED_HEADERS}
)

# Set generated headers as source group
source_group(TREE ${RESID_GENERATED_DIR} PREFIX Generated FILES ${GENERATED_HEADERS})

# Add include directories
target_include_directories(resid-1.0 PUBLIC
    ${RESID_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/resid-1.0/generated
)

# Set C++ standard
target_compile_features(resid-1.0 PUBLIC cxx_std_17)

target_compile_definitions(resid-1.0 PRIVATE
    VERSION="1.0.0"
    RESID_INLINING=1
)