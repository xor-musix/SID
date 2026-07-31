cmake_minimum_required (VERSION 3.24.0 FATAL_ERROR)

#
# Set for each plugin
#
set (PLUGIN_NAME SID)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PLUGIN_VERSION)
string(STRIP "${PLUGIN_VERSION}" PLUGIN_VERSION)
set (BUNDLE_ID com.socalabs.SID)
set (AU_ID SIDAU)
set (LV2_URI https://socalabs.com/sid/)
set (PLUGIN_CODE Sidd)

set (CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set (CMAKE_SUPPRESS_REGENERATION true)
# Skip install rules everywhere except Linux, where CPack/DEB needs them.
if (NOT (UNIX AND NOT APPLE))
	set (CMAKE_SKIP_INSTALL_RULES YES)
endif()
set_property (GLOBAL PROPERTY DEBUG_CONFIGURATIONS "Debug")

set (CMAKE_C_FLAGS_DEVELOPMENT ${CMAKE_C_FLAGS_RELEASE})
set (CMAKE_CXX_FLAGS_DEVELOPMENT ${CMAKE_CXX_FLAGS_RELEASE})

project (${PLUGIN_NAME} VERSION ${PLUGIN_VERSION} LANGUAGES CXX C HOMEPAGE_URL "https://socalabs.com/")

include (CMakeDependentOption)

set_property (DIRECTORY APPEND PROPERTY LABELS ${PLUGIN_NAME})

set_property (DIRECTORY APPEND PROPERTY LABELS SocaLabs)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PLUGIN_NAME}_Standalone)

set (CMAKE_OSX_DEPLOYMENT_TARGET 10.11)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION ON)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_OBJCXX_STANDARD 20)
set (CMAKE_OBJCXX_STANDARD_REQUIRED ON)
 set (CMAKE_CXX_VISIBILITY_PRESET hidden)
 set (CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set (CMAKE_MINSIZEREL_POSTFIX -rm)
set (CMAKE_RELWITHDEBINFO_POSTFIX -rd)
set (CMAKE_OPTIMIZE_DEPENDENCIES OFF)

set (BUILD_SHARED_LIBS OFF)

if(APPLE)
	set (CMAKE_OSX_ARCHITECTURES arm64 x86_64)
endif()

# AU is an Objective-C++ / macOS-only format; only request it on Apple.
if (APPLE)
	set (FORMATS Standalone VST VST3 AU LV2)
else()
	set (FORMATS Standalone VST VST3 LV2)
endif()

set_property (GLOBAL PROPERTY USE_FOLDERS YES)
set_property (GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER utility)
set_property (GLOBAL PROPERTY REPORT_UNDEFINED_PROPERTIES "${CMAKE_BINARY_DIR}/undefined_properties.log")
set_property (GLOBAL PROPERTY JUCE_COPY_PLUGIN_AFTER_BUILD NO)

set_property (DIRECTORY APPEND PROPERTY LABELS External)

# JUCE

set (JUCE_MODULES_ONLY OFF)
set (JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
set (JUCE_BUILD_EXTRAS OFF)
set (JUCE_BUILD_EXAMPLES OFF)

add_subdirectory (modules/juce)
add_subdirectory (modules/clap-juce-extensions EXCLUDE_FROM_ALL)

set_property (DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/modules/juce" APPEND PROPERTY LABELS JUCE)

#

# Gin modules

foreach(module_name IN ITEMS gin gin_dsp gin_simd gin_graphics gin_gui gin_metadata gin_network gin_plugin gin_webp)
	juce_add_module (
		"${CMAKE_CURRENT_LIST_DIR}/modules/gin/modules/${module_name}"
		)

	set_property (TARGET "${module_name}" APPEND PROPERTY LABELS Gin)
endforeach()

# Binary Data

set_property (DIRECTORY APPEND PROPERTY LABELS Assets)

juce_add_binary_data (${PLUGIN_NAME}_Assets SOURCES
						"plugin/Resources/logo.png"
					 )

set_target_properties(${PLUGIN_NAME}_Assets PROPERTIES UNITY_BUILD ON UNITY_BUILD_MODE BATCH UNITY_BUILD_BATCH_SIZE 50)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Option to select SID emulator library (mutually exclusive)
# Options: 0= resid-0.16 (default), 1= resid-1.0, 2= libresidfp
set(SID_EMULATOR "2" CACHE STRING "SID emulator library to use: 0=resid-0.16 (default), 1=resid-1.0, 2=libresidfp")

# Add SID emulator library based on selection
if (SID_EMULATOR STREQUAL "1")
    # resid-1.0 selected
    add_subdirectory(plugin/Source/3rdparty/resid-1.0)
elseif (SID_EMULATOR STREQUAL "2")
    # libresidfp selected
    set(RESIDFP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Source/3rdparty/resid-fp/libresidfp/src)
    set(RESIDFP_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/plugin/Source/3rdparty/resid-fp/generated)
    
    # Create the output directory if it doesn't exist
    file(MAKE_DIRECTORY ${RESIDFP_BUILD_DIR})
    
    # Configure siddefs-fp.h from siddefs-fp.h.in with default values
    set(RESIDFP_BRANCH_HINTS 0)
    set(HAVE_BUILTIN_EXPECT 1)
    set(RESIDFP_INLINING 1)
    set(RESIDFP_INLINE "inline")
    set(PACKAGE_VERSION "1.1.2")
    
    configure_file(
        ${RESIDFP_SOURCE_DIR}/siddefs-fp.h.in
        ${RESIDFP_BUILD_DIR}/siddefs-fp.h
        @ONLY
    )
    
    # Configure sidversion.h from sidversion.h.in with default values
    set(LIB_MAJOR 1)
    set(LIB_MINOR 1)
    set(LIB_LEVEL 2)
    set(SID_VERSION_STRING "1.1.2")
    
    configure_file(
        ${RESIDFP_SOURCE_DIR}/residfp/sidversion.h.in
        ${RESIDFP_BUILD_DIR}/sidversion.h
        @ONLY
    )
    
    # Add the version string macro to sidversion.h
    file(APPEND ${RESIDFP_BUILD_DIR}/sidversion.h "\n#define SID_VERSION_STRING \"${SID_VERSION_STRING}\"\n")
    
    # Copy all libresidfp headers to build directory
    # This includes SID.h and all its dependencies
    file(COPY ${RESIDFP_SOURCE_DIR}/
        DESTINATION ${RESIDFP_BUILD_DIR}
        FILES_MATCHING PATTERN "*.h"
        PATTERN "residfp" EXCLUDE
    )
    # Copy residfp headers to residfp subdirectory
    file(COPY ${RESIDFP_SOURCE_DIR}/residfp/residfp.h ${RESIDFP_SOURCE_DIR}/residfp/residfp_defs.h DESTINATION ${RESIDFP_BUILD_DIR}/residfp/)
    # Copy the generated sidversion.h to residfp directory
    configure_file(
        ${RESIDFP_BUILD_DIR}/sidversion.h
        ${RESIDFP_BUILD_DIR}/residfp/sidversion.h
        COPYONLY
    )
    
    add_library(resid-fp STATIC
        ${RESIDFP_SOURCE_DIR}/SID.cpp
        ${RESIDFP_SOURCE_DIR}/EnvelopeGenerator.cpp
        ${RESIDFP_SOURCE_DIR}/ExternalFilter.cpp
        ${RESIDFP_SOURCE_DIR}/Filter.cpp
        ${RESIDFP_SOURCE_DIR}/Filter6581.cpp
        ${RESIDFP_SOURCE_DIR}/Filter8580.cpp
        ${RESIDFP_SOURCE_DIR}/FilterModelConfig.cpp
        ${RESIDFP_SOURCE_DIR}/FilterModelConfig6581.cpp
        ${RESIDFP_SOURCE_DIR}/FilterModelConfig8580.cpp
        ${RESIDFP_SOURCE_DIR}/Integrator6581.cpp
        ${RESIDFP_SOURCE_DIR}/Integrator8580.cpp
        ${RESIDFP_SOURCE_DIR}/OpAmp.cpp
        ${RESIDFP_SOURCE_DIR}/Spline.cpp
        ${RESIDFP_SOURCE_DIR}/version.cc
        ${RESIDFP_SOURCE_DIR}/WaveformCalculator.cpp
        ${RESIDFP_SOURCE_DIR}/WaveformGenerator.cpp
        ${RESIDFP_SOURCE_DIR}/Dac.cpp
        ${RESIDFP_SOURCE_DIR}/State.cpp
        ${RESIDFP_SOURCE_DIR}/residfp/residfp.cpp
        ${RESIDFP_SOURCE_DIR}/resample/SincResampler.cpp
    )
    target_include_directories(resid-fp PUBLIC ${RESIDFP_BUILD_DIR})
    target_include_directories(resid-fp PUBLIC ${RESIDFP_SOURCE_DIR})
    target_include_directories(resid-fp PUBLIC ${RESIDFP_SOURCE_DIR}/residfp)
    target_compile_features(resid-fp PUBLIC cxx_std_17)
    set_target_properties(resid-fp PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

juce_set_vst2_sdk_path (${CMAKE_SOURCE_DIR}/modules/plugin_sdk/vstsdk2.4)

juce_add_plugin (${PLUGIN_NAME}
				 PRODUCT_NAME ${PLUGIN_NAME}
				 VERSION ${PLUGIN_VERSION}
				 COMPANY_NAME SocaLabs
				 COMPANY_WEBSITE "https://socalabs.com/"
				 BUNDLE_ID ${BUNDLE_ID}
				 FORMATS ${FORMATS}
				 PLUGIN_MANUFACTURER_CODE Soca
				 PLUGIN_CODE ${PLUGIN_CODE}
				 IS_SYNTH ON
				 NEEDS_MIDI_INPUT ON
				 EDITOR_WANTS_KEYBOARD_FOCUS ON
				 VST2_CATEGORY kPlugCategSynth
				 VST3_CATEGORIES Instrument
				 AU_MAIN_TYPE kAudioUnitType_MusicDevice
				 AU_EXPORT_PREFIX ${AU_ID}
				 AU_SANDBOX_SAFE FALSE
				 LV2URI ${LV2_URI})

clap_juce_extensions_plugin (TARGET ${PLUGIN_NAME}
				 CLAP_ID "${BUNDLE_ID}"
				 CLAP_FEATURES instrument synthesizer chiptune)

file (GLOB_RECURSE source_files CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/*.h)

# Exclude files from resid-1.0 submodule (VICE)
# Exclude files from libresidfp tests
# Exclude residfp wrapper files when not using libresidfp
list(FILTER source_files EXCLUDE REGEX "resid-1.0/vice-git/vice/")
list(FILTER source_files EXCLUDE REGEX "resid-fp/libresidfp/tests/")
list(FILTER source_files EXCLUDE REGEX "resid-fp/libresidfp/src/")
list(FILTER source_files EXCLUDE REGEX "resid-fp/residfp_wrapper\\.cc$")
list(FILTER source_files EXCLUDE REGEX "resid-fp/residfp_wrapper\\.h$")

target_sources (${PLUGIN_NAME} PRIVATE ${source_files})
source_group (TREE ${CMAKE_CURRENT_SOURCE_DIR}/plugin PREFIX Source FILES ${source_files})

# Add residfp wrapper source files when libresidfp is selected
if (SID_EMULATOR STREQUAL "2")
    target_sources (${PLUGIN_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Source/3rdparty/resid-fp/residfp_wrapper.cc
    )
endif()

file (GLOB_RECURSE asset_files CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/Assets/*)

target_sources (${PLUGIN_NAME} PRIVATE ${asset_files})
source_group (TREE ${CMAKE_CURRENT_SOURCE_DIR}/Assets PREFIX Assets FILES ${asset_files})

target_link_libraries (${PLUGIN_NAME} PRIVATE
						${PLUGIN_NAME}_Assets

						gin
						gin_dsp
						gin_simd
						gin_graphics
						gin_gui
						gin_plugin

						juce::juce_audio_basics
						juce::juce_audio_devices
						juce::juce_audio_formats
						juce::juce_audio_plugin_client
						juce::juce_audio_processors
						juce::juce_audio_utils
						juce::juce_core
						juce::juce_cryptography
						juce::juce_data_structures
						juce::juce_events
						juce::juce_graphics
						juce::juce_gui_basics
						juce::juce_gui_extra

    					juce::juce_recommended_config_flags
					)

# Add resid library based on selection
if (SID_EMULATOR STREQUAL "2")
    # libresidfp selected
    target_link_libraries (${PLUGIN_NAME} PRIVATE resid-fp)
    target_compile_definitions (${PLUGIN_NAME} PRIVATE SID_EMULATOR=2)
    target_include_directories (${PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Source/3rdparty/resid-fp)
    target_include_directories (${PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Source/3rdparty/resid-fp/libresidfp/src)
elseif (SID_EMULATOR STREQUAL "1")
    # resid-1.0 selected
    target_link_libraries (${PLUGIN_NAME} PRIVATE resid-1.0)
    target_compile_definitions (${PLUGIN_NAME} PRIVATE SID_EMULATOR=1)
    # Add resid-1.0 build source directory FIRST (for patched headers), then generated directory
    target_include_directories (${PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/plugin/Source/3rdparty/resid-1.0/resid-1.0/build-src/vice/src/resid)
    target_include_directories (${PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/plugin/Source/3rdparty/resid-1.0/generated)
else()
    # resid-0.16 sources are included directly (default)
    target_include_directories (${PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Source/3rdparty/resid-0.16)
endif()

target_include_directories (${PLUGIN_NAME} PRIVATE modules/fmt/include)
target_include_directories (${PLUGIN_NAME} PRIVATE modules/ASIO/common)
target_include_directories (${PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/Definitions)

juce_generate_juce_header (${PLUGIN_NAME})

target_compile_definitions (${PLUGIN_NAME} PRIVATE
								JUCE_DISPLAY_SPLASH_SCREEN=0
								JUCE_MODAL_LOOPS_PERMITTED=1
								JUCE_WEB_BROWSER=0
								JUCE_USE_FLAC=0
								JUCE_USE_CURL=1
								JUCE_USE_MP3AUDIOFORMAT=0
								JUCE_USE_LAME_AUDIO_FORMAT=0
								JUCE_USE_WINDOWS_MEDIA_FORMAT=0
								JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
								# JucePlugin_PreferredChannelConfigurations={0,1}
								_CRT_SECURE_NO_WARNINGS
							)

if (APPLE)
	target_compile_definitions (${PLUGIN_NAME} PRIVATE JUCE_COREGRAPHICS_DRAW_ASYNC=1)

    foreach(t ${FORMATS} "Assets" "All" "")
        set(tgt ${CMAKE_PROJECT_NAME})
        if (NOT t STREQUAL "")
            set(tgt ${tgt}_${t})
        endif()
        if (TARGET ${tgt})
			set_target_properties(${tgt} PROPERTIES
				XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME NO
				#XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] YES
				XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES"
				XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf-with-dsym"
				)
			if (NOT t STREQUAL "All")
				target_compile_options(${tgt} PRIVATE
					-Wall -Wstrict-aliasing -Wunused-parameter -Wconditional-uninitialized -Woverloaded-virtual -Wreorder -Wconstant-conversion -Wbool-conversion -Wextra-semi
					-Wunreachable-code -Winconsistent-missing-destructor-override -Wshift-sign-overflow -Wnullable-to-nonnull-conversion -Wuninitialized -Wno-missing-field-initializers
					-Wno-ignored-qualifiers -Wno-missing-braces -Wno-char-subscripts -Wno-unused-private-field -fno-aligned-allocation -Wunused-private-field -Wunreachable-code
					-Wenum-compare -Wshadow -Wfloat-conversion -Wshadow-uncaptured-local -Wshadow-field -Wsign-compare -Wdeprecated-this-capture -Wimplicit-float-conversion
					-ffast-math -fno-finite-math-only)
			endif()
       	endif()
    endforeach()
endif()

if (WIN32)
    foreach(t ${FORMATS} "Assets" "All" "")
        set(tgt ${CMAKE_PROJECT_NAME})
        if (NOT t STREQUAL "")
            set(tgt ${tgt}_${t})
        endif()
        if (TARGET ${tgt})
			set_property(TARGET ${tgt} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG " /INCREMENTAL:NO")
			set_target_properties(${tgt} PROPERTIES LINK_FLAGS "/ignore:4099")
		endif()
	endforeach()

	# Generate debug symbols (PDBs) for Release builds so crashes can be symbolicated.
	# /Zi emits full debug info; /DEBUG produces the .pdb; /OPT:REF and /OPT:ICF
	# restore the size optimisations that /DEBUG otherwise disables.
	foreach(t ${FORMATS} "CLAP" "")
		set(tgt ${CMAKE_PROJECT_NAME})
		if (NOT t STREQUAL "")
			set(tgt ${tgt}_${t})
		endif()
		if (TARGET ${tgt})
			target_compile_options(${tgt} PRIVATE "$<$<CONFIG:Release>:/Zi>")
			target_link_options(${tgt} PRIVATE "$<$<CONFIG:Release>:/DEBUG>")
			target_link_options(${tgt} PRIVATE "$<$<CONFIG:Release>:/OPT:REF>")
			target_link_options(${tgt} PRIVATE "$<$<CONFIG:Release>:/OPT:ICF>")
		endif()
	endforeach()
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries (${PLUGIN_NAME} PRIVATE curl)
endif()
#
# Install + CPack (Linux only — macOS uses pkgbuild/productbuild, Windows uses Inno Setup)
#
if (UNIX AND NOT APPLE)
	set (_artefact_dir "${CMAKE_BINARY_DIR}/${PLUGIN_NAME}_artefacts/$<CONFIG>")

	install (FILES "${_artefact_dir}/VST/lib${PLUGIN_NAME}.so"
			 DESTINATION lib/vst
			 RENAME "${PLUGIN_NAME}.so"
			 COMPONENT VST)

	install (DIRECTORY "${_artefact_dir}/VST3/${PLUGIN_NAME}.vst3"
			 DESTINATION lib/vst3
			 COMPONENT VST3
			 USE_SOURCE_PERMISSIONS)

	install (DIRECTORY "${_artefact_dir}/LV2/${PLUGIN_NAME}.lv2"
			 DESTINATION lib/lv2
			 COMPONENT LV2
			 USE_SOURCE_PERMISSIONS)

	install (FILES "${_artefact_dir}/CLAP/${PLUGIN_NAME}.clap"
			 DESTINATION lib/clap
			 COMPONENT CLAP)


	# Factory presets ship flat — gin's loadDirectory doesn't recurse.
	file (GLOB _preset_xmls "${CMAKE_CURRENT_SOURCE_DIR}/plugin/Resources/Presets/*.xml")
	install (FILES ${_preset_xmls}
			 DESTINATION share/SocaLabs/${PLUGIN_NAME}/Presets
			 COMPONENT Resources)

	set (CPACK_PACKAGE_NAME "sid")
	set (CPACK_PACKAGE_VERSION "${PLUGIN_VERSION}")
	set (CPACK_PACKAGE_VENDOR "SocaLabs")
	set (CPACK_PACKAGE_HOMEPAGE_URL "https://socalabs.com/")
	set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Commodore 64 SID audio synthesizer plugin")
	set (CPACK_PACKAGE_CONTACT "support@socalabs.com")
	set (CPACK_PACKAGING_INSTALL_PREFIX "/usr")
	set (CPACK_GENERATOR "DEB")
	set (CPACK_DEB_COMPONENT_INSTALL OFF)
	set (CPACK_COMPONENTS_ALL VST VST3 LV2 CLAP Resources)
	set (CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
	set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Roland Rabien <support@socalabs.com>")
	set (CPACK_DEBIAN_PACKAGE_SECTION "sound")
	set (CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	set (CPACK_DEBIAN_FILE_NAME "SID.deb")

	include (CPack)
endif()