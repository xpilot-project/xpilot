cmake_minimum_required(VERSION 3.14)
project(xPilot LANGUAGES CXX)

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

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick REQUIRED)
find_package(Qt5QuickCompiler)

qtquick_compiler_add_resources(RESOURCES qml.qrc)

#AUX_SOURCE_DIRECTORY("${CMAKE_SOURCE_DIR}/protobuf" PROTOBUF_SRC)

set(PROJECT_SOURCES
    src/main.cpp
#    src/interprocess.cpp
#    src/interprocess.h
#    src/radiostack.h
#    src/appconfig.h
#    src/nearbyatc.h
#    src/base64.hpp
    src/afv.cpp
    src/afv.h
    src/core/threadutils.h
    src/core/worker.h
    src/core/worker.cpp
    src/fsd/FsdSession.cpp
    src/fsd/FsdSession.h
    src/fsd/pdu/PDUBase.cpp
    src/fsd/pdu/PDUBase.h
    src/fsd/pdu/PDUServerIdentification.cpp
    src/fsd/pdu/PDUServerIdentification.h
    src/fsd/pdu/PDUClientIdentification.cpp
    src/fsd/pdu/PDUClientIdentification.h
#    ${PROTOBUF_SRC}
)

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
    afv-native/include/afv-native/audio/PinkNoiseGenerator.h
    afv-native/include/afv-native/audio/RecordedSampleSource.h
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
IF(WIN32)
SET(GUI_TYPE WIN32)
ENDIF(WIN32)
# Set some Apple MacOS Specific settings
IF (APPLE)
SET(GUI_TYPE MACOSX_BUNDLE)
ENDIF (APPLE)

add_executable(xPilot
  ${GUI_TYPE}
  ${PROJECT_SOURCES}
  ${HEADERS}
  ${AFV_NATIVE_HEADERS}
  ${AFV_NATIVE_SOURCES}
  ${AFV_API_SOURCES}
  ${AFV_NATIVE_THIRDPARTY_SOURCES}
  ${RESOURCES}
  xpilot.rc
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/afv-native/src PREFIX "AFV Sources" FILES ${AFV_NATIVE_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/afv-native/include PREFIX "AFV Headers" FILES ${AFV_NATIVE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/afv-native/extern PREFIX "Third Party" FILES ${AFV_NATIVE_THIRDPARTY_SOURCES})

if(WIN32)
        set(OPUS_URL https://github.com/xpilot-project/opus/releases/download/xpilot/windows-opus.lib)
        set(OPUS_LIBARY_PATH ${CMAKE_BINARY_DIR}/import/windows-opus.lib)

        set(PORTAUDIO_URL https://github.com/xpilot-project/portaudio/releases/download/xpilot/windows-portaudio.lib)
        set(PORTAUDIO_LIBRARY_PATH ${CMAKE_BINARY_DIR}/import/windows-portaudio.lib)

        set(SPEEXDSP_URL https://github.com/xpilot-project/speexdsp/releases/download/xpilot/windows-speexdsp.lib)
        set(SPEEXDSP_LIBRARY_PATH ${CMAKE_BINARY_DIR}/import/windows-speexdsp.lib)
elseif(UNIX)
    set(OPUS_URL https://github.com/xpilot-project/opus/releases/download/xpilot/linux-opus.lib)
    set(OPUS_LIBARY_PATH ${CMAKE_BINARY_DIR}/import/linux-opus.lib)

    set(PORTAUDIO_URL https://github.com/xpilot-project/portaudio/releases/download/xpilot/linux-libportaudio.lib)
    set(PORTAUDIO_LIBRARY_PATH ${CMAKE_BINARY_DIR}/import/linux-libportaudio.lib)

    set(SPEEXDSP_URL https://github.com/xpilot-project/speexdsp/releases/download/xpilot/linux-libspeexdsp.lib)
    set(SPEEXDSP_LIBRARY_PATH ${CMAKE_BINARY_DIR}/import/linux-libspeexdsp.lib)
elseif(APPLE)
    set(OPUS_URL https://github.com/xpilot-project/opus/releases/download/xpilot/macos-opus.lib)
    set(OPUS_LIBARY_PATH ${CMAKE_BINARY_DIR}/import/macos-opus.lib)

    set(PORTAUDIO_URL https://github.com/xpilot-project/portaudio/releases/download/xpilot/macos-libportaudio.lib)
    set(PORTAUDIO_LIBRARY_PATH ${CMAKE_BINARY_DIR}/import/macos-libportaudio.lib)

    set(SPEEXDSP_URL https://github.com/xpilot-project/speexdsp/releases/download/xpilot/macos-libspeexdsp.lib)
    set(SPEEXDSP_LIBRARY_PATH ${CMAKE_BINARY_DIR}/import/macos-libspeexdsp.lib)
endif()

file(DOWNLOAD ${OPUS_URL} ${OPUS_LIBARY_PATH})
file(DOWNLOAD ${PORTAUDIO_URL} ${PORTAUDIO_LIBRARY_PATH})
file(DOWNLOAD ${SPEEXDSP_URL} ${SPEEXDSP_LIBRARY_PATH})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/afv-native/extern/cpp-jwt/include
    ${CMAKE_SOURCE_DIR}/afv-native/extern/simpleSource
    ${CMAKE_SOURCE_DIR}/afv-native/extern
    PUBLIC
    ${CMAKE_SOURCE_DIR}/afv-native/include)

target_compile_definitions(xPilot
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(xPilot
  PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Quick)

#find_package(Protobuf REQUIRED)
#if(Protobuf_FOUND)
#        message("Found Protobuf library")
#        message(${Protobuf_INCLUDE_DIRS})
#        include_directories(${Protobuf_INCLUDE_DIRS})
#        target_link_libraries(xPilot PRIVATE ${Protobuf_LIBRARIES})
#endif(Protobuf_FOUND)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib)
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    CONAN_PKG::openssl
    CONAN_PKG::msgpack
    CONAN_PKG::nlohmann_json
    CONAN_PKG::libcurl
    CONAN_PKG::libevent
    ${OPUS_LIBARY_PATH}
    ${PORTAUDIO_LIBRARY_PATH}
    ${SPEEXDSP_LIBRARY_PATH})

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES)
endif()
