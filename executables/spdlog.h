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

#ifndef SPDLOG_HPP_
#define SPDLOG_HPP_

// The goal is to eventually use spdlog throughout all openhd modules, but have
// specific logger(s) on a module basis such that we can enable / disable
// logging for a specific module (e.g. ohd_video: set log level to debug / info)
// when debugging ohd_video.

// #include <spdlog/fwd.h>
// #include <spdlog/fmt/fmt.h>
// #include <spdlog/common.h>
#include <spdlog/spdlog.h>
// # define FMT_STRING(s) s

#include <memory>
#include <mutex>
#include <vector>

namespace openhd::log {

// Note: the _mt loggers have threadsafety by design already, but we need to
// make sure to crete the instance only once For some reason there is no helper
// for that in speeddlog / i haven't found it yet

// Thread-safe but recommended to store result in an intermediate variable
std::shared_ptr<spdlog::logger> create_or_get(const std::string& logger_name);

// Uses the thread-safe create_or_get -> slower than using the intermediate
// variable approach, but sometimes you just don't care about that.
std::shared_ptr<spdlog::logger> get_default();

// these match the mavlink SEVERITY_LEVEL enum, but this code should not depend
// on the mavlink headers See
// https://mavlink.io/en/messages/common.html#MAV_SEVERITY
enum class STATUS_LEVEL {
  EMERGENCY = 0,
  ALERT,
  CRITICAL,
  ERROR,
  WARNING,
  INFO,
  NOTICE,
  DEBUG
};

// Please use sparingly.
void log_to_kernel(const std::string& message);

// Extra logging method to log without pulling in spdlog / fmt
void debug_log(const std::string& message);
void info_log(const std::string& message);
void warning_log(const std::string& message);

}  // namespace openhd::log

#endif  // OPENHD_OPENHD_OHD_COMMON_OPENHD_SPDLOG_HPP_