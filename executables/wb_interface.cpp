/******************************************************************************
 * OpenHD
 *
 * Licensed under the GNU General Public License (GPL) Version 3.
 *
 * This software is provided "as-is," without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose, and non-infringement. For details, see the
 * full license in the LICENSE file provided with this source code.
 *
 * Non-Military Use Only:
 * This software and its associated components are explicitly intended for
 * civilian and non-military purposes. Use in any military or defense
 * applications is strictly prohibited unless explicitly and individually
 * licensed otherwise by the OpenHD Team.
 *
 * Contributors:
 * A full list of contributors can be found at the OpenHD GitHub repository:
 * https://github.com/OpenHD
 *
 * © OpenHD, All Rights Reserved.
 ******************************************************************************/

#include "wb_interface.h"

#include <utility>

#include "wifi_card_discovery.h"
#include "ini_config.h"
#include "global_constants.hpp"
#include "util_filesystem.h"
#include "wb_link.h"

// Helper function to execute a shell command and return the output
std::string exec(const std::string& cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

OHDInterface::OHDInterface(OHDProfile profile1)
    : m_profile(std::move(profile1)) {
  m_console = openhd::log::create_or_get("interface");
  assert(m_console);
  m_monitor_mode_cards = {};
  m_opt_hotspot_card = std::nullopt;
  const auto config = openhd::load_config();

  DWifiCards::main_discover_an_process_wifi_cards(
      config, m_profile, m_console, m_monitor_mode_cards, m_opt_hotspot_card);
  m_console->debug("monitor_mode card(s):{}",
                   debug_cards(m_monitor_mode_cards));
  if (m_opt_hotspot_card.has_value()) {
    m_console->debug("Hotspot card:{}", m_opt_hotspot_card.value().device_name);
  } else {
    m_console->debug("No WiFi hotspot card");
  }
  // We don't have at least one card for monitor mode, which means we cannot
  // instantiate wb_link (no wifibroadcast connectivity at all)
  if (m_monitor_mode_cards.empty()) {
    m_console->warn(
        "Cannot start ohd_interface, no wifi card for monitor mode");
    const std::string message_for_user = "No WiFi card found, please reboot";
    m_console->warn(message_for_user);
    // TODO reason what to do. We do not support dynamically adding wifi cards
    // at run time, so somehow we need to signal to the user that something is
    // completely wrong. However, as an Ground pi, we can still run QOpenHD and
    // OpenHD, just it will never connect to an Air PI
  } else {
    // Set the card(s) we have into monitor mode
    openhd::wb::takeover_cards_monitor_mode(m_monitor_mode_cards, m_console);
    m_wb_link = std::make_shared<WBLink>(m_profile, m_monitor_mode_cards);
  }
  m_console->debug("OHDInterface::created");
}

OHDInterface::~OHDInterface() {
  // Terminate the link first
  m_wb_link = nullptr;
  // Then give the card(s) back to the system (no monitor mode)
  // give the monitor mode cards back to network manager
  openhd::wb::giveback_cards_monitor_mode(m_monitor_mode_cards, m_console);
}

std::vector<openhd::Setting> OHDInterface::get_all_settings() {
  std::vector<openhd::Setting> ret;
  m_console->debug("get all settings");
  if (m_wb_link) {
    auto settings = m_wb_link->get_all_settings();
    OHDUtil::vec_append(ret, settings);
  }
  openhd::validate_provided_ids(ret);
  return ret;
}

void OHDInterface::print_internal_fec_optimization_method() {
  fec_stream_print_fec_optimization_method();
}

std::shared_ptr<OHDLink> OHDInterface::get_link_handle() {
  if (m_wb_link) {
    // m_console->warn("Using Link: OpenHD-WifiBroadCast");
    return m_wb_link;
  }
  return nullptr;
}

void OHDInterface::generate_keys_from_pw_if_exists_and_delete() {
  // Make sure this stupid sodium init has been called
  if (sodium_init() == -1) {
    std::cerr << "Cannot init libsodium" << std::endl;
    exit(EXIT_FAILURE);
  }
  auto console = openhd::log::get_default();

  // If no keypair file exists (It was not created from the password.txt file)
  // we create the txrx.key once (from the default password) such that the boot
  // up time is sped up on successive boot(s)
  auto val = wb::read_keypair_from_file(openhd::SECURITY_KEYPAIR_FILENAME);
  if ((!OHDFilesystemUtil::exists(openhd::SECURITY_KEYPAIR_FILENAME)) ||
      (!val)) {
    console->debug("Creating txrx.key from default pw (once)");
    auto keys = wb::generate_keypair_from_bind_phrase(wb::DEFAULT_BIND_PHRASE);
    wb::write_keypair_to_file(keys, openhd::SECURITY_KEYPAIR_FILENAME);
  }
}

void OHDInterface::update_wifi_hotspot_enable() {

}