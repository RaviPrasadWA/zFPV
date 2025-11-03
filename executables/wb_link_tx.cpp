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
  int tx_interval_ms = 100;
  bool use_pipe = false;
  bool is_air = true;
  size_t pipe_chunk_size = 1400; // fits max payload

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--emulate") emulate = true;
    else if (a == "--freq" && i + 1 < argc) initial_freq = std::stoi(argv[++i]);
    else if (a == "--bw" && i + 1 < argc) initial_bw = std::stoi(argv[++i]);
    else if (a == "--mcs" && i + 1 < argc) mcs_index = std::stoi(argv[++i]);
    else if (a == "--interval" && i + 1 < argc) tx_interval_ms = std::stoi(argv[++i]);
    else if (a == "--pipe") use_pipe = true;
    else if (a == "--pipe-chunk-size" && i + 1 < argc) pipe_chunk_size = std::stoi(argv[++i]);
    else if (a == "--help") {
      std::cout << "Usage: wb_link_tx_example [--emulate] [--freq MHz] [--bw MHz] [--mcs N] [--interval ms] [--pipe] [--pipe-chunk-size N]\n";
      std::cout << "  Example: libcamera-vid ... | ./wb_link_tx_example --emulate --pipe\n";
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

  std::shared_ptr<WBTxRx> txrx;
  try { txrx = std::make_shared<WBTxRx>(cards, options_txrx, radiotap_header_holder); }
  catch (const std::exception &e) { std::cerr << "Failed to create WBTxRx: " << e.what() << std::endl; return 2; }

  //std::shared_ptr<WBTxRx> txrx =
  //    std::make_shared<WBTxRx>(cards, options_txrx, radiotap_header_holder);

  std::signal(SIGINT, sigint_handler);
  std::signal(SIGTERM, sigint_handler);

  std::cout << "Transmitter running. Press Ctrl-C to quit." << std::endl;
  if (use_pipe) {
     std::vector<uint8_t> buffer(pipe_chunk_size);
     while (keep_running) {
       std::cin.read(reinterpret_cast<char*>(buffer.data()), pipe_chunk_size);
       std::streamsize got = std::cin.gcount();
       if (got <= 0) break;
       auto header = radiotap_header_holder->thread_safe_get();
       
       txrx->tx_inject_packet(1, buffer.data(), got, header, false);
       std::cout << "TX pipe chunk: " << got << " bytes" << std::endl;
     }
   } else {
    int counter = 0;
    while (keep_running) {
      std::string payload = "Hello " + std::to_string(counter);
      auto header = radiotap_header_holder->thread_safe_get();
      txrx->tx_inject_packet(0, (uint8_t *)payload.data(), payload.size(), header, false);
      std::cout << "TX: " << payload << std::endl;
      ++counter;
      std::this_thread::sleep_for(std::chrono::milliseconds(tx_interval_ms));
    }
   }
  std::cout << "Transmitter shutting down..." << std::endl;
  return 0;
}
