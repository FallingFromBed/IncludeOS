// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#ifndef NET_PORT_UTIL_HPP
#define NET_PORT_UTIL_HPP

#include "inet_common.hpp"
#include <util/fixed_bitmap.hpp>

namespace net {

struct Port_error : public std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

/**
 * @brief      Class for handling a full range of network ports.
 *             Generates ephemeral ports and track what ports are bound or not.
 *             1 means free, 0 means bound (occupied)
 */
class Port_util {
public:
  /**
   * @brief      Construct a port util with a new generated ephemeral port
   *             and a empty port list.
   */
  Port_util()
    : ports(),
      eph_view{ // set the ephemeral view to be between 49152-65535
        ports.data() + port_ranges::DYNAMIC_START,
        (port_ranges::DYNAMIC_END - port_ranges::DYNAMIC_START + 1) / sizeof(MemBitmap::word)
      },
      ephemeral_(net::new_ephemeral_port()),
      eph_count(0)
  {
    // all ports are free
    ports.set_all();
  }

  /**
   * @brief      Gets the next ephemeral port.
   *             increment_ephemeral may throw
   *
   * @return     The next ephemeral port.
   */
  uint16_t get_next_ephemeral()
  {
    increment_ephemeral();
    return ephemeral_;
  }

  /**
   * @brief      Bind a port, making it reserved.
   *
   * @param[in]  port  The port
   */
  void bind(const uint16_t port) noexcept
  {
    ports.reset(port);

    if(port_ranges::is_dynamic(port)) ++eph_count;
  }

  /**
   * @brief      Unbind a port, making it available.
   *
   * @param[in]  port  The port
   */
  void unbind(const uint16_t port) noexcept
  {
    ports.set(port);

    if(port_ranges::is_dynamic(port)) --eph_count;
  }

  /**
   * @brief      Determines if the port is bound.
   *
   * @param[in]  port  The port
   *
   * @return     True if bound, False otherwise.
   */
  bool is_bound(const uint16_t port) const noexcept
  {
    return !ports[port];
  }

  /**
   * @brief      Determines if it has any free ephemeral ports.
   *
   * @return     True if has free ephemeral, False otherwise.
   */
  bool has_free_ephemeral() const noexcept
  { return eph_count < (port_ranges::DYNAMIC_END - port_ranges::DYNAMIC_START); }

private:
  Fixed_bitmap<65536> ports;
  MemBitmap           eph_view;
  uint16_t            ephemeral_;
  uint16_t            eph_count;

  /**
   * @brief      Increment the ephemeral port by one.
   *             Throws if there are no more free ephemeral ports available.
   */
  void increment_ephemeral()
  {
    if(UNLIKELY( not has_free_ephemeral() ))
      throw Port_error{"All ephemeral ports are taken"};

    ephemeral_++;

    // wrap around to dynamic start if end
    if(UNLIKELY(ephemeral_ == 0))
      ephemeral_ = port_ranges::DYNAMIC_START;

    if(UNLIKELY( is_bound(ephemeral_) ))
    {
      auto i = eph_view.first_set();
      Ensures(i != -1 && "Did not found a free ephemeral even tho has_free_ephemeral() == true...");
      ephemeral_ = port_ranges::DYNAMIC_START + i;
    }

    Expects(not is_bound(ephemeral_) && "Generated ephemeral port is already bound. Please fix me!");
  }
}; // < class Port_util

} // < namespace net

#endif
