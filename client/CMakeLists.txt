cmake_minimum_required(VERSION 3.19)
project(xPilot LANGUAGES C CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    if(NOT DEFINED CONAN_LINK_RUNTIME AND DEFINED CONAN_SETTINGS_COMPILER_RUNTIME)
        message(WARNING "Forcing MSVC runtime based on conan profile")
        # this means Conan is not running the build, so we need to fix our linker flags ourself.... although we can probably cheat...
        set(CONAN_LINK_RUNTIME "/${CONAN_SETTINGS_COMPILER_RUNTIME}")
        conan_set_vs_runtime()
    endif()
endif(MSVC)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(READ ${CMAKE_SOURCE_DIR}/../xpilot.json XPILOT_JSON)

string(JSON VERSION_URL GET ${XPILOT_JSON} version_url)
string(JSON IS_BETA_VERSION GET ${XPILOT_JSON} is_beta)
string(JSON BETA_NUMBER GET ${XPILOT_JSON} beta_number)
string(JSON VERSION_MAJOR GET ${XPILOT_JSON} version major)
string(JSON VERSION_MINOR GET ${XPILOT_JSON} version minor)
string(JSON VERSION_PATCH GET ${XPILOT_JSON} version patch)

if (IS_BETA_VERSION)
    set(IS_BETA_VERSION true)
else()
    set(IS_BETA_VERSION false)
endif()

if (NOT VATSIM_CLIENT_ID)
    set(VATSIM_CLIENT_ID 0)
endif ()

if (NOT VATSIM_CLIENT_KEY)
    set(VATSIM_CLIENT_KEY "")
endif ()

if (NOT CONFIG_ENCRYPTION_KEY)
    set(CONFIG_ENCRYPTION_KEY 0)
endif ()

if(NOT SENTRY_DSN)
    set(SENTRY_DSN "")
endif ()

configure_file(src/common/build_config.cpp.in ${CMAKE_BINARY_DIR}/generated/build_config.cpp)
include_directories(${CMAKE_BINARY_DIR}/generated)

configure_file(xpilot.rc.in ${CMAKE_SOURCE_DIR}/xpilot.rc @ONLY)

find_package(QT NAMES Qt5 COMPONENTS Core Quick Network Svg REQUIRED)
find_package(Qt5 COMPONENTS Core Quick Network Svg REQUIRED)
find_package(Qt5QuickCompiler)

qtquick_compiler_add_resources(RESOURCES qml.qrc)

set(FSD_SOURCES
    src/fsd/enums.h
    src/fsd/client_properties.h
    src/fsd/fsd_client.cpp
    src/fsd/fsd_client.h
    src/fsd/serializer.h
    src/fsd/serializer.cpp
    src/fsd/pdu/pdu_format_exception.h
    src/fsd/pdu/pdu_base.cpp
    src/fsd/pdu/pdu_base.h
    src/fsd/pdu/pdu_server_identification.cpp
    src/fsd/pdu/pdu_server_identification.h
    src/fsd/pdu/pdu_client_identification.cpp
    src/fsd/pdu/pdu_client_identification.h
    src/fsd/pdu/pdu_atc_position.h
    src/fsd/pdu/pdu_atc_position.cpp
    src/fsd/pdu/pdu_add_atc.h
    src/fsd/pdu/pdu_add_atc.cpp
    src/fsd/pdu/pdu_add_pilot.h
    src/fsd/pdu/pdu_add_pilot.cpp
    src/fsd/pdu/pdu_auth_challenge.h
    src/fsd/pdu/pdu_auth_challenge.cpp
    src/fsd/pdu/pdu_auth_response.h
    src/fsd/pdu/pdu_auth_response.cpp
    src/fsd/pdu/pdu_broadcast_message.h
    src/fsd/pdu/pdu_broadcast_message.cpp
    src/fsd/pdu/pdu_client_query.h
    src/fsd/pdu/pdu_client_query.cpp
    src/fsd/pdu/pdu_client_query_response.h
    src/fsd/pdu/pdu_client_query_response.cpp
    src/fsd/pdu/pdu_delete_atc.h
    src/fsd/pdu/pdu_delete_atc.cpp
    src/fsd/pdu/pdu_delete_pilot.h
    src/fsd/pdu/pdu_delete_pilot.cpp
    src/fsd/pdu/pdu_fast_pilot_position.h
    src/fsd/pdu/pdu_fast_pilot_position.cpp
    src/fsd/pdu/pdu_kill_request.h
    src/fsd/pdu/pdu_kill_request.cpp
    src/fsd/pdu/pdu_metar_request.h
    src/fsd/pdu/pdu_metar_request.cpp
    src/fsd/pdu/pdu_metar_response.h
    src/fsd/pdu/pdu_metar_response.cpp
    src/fsd/pdu/pdu_pilot_position.h
    src/fsd/pdu/pdu_pilot_position.cpp
    src/fsd/pdu/pdu_ping.h
    src/fsd/pdu/pdu_ping.cpp
    src/fsd/pdu/pdu_plane_info_request.h
    src/fsd/pdu/pdu_plane_info_request.cpp
    src/fsd/pdu/pdu_plane_info_response.h
    src/fsd/pdu/pdu_plane_info_response.cpp
    src/fsd/pdu/pdu_pong.h
    src/fsd/pdu/pdu_pong.cpp
    src/fsd/pdu/pdu_protocol_error.h
    src/fsd/pdu/pdu_protocol_error.cpp
    src/fsd/pdu/pdu_radio_message.h
    src/fsd/pdu/pdu_radio_message.cpp
    src/fsd/pdu/pdu_send_fast.h
    src/fsd/pdu/pdu_send_fast.cpp
    src/fsd/pdu/pdu_text_message.h
    src/fsd/pdu/pdu_text_message.cpp
    src/fsd/pdu/pdu_wallop.h
    src/fsd/pdu/pdu_wallop.cpp
    src/fsd/pdu/pdu_change_server.h
    src/fsd/pdu/pdu_change_server.cpp)

set(PROJECT_SOURCES
    src/main.cpp
    src/network/vatsim_auth.h
    src/common/build_config.h
    src/common/build_config.inc
    ${CMAKE_BINARY_DIR}/generated/build_config.cpp
    src/common/simplecrypt.h
    src/common/simplecrypt.cpp
    src/common/frequency_utils.h
    src/common/qserializer.h
    src/common/notificationtype.h
    src/common/utils.h
    src/common/versioncheck.h
    src/common/versioncheck.cpp
    src/common/installmodels.h
    src/common/installmodels.cpp
    src/common/typecodedatabase.h
    src/common/typecodedatabase.cpp
    src/common/libzippp.h
    src/common/libzippp.cpp
    src/common/runguard.h
    src/common/runguard.cpp
    src/config/appconfig.h
    src/config/appconfig.cpp
    src/config/windowconfig.h
    src/appcore.h
    src/appcore.cpp
    src/audio/afv.cpp
    src/audio/afv.h
    src/audio/audiodeviceinfo.h
    src/controllers/controller.h
    src/controllers/controller_manager.h
    src/controllers/controller_manager.cpp
    src/network/networkmanager.h
    src/network/networkmanager.cpp
    src/network/connectinfo.h
    src/network/serverlistmanager.h
    src/network/serverlistmanager.cpp
    src/network/nearbyatc.h
    src/network/events/radio_message_received.h
    src/simulator/xplane_adapter.h
    src/simulator/xplane_adapter.cpp
    src/aircrafts/network_aircraft.h
    src/aircrafts/aircraft_visual_state.h
    src/aircrafts/velocity_vector.h
    src/aircrafts/aircraft_configuration.h
    src/aircrafts/aircraft_configuration.cpp
    src/aircrafts/user_aircraft_data.h
    src/aircrafts/user_aircraft_config_data.h
    src/aircrafts/radio_stack_state.h
    src/aircrafts/user_aircraft_manager.h
    src/aircrafts/user_aircraft_manager.cpp
    src/aircrafts/network_aircraft_manager.h
    src/aircrafts/network_aircraft_manager.cpp)

set(AFV_NATIVE_HEADERS
    afv-native/include/afv-native/Client.h
    afv-native/include/afv-native/event.h
    afv-native/include/afv-native/Log.h
    afv-native/include/afv-native/afv/APISession.h
    afv-native/include/afv-native/afv/EffectResources.h
    afv-native/include/afv-native/afv/params.h
    afv-native/include/afv-native/afv/RadioSimulation.h
    afv-native/include/afv-native/afv/RemoteVoiceSource.h
    afv-native/include/afv-native/afv/RollingAverage.h
    afv-native/include/afv-native/afv/VoiceCompressionSink.h
    afv-native/include/afv-native/afv/VoiceSession.h
    afv-native/include/afv-native/afv/dto/AuthRequest.h
    afv-native/include/afv-native/afv/dto/PostCallsignResponse.h
    afv-native/include/afv-native/afv/dto/Station.h
    afv-native/include/afv-native/afv/dto/Transceiver.h
    afv-native/include/afv-native/afv/dto/VoiceServerConnectionData.h
    afv-native/include/afv-native/afv/dto/interfaces/IAudio.h
    afv-native/include/afv-native/afv/dto/interfaces/IAudioDirect.h
    afv-native/include/afv-native/afv/dto/interfaces/IAudioVoiceRoom.h
    afv-native/include/afv-native/afv/dto/domain/RxTransceiver.h
    afv-native/include/afv-native/afv/dto/domain/TxTransceiver.h
    afv-native/include/afv-native/afv/dto/voice_server/AudioOnDirect.h
    afv-native/include/afv-native/afv/dto/voice_server/AudioRxOnTransceivers.h
    afv-native/include/afv-native/afv/dto/voice_server/AudioTxOnTransceivers.h
    afv-native/include/afv-native/afv/dto/voice_server/Heartbeat.h
    afv-native/include/afv-native/audio/audio_params.h
    afv-native/include/afv-native/audio/AudioDevice.h
    afv-native/include/afv-native/audio/BiQuadFilter.h
    afv-native/include/afv-native/audio/FilterSource.h
    afv-native/include/afv-native/audio/IFilter.h
    afv-native/include/afv-native/audio/ISampleSink.h
    afv-native/include/afv-native/audio/ISampleSource.h
    afv-native/include/afv-native/audio/ISampleStorage.h
    afv-native/include/afv-native/audio/OutputMixer.h
    afv-native/include/afv-native/audio/RecordedSampleSource.h
    afv-native/include/afv-native/audio/SimpleCompressorEffect.h
    afv-native/include/afv-native/audio/SineToneSource.h
    afv-native/include/afv-native/audio/SinkFrameSizeAdjuster.h
    afv-native/include/afv-native/audio/SourceFrameSizeAdjuster.h
    afv-native/include/afv-native/audio/SpeexPreprocessor.h
    afv-native/include/afv-native/audio/VHFFilterSource.h
    afv-native/include/afv-native/audio/WavFile.h
    afv-native/include/afv-native/audio/WavSampleStorage.h
    afv-native/include/afv-native/audio/WhiteNoiseGenerator.h
    afv-native/include/afv-native/cryptodto/Channel.h
    afv-native/include/afv-native/cryptodto/dto/ICryptoDTO.h
    afv-native/include/afv-native/cryptodto/params.h
    afv-native/include/afv-native/cryptodto/SequenceTest.h
    afv-native/include/afv-native/cryptodto/UDPChannel.h
    afv-native/include/afv-native/cryptodto/dto/ChannelConfig.h
    afv-native/include/afv-native/cryptodto/dto/Header.h
    afv-native/include/afv-native/event/EventTimer.h
    afv-native/include/afv-native/event/EventCallbackTimer.h
    afv-native/include/afv-native/http/EventTransferManager.h
    afv-native/include/afv-native/http/http.h
    afv-native/include/afv-native/http/Request.h
    afv-native/include/afv-native/http/RESTRequest.h
    afv-native/include/afv-native/http/TransferManager.h
    afv-native/include/afv-native/util/base64.h
    afv-native/include/afv-native/util/ChainedCallback.h
    afv-native/include/afv-native/util/monotime.h
    afv-native/include/afv-native/utility.h)

set(AFV_NATIVE_AUDIO_SOURCES
    afv-native/src/audio/PortAudioAudioDevice.cpp
    afv-native/src/audio/PortAudioAudioDevice.h)

set(AFV_NATIVE_SOURCES
    afv-native/src/afv/APISession.cpp
    afv-native/src/afv/EffectResources.cpp
    afv-native/src/afv/RadioSimulation.cpp
    afv-native/src/afv/RemoteVoiceSource.cpp
    afv-native/src/afv/VoiceCompressionSink.cpp
    afv-native/src/afv/VoiceSession.cpp
    afv-native/src/afv/dto/AuthRequest.cpp
    afv-native/src/afv/dto/PostCallsignResponse.cpp
    afv-native/src/afv/dto/Station.cpp
    afv-native/src/afv/dto/Transceiver.cpp
    afv-native/src/afv/dto/VoiceServerConnectionData.cpp
    afv-native/src/audio/AudioDevice.cpp
    afv-native/src/audio/BiQuadFilter.cpp
    afv-native/src/audio/FilterSource.cpp
    afv-native/src/audio/OutputMixer.cpp
    afv-native/src/audio/RecordedSampleSource.cpp
    afv-native/src/audio/SimpleCompressorEffect.cpp
    afv-native/src/audio/SineToneSource.cpp
    afv-native/src/audio/SinkFrameSizeAdjuster.cpp
    afv-native/src/audio/SourceFrameSizeAdjuster.cpp
    afv-native/src/audio/SpeexPreprocessor.cpp
    afv-native/src/audio/VHFFilterSource.cpp
    afv-native/src/audio/WavFile.cpp
    afv-native/src/audio/WavSampleStorage.cpp
    afv-native/src/core/Client.cpp
    afv-native/src/core/Log.cpp
    afv-native/src/cryptodto/Channel.cpp
    afv-native/src/cryptodto/SequenceTest.cpp
    afv-native/src/cryptodto/UDPChannel.cpp
    afv-native/src/cryptodto/dto/ChannelConfig.cpp
    afv-native/src/cryptodto/dto/Header.cpp
    afv-native/src/event/EventCallbackTimer.cpp
    afv-native/src/event/EventTimer.cpp
    afv-native/src/http/EventTransferManager.cpp
    afv-native/src/http/TransferManager.cpp
    afv-native/src/http/Request.cpp
    afv-native/src/http/RESTRequest.cpp
    afv-native/src/util/base64.cpp
    afv-native/src/util/monotime.cpp
    ${AFV_NATIVE_AUDIO_SOURCES})

set(AFV_NATIVE_THIRDPARTY_SOURCES
    afv-native/extern/simpleSource/SimpleComp.cpp
    afv-native/extern/simpleSource/SimpleComp.h
    afv-native/extern/simpleSource/SimpleEnvelope.cpp
    afv-native/extern/simpleSource/SimpleEnvelope.h
    afv-native/extern/simpleSource/SimpleGain.h
    afv-native/extern/simpleSource/SimpleGate.cpp
    afv-native/extern/simpleSource/SimpleGate.h
    afv-native/extern/simpleSource/SimpleHeader.h
    afv-native/extern/simpleSource/SimpleLimit.cpp
    afv-native/extern/simpleSource/SimpleLimit.h
    afv-native/extern/simpleSource/SimpleCompProcess.inl)

# Set some Win32 Specific Settings
if (WIN32)
    set(GUI_TYPE WIN32)
endif (WIN32)

# Set some Apple MacOS Specific settings
if (APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)
endif ()

add_executable(xPilot
    ${GUI_TYPE}
    ${PROJECT_SOURCES}
    ${HEADERS}
    ${FSD_SOURCES}
    ${AFV_NATIVE_HEADERS}
    ${AFV_NATIVE_SOURCES}
    ${AFV_API_SOURCES}
    ${AFV_NATIVE_THIRDPARTY_SOURCES}
    xpilot.rc
    qml.qrc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/afv-native/src PREFIX "AFV Sources" FILES ${AFV_NATIVE_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/afv-native/include PREFIX "AFV Headers" FILES ${AFV_NATIVE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/afv-native/extern PREFIX "Third Party" FILES ${AFV_NATIVE_THIRDPARTY_SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/afv-native/extern/cpp-jwt/include
    ${CMAKE_SOURCE_DIR}/afv-native/extern/simpleSource
    ${CMAKE_SOURCE_DIR}/afv-native/extern
    PUBLIC
    ${CMAKE_SOURCE_DIR}/afv-native/include)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG> BOOST_DATE_TIME_NO_LIB)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Quick Qt5::Network Qt5::Svg)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib)
endif ()

if (WIN32)
    set(VATSIM_AUTH_LIBRARY ${CMAKE_SOURCE_DIR}/externals/windows/vatsimauth.lib)
    set(OPUS_LIBRARY ${CMAKE_SOURCE_DIR}/externals/windows/opus.lib)
    set(PORTAUDIO_LIBRARY ${CMAKE_SOURCE_DIR}/externals/windows/portaudio.lib)
    set(SPEEX_LIBRARY ${CMAKE_SOURCE_DIR}/externals/windows/speexdsp.lib)
    set(ALSA_LIBRARY "")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONAN_BIN_DIRS_SENTRY-CRASHPAD}/crashpad_handler.exe"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif ()

if (UNIX)
    set(VATSIM_AUTH_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/vatsimauth.a)
    set(OPUS_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/libopus.a)
    set(PORTAUDIO_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/libportaudio.a)
    set(SPEEX_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/libspeexdsp.a)

    find_library(ALSA_LIBRARY NAMES asound)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONAN_BIN_DIRS_SENTRY-CRASHPAD}/crashpad_handler"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif ()

if (APPLE)
    set(VATSIM_AUTH_LIBRARY ${CMAKE_SOURCE_DIR}/externals/macos/vatsimauth.a)
    set(OPUS_LIBRARY ${CMAKE_SOURCE_DIR}/externals/macos/libopus.a)
    set(PORTAUDIO_LIBRARY ${CMAKE_SOURCE_DIR}/externals/macos/libportaudio.a)
    set(SPEEX_LIBRARY ${CMAKE_SOURCE_DIR}/externals/macos/libspeexdsp.a)
    set(ALSA_LIBRARY "")

    find_library(COREFOUNDATION CoreFoundation)
    find_library(COREAUDIO CoreAudio)
    find_library(AUDIOTOOLBOX AudioToolbox)

    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")

    add_custom_target(osx_bundle_dirs
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/bin/xPilot.app/Contents/Resources
        COMMAND cp ${CMAKE_SOURCE_DIR}/xpilot.icns ${CMAKE_CURRENT_BINARY_DIR}/bin/xPilot.app/Contents/Resources/xpilot.icns)

    add_dependencies(${PROJECT_NAME} osx_bundle_dirs)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONAN_BIN_DIRS_SENTRY-CRASHPAD}/crashpad_handler"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif ()

add_subdirectory(3rdparty/qtpromise)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    CONAN_PKG::openssl
    CONAN_PKG::msgpack
    CONAN_PKG::nlohmann_json
    CONAN_PKG::libcurl
    CONAN_PKG::libevent
    CONAN_PKG::zeromq
    CONAN_PKG::cppzmq
    CONAN_PKG::sentry-native
    CONAN_PKG::sentry-crashpad
    CONAN_PKG::libzip
    CONAN_PKG::boost
    ${OPUS_LIBRARY}
    ${PORTAUDIO_LIBRARY}
    ${SPEEX_LIBRARY}
    ${VATSIM_AUTH_LIBRARY}
    ${COREFOUNDATION}
    ${COREAUDIO}
    ${AUDIOTOOLBOX}
    ${ALSA_LIBRARY}
    qtpromise)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES)
endif()
