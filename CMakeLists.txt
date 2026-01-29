find_package(DPP CONFIG REQUIRED)

add_library(discord_webhook_plugin
MODULE
    discord_webhook_plugin.cc
    commands/help.cpp
    commands/ping.cpp
    commands/info.cpp
    commands/rates.cpp
    commands/tg.cpp
    commands/active.cpp
    )

target_link_libraries(discord_webhook_plugin trunk_recorder_library dpp ssl crypto ${CURL_LIBRARIES} ${Boost_LIBRARIES} ${GNURADIO_PMT_LIBRARIES} ${GNURADIO_RUNTIME_LIBRARIES} ${GNURADIO_FILTER_LIBRARIES} ${GNURADIO_DIGITAL_LIBRARIES} ${GNURADIO_ANALOG_LIBRARIES} ${GNURADIO_AUDIO_LIBRARIES} ${GNURADIO_UHD_LIBRARIES} ${UHD_LIBRARIES} ${GNURADIO_BLOCKS_LIBRARIES} ${GNURADIO_OSMOSDR_LIBRARIES}  ${LIBOP25_REPEATER_LIBRARIES} gnuradio-op25_repeater)

install(TARGETS discord_webhook_plugin LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/trunk-recorder)