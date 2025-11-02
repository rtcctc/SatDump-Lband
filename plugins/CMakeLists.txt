set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

# We need those to be available for plugins!
include_directories(../src-core ../src-interface)

option(PLUGINS_ALL "Enable all plugins by default" ON)

option(PLUGIN_SPACEX "Enable the SpaceX support plugin" OFF)

# ##############################################################################
option(PLUGIN_SCRIPTING "Enable the scripting support plugin (EXPERIMENTAL)"
       OFF)

if(PLUGIN_SCRIPTING) # OR PLUGINS_ALL)
  add_subdirectory(scripting_support)
endif()

# ##############################################################################

option(PLUGIN_METEOR "Enable the Meteor (HRPT, LRPT, etc) support plugin" OFF)

if(PLUGIN_METEOR OR PLUGINS_ALL)
  add_subdirectory(meteor_support)
endif()

option(
  PLUGIN_NOAA_METOP
  "Enable the NOAA/MetOp POES (AHRPT, HRPT, X-Band dumps, GAC, etc) support plugin"
  OFF)

if(PLUGIN_NOAA_METOP OR PLUGINS_ALL)
  add_subdirectory(noaa_metop_support)
endif()

option(PLUGIN_AWS "Enable the AWS (Sterna) support plugin" OFF)

if(PLUGIN_AWS OR PLUGINS_ALL)
  add_subdirectory(aws_support)
endif()

# ##############################################################################
option(PLUGIN_SIMD_SSE41 "Enable SIMD SSE41 plugin" ON)

if(PLUGIN_SIMD_SSE41)
  add_subdirectory(simd_extensions/simd_sse41)
endif()

option(PLUGIN_SIMD_NEON "Enable SIMD NEON plugin" ON)

if(PLUGIN_SIMD_NEON)
  add_subdirectory(simd_extensions/simd_neon)
endif()

option(PLUGIN_SIMD_AVX2 "Enable SIMD AVX2 plugin" ON)

if(PLUGIN_SIMD_AVX2)
  add_subdirectory(simd_extensions/simd_avx2)
endif()

# ##############################################################################
option(PLUGIN_AIRSPY_SDR_SUPPORT "Enable Airspy SDR Support plugin" ON)

if(PLUGIN_AIRSPY_SDR_SUPPORT)
  add_subdirectory(sdr_sources/airspy_sdr_support)
endif()

option(PLUGIN_HYDRASDR_SDR_SUPPORT "Enable HydraSDR Support plugin" ON)

if(PLUGIN_HYDRASDR_SDR_SUPPORT)
  add_subdirectory(sdr_sources/hydrasdr_sdr_support)
endif()

option(PLUGIN_AIRSPYHF_SDR_SUPPORT "Enable AirspyHF SDR Support plugin" ON)

if(PLUGIN_AIRSPYHF_SDR_SUPPORT)
  add_subdirectory(sdr_sources/airspyhf_sdr_support)
endif()

option(PLUGIN_HACKRF_SDR_SUPPORT "Enable HackRF SDR Support plugin" ON)

if(PLUGIN_HACKRF_SDR_SUPPORT)
  add_subdirectory(sdr_sources/hackrf_sdr_support)
endif()

option(PLUGIN_LIMESDR_SDR_SUPPORT "Enable LimeSDR SDR Support plugin" ON)

if(PLUGIN_LIMESDR_SDR_SUPPORT)
  add_subdirectory(sdr_sources/limesdr_sdr_support)
endif()

option(PLUGIN_SDRPLAY_SDR_SUPPORT "Enable SDRPlay SDR Support plugin" ON)

if(PLUGIN_SDRPLAY_SDR_SUPPORT)
  add_subdirectory(sdr_sources/sdrplay_sdr_support)
endif()

option(PLUGIN_PLUTOSDR_SDR_SUPPORT "Enable PlutoSDR SDR Support plugin" ON)

if(PLUGIN_PLUTOSDR_SDR_SUPPORT)
  add_subdirectory(sdr_sources/plutosdr_sdr_support)
endif()

option(PLUGIN_BLADERF_SDR_SUPPORT "Enable BladeRF SDR Support plugin" ON)

if(PLUGIN_BLADERF_SDR_SUPPORT)
  add_subdirectory(sdr_sources/bladerf_sdr_support)
endif()

option(PLUGIN_USRP_SDR_SUPPORT "Enable USRP SDR Support plugin" ON)

if(PLUGIN_USRP_SDR_SUPPORT)
  add_subdirectory(sdr_sources/usrp_sdr_support)
endif()

option(PLUGIN_RTLSDR_SDR_SUPPORT "Enable RTL-SDR SDR Support plugin" ON)

if(PLUGIN_RTLSDR_SDR_SUPPORT)
  add_subdirectory(sdr_sources/rtlsdr_sdr_support)
endif()

option(PLUGIN_MIRISDR_SDR_SUPPORT "Enable MiriSDR SDR Support plugin" ON)

if(PLUGIN_MIRISDR_SDR_SUPPORT)
  add_subdirectory(sdr_sources/mirisdr_sdr_support)
endif()

option(PLUGIN_SPYSERVER_SUPPORT "Enable SpyServer Support plugin" ON)

if(PLUGIN_SPYSERVER_SUPPORT)
  add_subdirectory(sdr_sources/spyserver_support)
endif()

option(PLUGIN_RTLTCP_SUPPORT "Enable RTL-TCP Support plugin" ON)

if(PLUGIN_RTLTCP_SUPPORT)
  add_subdirectory(sdr_sources/rtltcp_support)
endif()

option(PLUGIN_SDRPP_SERVER_SUPPORT "Enable SDR++ Server Support plugin" ON)

if(PLUGIN_SDRPP_SERVER_SUPPORT)
  add_subdirectory(sdr_sources/sdrpp_server_support)
endif()

option(PLUGIN_AARONIA_SDR_SUPPORT "Enable Aaronia Spectran Support plugin" ON)

if(PLUGIN_AARONIA_SDR_SUPPORT)
  add_subdirectory(sdr_sources/aaronia_sdr_support)
endif()

option(PLUGIN_HAROGIC_SDR_SUPPORT "Enable Aaronia Spectran Support plugin" OFF)

if(PLUGIN_HAROGIC_SDR_SUPPORT)
  add_subdirectory(sdr_sources/harogic_sdr_support)
endif()

option(PLUGIN_RFNM_SDR_SUPPORT "Enable RFNM Support plugin" ON)

if(PLUGIN_RFNM_SDR_SUPPORT)
  add_subdirectory(sdr_sources/rfnm_sdr_support)
endif()

option(PLUGIN_FOBOSSDR_SDR_SUPPORT "Enable FobosSDR Support plugin" ON)

if(PLUGIN_FOBOSSDR_SDR_SUPPORT)
  add_subdirectory(sdr_sources/fobos_sdr_support)
endif()

option(PLUGIN_RTAUDIO_SDR_SUPPORT "Enable RtAudio Source Support plugin" OFF)

if(PLUGIN_RTAUDIO_SDR_SUPPORT)
  add_subdirectory(sdr_sources/rtaudio_sdr_support)
endif()

option(PLUGIN_NET_SOURCE_SDR_SUPPORT "Enable Network Source Support plugin" ON)

if(PLUGIN_NET_SOURCE_SDR_SUPPORT)
  add_subdirectory(sdr_sources/net_source_support)
endif()

option(PLUGIN_REMOTE_SDR_SUPPORT "Enable Remote SDR Support plugin (WIP!)" ON)

if(PLUGIN_REMOTE_SDR_SUPPORT)
  add_subdirectory(sdr_sources/remote_sdr_support)
endif()

# Disable by default, sort of experimental
option(PLUGIN_SDDC_SDR_SUPPORT
       "Enable SDDC (RX666, RX888, etc) SDR Support plugin" OFF)

if(PLUGIN_SDDC_SDR_SUPPORT)
  add_subdirectory(sdr_sources/sddc_sdr_support)
endif()

# NOT recommended
option(PLUGIN_SOAPY_SDR_SUPPORT
       "Enable Soapy SDR Support plugin (NOT RECOMMENDED!)" OFF)

if(PLUGIN_SOAPY_SDR_SUPPORT)
  add_subdirectory(sdr_sources/soapy_sdr_support)
endif()

# ##############################################################################
option(PLUGIN_RTAUDIO_SINK "Enable RtAudio Audio Sink plugin" OFF)

if(PLUGIN_RTAUDIO_SINK)
  add_subdirectory(audio_sinks/rtaudio_audio_sink)
endif()

option(PLUGIN_PORTAUDIO_SINK "Enable PortAudio Audio Sink plugin" ON)

if(PLUGIN_PORTAUDIO_SINK)
  add_subdirectory(audio_sinks/portaudio_audio_sink)
endif()

option(PLUGIN_AAUDIO_SINK "Enable AAudio Audio Sink plugin" ON)

if(PLUGIN_AAUDIO_SINK)
  add_subdirectory(audio_sinks/aaudio_audio_sink)
endif()

# ##############################################################################
option(PLUGIN_BOCHUM_SUPPORT "Enable Bochum-specific plugin" OFF)

if(PLUGIN_BOCHUM_SUPPORT)
  add_subdirectory(bochum_support)
endif()

# ##############################################################################
option(PLUGIN_STANDARD_CPP_COMPOS "Enable Standard C++ Composites plugin" OFF)

if(PLUGIN_STANDARD_CPP_COMPOS OR PLUGINS_ALL)
  add_subdirectory(standard_cpp_compos_support)
endif()

# ##############################################################################
option(PLUGIN_BITVIEW_APP "Enable BitView Application (Very WIP)" OFF)

if(PLUGIN_BITVIEW_APP)
  add_subdirectory(bitview_app)
endif()
