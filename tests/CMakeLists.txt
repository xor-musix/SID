cmake_minimum_required(VERSION 3.24.0)

# Test harness for SID plugin
# Compares audio output across different SID engines

set(TEST_NAME SID_tests)

# Set C++20 standard globally (required by JUCE headers)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build test as standalone console application
add_executable(${TEST_NAME}
    tests_main.cpp
    ../plugin/Source/PluginEditor.cpp
    ../plugin/Source/PluginProcessor.cpp
)

# Include test header files
target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Include JuceHeader.h directory - JUCE module headers should be included via JuceHeader.h
# The JUCE module directories are also included so that JuceHeader.h can find the headers
target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../modules/juce/modules)

# Set C++20 standard (required by JUCE headers)
target_compile_features(${TEST_NAME} PUBLIC cxx_std_20)

# Add JUCE compile definitions
target_compile_definitions(${TEST_NAME} PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_FLAC=0
    JUCE_USE_CURL=1
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_LAME_AUDIO_FORMAT=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
    _CRT_SECURE_NO_WARNINGS
)

# Note: The test harness needs JUCE headers and gin headers (for plugin files),
# but doesn't need to link to gin modules since we're using standard JUCE types
target_include_directories(${TEST_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/gin/modules
)

# Add dependency on main plugin targets to ensure headers and binary data are generated first
if(TARGET SID)
    add_dependencies(${TEST_NAME} SID)
endif()
if(TARGET SID_Assets)
    # Ensure SID_Assets is built before the test target
    add_dependencies(${TEST_NAME} SID_Assets)
    
    # Copy BinaryData.h to the correct location (JuceHeader.h expects it in SID_artefacts/JuceLibraryCode)
    # Create a custom target that copies the file after SID_Assets is built
    add_custom_target(SID_tests_copy_assets DEPENDS ${CMAKE_BINARY_DIR}/SID_artefacts/JuceLibraryCode/BinaryData.h)
    add_dependencies(SID_tests_copy_assets SID_Assets)
    
    # Add command to copy the file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/SID_artefacts/JuceLibraryCode/BinaryData.h
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/juce_binarydata_SID_Assets/JuceLibraryCode/BinaryData.h
            ${CMAKE_BINARY_DIR}/SID_artefacts/JuceLibraryCode/BinaryData.h
        DEPENDS ${CMAKE_BINARY_DIR}/juce_binarydata_SID_Assets/JuceLibraryCode/BinaryData.h
        COMMENT "Copying BinaryData.h to correct location"
    )
    
    # Make test target depend on the copy target
    add_dependencies(${TEST_NAME} SID_tests_copy_assets)
endif()

# Link to the main SID library to get the implementation of SIDAudioProcessor
# Note: We do NOT link to gin because it would compile gin source files which have missing includes
# Instead, we just need the include paths which are provided by juce_add_module in the main CMakeLists.txt
target_link_libraries(${TEST_NAME} PRIVATE
    SID
)

# Add SID library based on selection
if(SID_EMULATOR STREQUAL "2")
    # libresidfp selected
    target_link_libraries(${TEST_NAME} PRIVATE resid-fp)
    target_compile_definitions(${TEST_NAME} PRIVATE SID_EMULATOR=2)
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../plugin/Source/3rdparty/resid-fp)
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../plugin/Source/3rdparty/resid-fp/libresidfp/src)
elseif(SID_EMULATOR STREQUAL "1")
    # resid-1.0 selected
    target_link_libraries(${TEST_NAME} PRIVATE resid-1.0)
    target_compile_definitions(${TEST_NAME} PRIVATE SID_EMULATOR=1)
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_BINARY_DIR}/plugin/Source/3rdparty/resid-1.0/resid-1.0/build-src/vice/src/resid)
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_BINARY_DIR}/plugin/Source/3rdparty/resid-1.0/generated)
endif()

# Include plugin headers
target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../plugin/Source)
# JuceHeader.h is in the SID_artefacts/JuceLibraryCode directory
target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_BINARY_DIR}/SID_artefacts/JuceLibraryCode)

# Output directory
set_target_properties(${TEST_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Set as standalone application
target_compile_definitions(${TEST_NAME} PRIVATE JUCE_STANDALONE_APPLICATION=1)

if(APPLE)
    target_compile_definitions(${TEST_NAME} PRIVATE JUCE_COREGRAPHICS_DRAW_ASYNC=1)
endif()