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
    std::vector<WiFiCard> m_monitor_mode_cards{};
    std::optional<WiFiCard> m_opt_hotspot_card = std::nullopt;
    const auto config = openhd::load_config();
    DWifiCards::main_discover_an_process_wifi_cards(config, m_profile, m_console, m_monitor_mode_cards, m_opt_hotspot_card);
    m_console->debug("monitor_mode card(s):{}",debug_cards(m_monitor_mode_cards));
    if (m_opt_hotspot_card.has_value()) {
        m_console->debug("Hotspot card:{}", m_opt_hotspot_card.value().device_name);
    } else {
        m_console->debug("No WiFi hotspot card");
    }
    // We don't have at least one card for monitor mode, which means we cannot
    // instantiate wb_link (no wifibroadcast connectivity at all)
    if (m_monitor_mode_cards.empty()) {
        m_console->warn(
            "Cannot start Tx, no wifi card for monitor mode");
        const std::string message_for_user = "No WiFi card found, please reboot";
        m_console->warn(message_for_user);
    }else{
        openhd::wb::takeover_cards_monitor_mode(m_monitor_mode_cards, m_console);
        std::shared_ptr<WBLink> m_wb_link = std::make_shared<WBLink>(m_profile, m_monitor_mode_cards);
    }
    // const std::string card_name = "wlan1";
    // auto card_opt = DWifiCards::process_card(card_name);

    // if (!card_opt.has_value()) {
    //     openhd::log::get_default()->warn("Cannot find card {}", card_name);
    //     return 0;
    // }

    // auto card = card_opt.value();

    // Take over the card
    // wifi::commandhelper::nmcli_set_device_managed_status(card.device_name, false);
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    // wifi::commandhelper::iw_enable_monitor_mode(card.device_name);
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    // wifi::commandhelper::iw_set_frequency_and_channel_width(card.device_name,5180,20);
    // std::this_thread::sleep_for(std::chrono::seconds(2));

    // bool emulate = false;
    // int initial_freq = 5180;
    // int initial_bw = 20;
    // int mcs_index = DEFAULT_MCS_INDEX;
    // int tx_interval_ms = 100;
    // bool use_pipe = false;
    // bool is_air = true;
    // size_t pipe_chunk_size = 1400; // fits max payload
    // bool tx_set_high_retransmit_count = true;
    // const bool set_flag_tx_no_ack = !tx_set_high_retransmit_count;
    // bool wb_enable_stbc = false;  // 0==disabled
    // bool wb_enable_ldpc = DEFAULT_ENABLE_LDPC;

    // std::shared_ptr<RadiotapHeaderTxHolder> m_tx_header_1;
    // On air, we use different radiotap data header(s) for different streams
    // (20Mhz vs 40Mhz)
    // std::shared_ptr<RadiotapHeaderTxHolder> m_tx_header_2;

    // int tx_channel_width = static_cast<int>(initial_bw);

    // std::shared_ptr<WBTxRx> m_wb_txrx;

    // for (int i = 1; i < argc; ++i) {
    //     std::string a = argv[i];
    //     if (a == "--emulate") emulate = true;
    //     else if (a == "--freq" && i + 1 < argc) initial_freq = std::stoi(argv[++i]);
    //     else if (a == "--bw" && i + 1 < argc) tx_channel_width = std::stoi(argv[++i]);
    //     else if (a == "--interval" && i + 1 < argc) tx_interval_ms = std::stoi(argv[++i]);
    //     else if (a == "--pipe") use_pipe = true;
    //     else if (a == "--pipe-chunk-size" && i + 1 < argc) pipe_chunk_size = std::stoi(argv[++i]);
    //     else if (a == "--help") {
    //     std::cout << "Usage: wb_link_tx_example [--emulate] [--freq MHz] [--bw MHz] [--mcs N] [--interval ms] [--pipe] [--pipe-chunk-size N]\n";
    //     std::cout << "  Example: libcamera-vid ... | ./wb_link_tx_example --emulate --pipe\n";
    //     return 0;
    //     }
    // }

    // auto tmp_params =
    //     RadiotapHeaderTx::UserSelectableParams{tx_channel_width,
    //                                            DEFAULT_ENABLE_SHORT_GUARD,
    //                                            wb_enable_stbc,
    //                                            wb_enable_ldpc,
    //                                            mcs_index,
    //                                            set_flag_tx_no_ack};
    

    // WBTxRx::Options txrx_options{};
    // WBStreamTx::Options options_video_tx{};
    // std::vector<wifibroadcast::WifiCard> wifi_cards;
    // wifi_cards.push_back(wifibroadcast::WifiCard{"wlan1", wifibroadcast::WIFI_CARD_TYPE_UNKNOWN});

    // m_tx_header_1 = std::make_shared<RadiotapHeaderTxHolder>();

    // txrx_options.session_key_packet_interval = SESSION_KEY_PACKETS_INTERVAL;
    // txrx_options.use_gnd_identifier = !is_air;
    // txrx_options.debug_rssi = 0;
    // txrx_options.debug_multi_rx_packets_variance = false;
    // txrx_options.tx_without_pcap = true;
    // txrx_options.set_tx_sock_qdisc_bypass = true;
    // txrx_options.max_sane_injection_time = std::chrono::milliseconds(1);

    // m_tx_header_1->thread_safe_set(tmp_params);


    // options_video_tx.enable_fec = true;
    // For now, have a fifo of X frame(s) to smooth out extreme edge cases of
    // bitrate overshoot
    // TODO: In ohd_video,  differentiate between "frame" and NALU (nalu can
    // also be config data) such that we can make this queue smaller.
    // options_video_tx.block_data_queue_size = 2;
    // options_video_tx.radio_port = VIDEO_PRIMARY_RADIO_PORT;


    // m_wb_txrx =
    //   std::make_shared<WBTxRx>(wifi_cards, txrx_options, m_tx_header_1);

    // auto primary = std::make_unique<WBStreamTx>(m_wb_txrx, options_video_tx,m_tx_header_1);
}