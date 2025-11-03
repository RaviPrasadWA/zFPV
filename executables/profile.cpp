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

#include "profile.h"

#include <sstream>

#include "spdlog.h"
#include "spdlog_include.h"

OHDProfile DProfile::discover(bool is_air) {
  openhd::log::get_default()->debug("Profile:[{}]", is_air ? "AIR" : "GND");
  // We read the unit id from the persistent storage, later write it to the tmp
  // storage json
  const auto unit_id = "899i9wd99ms2i8inii";
  return OHDProfile(is_air, unit_id);
}