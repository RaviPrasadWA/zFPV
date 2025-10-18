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
  if (emulate) cards.push_back(wifibroadcast::create_card_emulate(false));
  else cards.push_back(wifibroadcast::WifiCard{"wlan1", wifibroadcast::WIFI_CARD_TYPE_UNKNOWN});

  WBTxRx::Options options;
  auto radiotap_holder = std::make_shared<RadiotapHeaderTxHolder>();
  RadiotapHeaderTx::UserSelectableParams params{};
  params.bandwidth = initial_bw;
  params.mcs_index = mcs_index;
  radiotap_holder->thread_safe_set(params);

  std::shared_ptr<WBTxRx> wb;
  try { wb = std::make_shared<WBTxRx>(cards, options, radiotap_holder); }
  catch (const std::exception &e) { std::cerr << "Failed to create WBTxRx: " << e.what() << std::endl; return 2; }

  if (use_pipe) {
    wb->rx_register_callback(
        [](uint64_t nonce, int wlan_index, const uint8_t radioPort, const uint8_t *data, const int data_len) {
          std::cout.write(reinterpret_cast<const char*>(data), data_len);
          std::cout.flush();
        });
  } else {
    wb->rx_register_callback(
        [](uint64_t nonce, int wlan_index, const uint8_t radioPort, const uint8_t *data, const int data_len) {
          std::cout << "RX nonce=" << nonce << " wlan=" << wlan_index << " port=" << (int)radioPort
                    << " len=" << data_len << " first_bytes=";
          for (int i = 0; i < std::min(8, data_len); ++i) std::cout << std::hex << (int)data[i] << " ";
          std::cout << std::dec << std::endl;
        });
  }

  wb->start_receiving();

  std::signal(SIGINT, sigint_handler);
  std::signal(SIGTERM, sigint_handler);

  std::cout << "Receiver running. Press Ctrl-C to quit." << std::endl;
  while (keep_running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  std::cout << "Receiver shutting down..." << std::endl;
  wb->stop_receiving();
  return 0;
}
