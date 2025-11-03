// Simple receiver example for WBTxRx
// Build: g++ -std=c++17 -IOpenHD/ohd_interface/inc -IOpenHD/ohd_interface/lib/wifibroadcast/wifibroadcast/src \
//   OpenHD/ohd_interface/tools/wb_link_rx_example.cpp -lpthread -lpcap -o wb_link_rx_example
// Usage: ./wb_link_rx_example --emulate --freq 5180 --bw 20

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "wb_link_manager.h"
#include "../src/WBTxRx.h"
#include "../src/WiFiCard.h"
#include "../src/radiotap/RadiotapHeaderTxHolder.hpp"

static std::atomic<bool> keep_running{true};
static void sigint_handler(int) { keep_running = false; }

int main(int argc, char **argv) {
  bool emulate = false;
  int initial_freq = 5180;
  int initial_bw = 20;
  int mcs_index = 3;
  bool use_pipe = false;
  bool is_air = false;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--emulate") emulate = true;
    else if (a == "--freq" && i + 1 < argc) initial_freq = std::stoi(argv[++i]);
    else if (a == "--bw" && i + 1 < argc) initial_bw = std::stoi(argv[++i]);
    else if (a == "--mcs" && i + 1 < argc) mcs_index = std::stoi(argv[++i]);
    else if (a == "--pipe") use_pipe = true;
    else if (a == "--help") {
      std::cout << "Usage: wb_link_rx_example [--emulate] [--freq MHz] [--bw MHz] [--mcs N] [--pipe]\n";
      std::cout << "  Example: ./wb_link_rx_example --emulate --pipe | ffplay -fflags nobuffer -i -\n";
      return 0;
    }
  }

  std::vector<wifibroadcast::WifiCard> cards;
  if (emulate) cards.push_back(wifibroadcast::create_card_emulate(true));
  else cards.push_back(wifibroadcast::WifiCard{"wlan1", wifibroadcast::WIFI_CARD_TYPE_UNKNOWN});

  WBTxRx::Options options_txrx{};
  options_txrx.pcap_rx_set_direction = true;
  options_txrx.use_gnd_identifier = !is_air;


  auto radiotap_header_holder = std::make_shared<RadiotapHeaderTxHolder>();
  RadiotapHeaderTx::UserSelectableParams params{};
  params.bandwidth = initial_bw;
  params.mcs_index = mcs_index;
  radiotap_header_holder->thread_safe_set(params);

  // std::shared_ptr<WBTxRx> wb;
  // try { wb = std::make_shared<WBTxRx>(cards, options, radiotap_holder); }

  // auto radiotap_header_holder = std::make_shared<RadiotapHeaderTxHolder>();
  std::shared_ptr<WBTxRx> txrx =
      std::make_shared<WBTxRx>(cards, options_txrx, radiotap_header_holder);


  if (use_pipe) {
    txrx->rx_register_callback(
        [](uint64_t nonce, int wlan_index, const uint8_t radioPort, const uint8_t *data, const int data_len) {
          std::cout.write(reinterpret_cast<const char*>(data), data_len);
          std::cout.flush();
        });
  } else {
    txrx->rx_register_callback(
        [](uint64_t nonce, int wlan_index, const uint8_t radioPort, const uint8_t *data, const int data_len) {
          std::cout << "RX nonce=" << nonce << " wlan=" << wlan_index << " port=" << (int)radioPort
                    << " len=" << data_len << " first_bytes=";
          for (int i = 0; i < std::min(8, data_len); ++i) std::cout << std::hex << (int)data[i] << " ";
          std::cout << std::dec << std::endl;
        });
  }

  txrx->start_receiving();

  // WBTxRx::OUTPUT_DATA_CALLBACK cb =
  //     [](uint64_t nonce, int wlan_index, const uint8_t radioPort,
  //        const uint8_t *data, const std::size_t data_len) {
  //       std::string message((const char *)data, data_len);
  //       fmt::print("Got packet[{}]\n", message);
  //     };
  // txrx->rx_register_callback(cb);

  std::signal(SIGINT, sigint_handler);
  std::signal(SIGTERM, sigint_handler);

  std::cout << "Receiver running. Press Ctrl-C to quit." << std::endl;
  while (keep_running) {
    // auto stats = txrx->get_rx_stats();
    // std::cout << "RX stats: " << WBTxRx::rx_stats_to_string(stats) << std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  std::cout << "Receiver shutting down..." << std::endl;
  txrx->stop_receiving();
  return 0;
}
