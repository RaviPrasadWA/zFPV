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

#include "spdlog.h"

#include <spdlog/common.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <mutex>

#include "utils.h"


std::shared_ptr<spdlog::logger> openhd::log::create_or_get(
    const std::string& logger_name) {
  static std::mutex logger_mutex2{};
  std::lock_guard<std::mutex> guard(logger_mutex2);
  auto ret = spdlog::get(logger_name);
  if (ret == nullptr) {
    auto created = spdlog::stdout_color_mt(logger_name);
    assert(created);
    created->set_level(spdlog::level::debug);
    return created;
  }
  return ret;
}

std::shared_ptr<spdlog::logger> openhd::log::get_default() {
  return create_or_get("default");
}


void openhd::log::log_to_kernel(const std::string& message) {
  OHDUtil::run_command(fmt::format("echo \"{}\" > /dev/kmsg", message), {},
                       false);
}

void openhd::log::debug_log(const std::string& message) {
  openhd::log::get_default()->debug(message);
}
void openhd::log::info_log(const std::string& message) {
  openhd::log::get_default()->info(message);
}
void openhd::log::warning_log(const std::string& message) {
  openhd::log::get_default()->warn(message);
}