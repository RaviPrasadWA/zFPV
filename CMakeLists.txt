cmake_minimum_required(VERSION 3.16.3)
project(Wifibroadcast)

cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-address-use-after-scope -fsanitize=address")
find_package(PkgConfig REQUIRED)

# Get spdlog from package manager for those tests
set(WB_USE_SPDLOG_EXTERNALLY OFF)
option(ENABLE_USB_CAMERAS "USB cameras support" OFF)

pkg_search_module(GST REQUIRED
            gstreamer-1.0>=1.4
            gstreamer-app-1.0>=1.4
    )
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)

#----------------------------------------------------------------------------------------------------------------------
# Dependencies: spdlog and nlohmann::json
#----------------------------------------------------------------------------------------------------------------------
set(SPDLOG_MASTER_PROJECT OFF)
set(SPDLOG_PROJECT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/executables/spdlog)
set(SPDLOG_SOURCES_DIRECTORY ${SPDLOG_PROJECT_DIRECTORY}/src)

set(spdlog_sources
    "${SPDLOG_SOURCES_DIRECTORY}/bundled_fmtlib_format.cpp"
)

# Create the SpdlogLIB target
add_library(SpdlogLIB executables/spdlog.cpp)
target_precompile_headers(SpdlogLIB PRIVATE ${SPDLOG_PROJECT_DIRECTORY}/include/spdlog/spdlog.h)
target_include_directories(SpdlogLIB PUBLIC ${SPDLOG_PROJECT_DIRECTORY}/include)

# Add nlohmann::json
add_subdirectory(executables/json)
# target_link_libraries(JsonRWLib PUBLIC nlohmann_json::nlohmann_json)

#----------------------------------------------------------------------------------------------------------------------
# Sources
#----------------------------------------------------------------------------------------------------------------------
set(sources
    "executables/utils.cpp"
    "executables/spdlog.cpp"
    "executables/profile.cpp"
    "executables/camera_enums.hpp"
    "executables/camera_settings.hpp"
    "executables/camera.hpp"
    "executables/ini_config.cpp"
    "executables/util_filesystem.cpp"
    "executables/wifi_card_discovery.cpp"
    "executables/wb_link_helper.cpp"
    "executables/util_time.cpp"
    "executables/settings_imp.cpp"
    "executables/action_handler.cpp"
    "executables/util_async.cpp"
    "executables/reboot_util.cpp"
    "executables/bitrate.cpp"
    "executables/wb_link_settings.cpp"
    "executables/platform.cpp"
    "executables/settings_directories.cpp"
    "executables/wifi_command_helper.cpp"
    "executables/wb_link_manager.cpp"
    "executables/wb_link.cpp"
    "executables/wb_interface.cpp"
    "executables/usb_thermal_cam_helper.cpp"
    "executables/validate_camera_settings.cpp"
    "executables/camera_holder.cpp"
    "executables/camera_discovery.cpp"
    "executables/camerastream.cpp"
    "executables/video_air_generic_settings.cpp"
    "executables/external_device.cpp"
    "executables/air_recording_helper.hpp"
    "executables/gst_bitrate_control_wrapper.hpp"
    "executables/udp.cpp"
    "executables/rtp.cpp"
    "executables/rtp_eof_helper.cpp"
    "executables/gst_recording_demuxer.cpp"
    "executables/gstreamstreamer.cpp"
    "executables/gstaudiostream.cpp"
    "executables/video_air.cpp"
)

list(APPEND sources
        executables/librtp/src/rtp-h264-pack.c
        executables/librtp/src/rtp-h265-pack.c
        executables/librtp/src/rtp-packet.c
        executables/librtp/src/rtp-payload.c
        executables/librtp/src/rtp-h264-bitstream.c
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/executables/librtp/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/executables/librtp/src
)

file(GLOB LIBRTP_SRC "executables/librtp/src/*.c")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

#
# Defines WB_TARGET_LINK_LIBRARIES
# which can then be used to integrate wifibroadcast into your project

option(WB_ENABLE_SIMD_OPTIMIZATIONS "Enable NEON on ARM and SSSE3 on X86 if the platform/compiler supports it" ON)
option(WB_USE_SPDLOG_EXTERNALLY "Do not find and link to spdlog installed on system" ON)
#if(WIFIBROADCAST_LIBRARIES_ALREADY_BUILD)
#if(get_property(source_list GLOBAL PROPERTY source_list_property))
#    message(STATUS "WIFIBROADCAST_LIBRARIES_ALREADY_BUILD")
#    return()
#endif()
if (TARGET wifibroadcast)
    message(STATUS "WIFIBROADCAST_LIBRARIES_ALREADY_BUILD")
    return()
endif()

add_library(wifibroadcast STATIC) # initialized below
add_library(wifibroadcast::wifibroadcast ALIAS wifibroadcast)
#target_compile_options(wifibroadcast INTERFACE -Wno-address-of-packed-member -Wno-cast-align)

# the stuff in lib is a bit extra
target_sources(wifibroadcast PRIVATE
        # radiotap and fec
        ${CMAKE_CURRENT_LIST_DIR}/lib/radiotap/radiotap.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/fec/fec_base.cpp
)

# Well, let's just build everything together
target_sources(wifibroadcast PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/encryption/KeyPair.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/encryption/Encryptor.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/encryption/Encryption.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/encryption/EncryptionFsUtils.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/encryption/Decryptor.cpp
        ##
        ${CMAKE_CURRENT_LIST_DIR}/src/dummy_link/DummyLink.cpp

        ${CMAKE_CURRENT_LIST_DIR}/src/fec/FEC.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/fec/FECConstants.hpp
        ${CMAKE_CURRENT_LIST_DIR}/src/fec/FECDecoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/fec/FECEncoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/fec/RxBlock.cpp

        ${CMAKE_CURRENT_LIST_DIR}/src/WBStreamRx.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/WBStreamTx.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/WBVideoStreamTx.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/WBTxRx.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/FunkyQueue.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/wifibroadcast_spdlog.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/radiotap/RadiotapRxRfAggregator.cpp
)

target_include_directories(wifibroadcast PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/HelperSources
)

## FEC Optimizations begin ---------------------------------
set(WIFIBROADCAST_FEC_OPTIMIZATION_FLAGS_X86_SSSE3 -mssse3 -faligned-new=256)
set(WIFIBROADCAST_FEC_OPTIMIZATION_FLAGS_ARM_NEON -mfpu=neon -march=armv7-a -marm)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mssse3" COMPILER_SUPPORTS_X86_SSSE3)
# ARMv7 might not support neon - we have to ask (and set some flags)
check_cxx_compiler_flag("-mfpu=neon" COMPILER_SUPPORTS_NEON)
# ARMv8 always supports ASIMD which is a superset of NEON. No flags needed, just compile.
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    set(COMPILER_SUPPORTS_ASIMD true)
else ()
    set(COMPILER_SUPPORTS_ASIMD false)
endif ()


# SSSE3 if supported and option WB_ENABLE_SIMD_OPTIMIZATIONS is true
if(COMPILER_SUPPORTS_X86_SSSE3)
    message(STATUS "Compiler supports SSSE3")
    if(WB_ENABLE_SIMD_OPTIMIZATIONS)
        message(STATUS "WB compile with SSSE3")
        target_compile_options(wifibroadcast PUBLIC ${WIFIBROADCAST_FEC_OPTIMIZATION_FLAGS_X86_SSSE3})
        # I do not know why, but using target_compile_options seems to be not enough ...
        add_compile_options(${WIFIBROADCAST_FEC_OPTIMIZATION_FLAGS_X86_SSSE3})
        target_compile_definitions(wifibroadcast PUBLIC WIFIBROADCAST_HAS_X86_SSSE3)
    endif()
endif()
# NEON if supported and option WB_ENABLE_SIMD_OPTIMIZATIONS is true
if(COMPILER_SUPPORTS_NEON)
    message(STATUS "Compiler supports NEON")
    if(WB_ENABLE_SIMD_OPTIMIZATIONS)
        message(STATUS "WB compile with NEON")
        target_compile_options(wifibroadcast PUBLIC ${WIFIBROADCAST_FEC_OPTIMIZATION_FLAGS_ARM_NEON})
        # I do not know why, but using target_compile_options seems to be not enough ...
        add_compile_options(${WIFIBROADCAST_FEC_OPTIMIZATION_FLAGS_ARM_NEON})
        target_compile_definitions(wifibroadcast PUBLIC WIFIBROADCAST_HAS_ARM_NEON)
    endif()
endif()
if(COMPILER_SUPPORTS_ASIMD)
    message(STATUS "Compiler supports ASIMD")
    if(WB_ENABLE_SIMD_OPTIMIZATIONS)
        message(STATUS "WB compile with ASIMD")
        target_compile_definitions(wifibroadcast PUBLIC WIFIBROADCAST_HAS_ARM_NEON)
    endif ()
endif ()
## FEC Optimizations end ---------------------------------

# We need pcap and libsodium to build wifibroadcast
include(${CMAKE_CURRENT_LIST_DIR}/cmake/FindPCAP.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/FindSodium.cmake)

target_include_directories(wifibroadcast PUBLIC ${sodium_INCLUDE_DIR})
target_include_directories(wifibroadcast PUBLIC ${PCAP_INCLUDE_DIR})
target_link_libraries(wifibroadcast PUBLIC ${PCAP_LIBRARY})
target_link_libraries(wifibroadcast PUBLIC ${sodium_LIBRARY_RELEASE})

# for some reason, we also need to manually link pthread
find_package(Threads REQUIRED)
target_link_libraries(wifibroadcast PUBLIC Threads::Threads)
# spdlog might be already exist as a target in OpenHD - only use package manager's spdlog if needed
if(WB_USE_SPDLOG_EXTERNALLY)
    message(STATUS "spdlog needs to be already provided by top cmake")
    # LOL- In openhd we build spdlog into OHDCommonLib and get it from there
    # There were some weird issues with using their cmake and buildroot
    target_link_libraries(wifibroadcast PRIVATE OHDCommonLib)
else ()
    message(STATUS "Using spdlog from package manager")
    find_package(spdlog REQUIRED)
    target_link_libraries(wifibroadcast PRIVATE spdlog::spdlog)
endif ()


SET(WB_TARGET_LINK_LIBRARIES wifibroadcast)
SET(SPDLOG_LINK_LIBRARIES SpdlogLIB)
# SET(WB_INCLUDE_DIRECTORES ${CMAKE_CURRENT_LIST_DIR}/wifibroadcast)

add_executable(wb_link_tx_impv executables/wb_link_tx_impv.cpp 
    ${LIBRTP_SRC}
    executables/spdlog.cpp 
    executables/profile.cpp
    executables/utils.cpp
    executables/camera_enums.hpp
    executables/camera_settings.hpp
    executables/camera.hpp
    executables/ini_config.cpp
    executables/util_filesystem.cpp
    executables/platform.cpp
    executables/wb_link_helper.cpp
    executables/util_time.cpp
    executables/action_handler.cpp
    executables/settings_imp.cpp
    executables/wb_link_settings.cpp
    executables/util_async.cpp
    executables/reboot_util.cpp
    executables/bitrate.cpp
    executables/wifi_card_discovery.cpp
    executables/settings_directories.cpp
    executables/wifi_command_helper.cpp
    executables/wb_link_manager.cpp
    executables/wb_link.cpp
    executables/wb_interface.cpp
    executables/usb_thermal_cam_helper.cpp
    executables/validate_camera_settings.cpp
    executables/camera_holder.cpp
    executables/camera_discovery.cpp
    executables/camerastream.cpp
    executables/video_air_generic_settings.cpp
    executables/external_device.cpp
    executables/udp.cpp
    executables/rtp.cpp
    executables/rtp_eof_helper.cpp
    executables/gst_bitrate_control_wrapper.hpp
    executables/air_recording_helper.hpp
    executables/gst_recording_demuxer.cpp
    executables/gstreamstreamer.cpp
    executables/gstaudiostream.cpp
    executables/video_air.cpp)

target_include_directories(wb_link_tx_impv PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/executables
    ${CMAKE_CURRENT_SOURCE_DIR}/executables/librtp/inc
)
target_link_libraries(wb_link_tx_impv PRIVATE ${WB_TARGET_LINK_LIBRARIES} ${SPDLOG_LINK_LIBRARIES} nlohmann_json::nlohmann_json PkgConfig::gstreamer PkgConfig::gstreamer-app)


# add_executable(wb_link_tx executables/wb_link_tx.cpp)
# target_link_libraries(wb_link_tx PRIVATE ${WB_TARGET_LINK_LIBRARIES})

# add_executable(wb_link_rx executables/wb_link_rx.cpp)
# target_link_libraries(wb_link_rx PRIVATE ${WB_TARGET_LINK_LIBRARIES})



