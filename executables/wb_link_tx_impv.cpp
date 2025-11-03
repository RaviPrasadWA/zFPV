// Simple transmitter example for WBTxRx
// Build: g++ -std=c++17 -IOpenHD/ohd_interface/inc -IOpenHD/ohd_interface/lib/wifibroadcast/wifibroadcast/src \
//   OpenHD/ohd_interface/tools/wb_link_tx_example.cpp -lpthread -lpcap -o wb_link_tx_example
// Usage: ./wb_link_tx_example --emulate --freq 5180 --bw 20

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../src/WBStreamRx.h"
#include "../src/WBStreamTx.h"
#include "../src/WBTxRx.h"
#include "../src/WiFiCard.h"
#include "spdlog.h"
#include "profile.h"
#include "camera.hpp"
#include "wb_link_helper.h"
#include "wifi_card_discovery.h"
#include "wifi_command_helper.h"
#include "wb_link.h"
#include "wb_interface.h"

// static constexpr std::chrono::milliseconds SESSION_KEY_PACKETS_INTERVAL =
//       std::chrono::milliseconds(500);
// static constexpr bool DEFAULT_ENABLE_SHORT_GUARD = false;
// static constexpr auto DEFAULT_MCS_INDEX = 2;
// static constexpr bool DEFAULT_ENABLE_LDPC = false;
// Video is unidirectional from air to ground
// static constexpr auto VIDEO_PRIMARY_RADIO_PORT = 10;
// static constexpr auto VIDEO_SECONDARY_RADIO_PORT = 11;

int main(int argc, char **argv) {

    std::shared_ptr<spdlog::logger> m_console = openhd::log::create_or_get("main");
    assert(m_console);
    const auto m_profile = DProfile::discover(true);  // true since this is the tx main
    m_console->debug("Starting the Tx");
    auto m_wb_interface = std::make_shared<OHDInterface>( m_profile);
}