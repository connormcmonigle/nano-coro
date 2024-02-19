/*
  nano-coro is a minimal coroutine library by Connor McMonigle
  Copyright (C) 2024  Connor McMonigle

  nano-coro is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  nano-coro is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <nano/detail.hpp>

#include <coroutine>

namespace nano {

class yield {
   public:
    using awaiter_type = std::suspend_always;

    [[nodiscard]] std::suspend_always awaiter(detail::signal receiver_signal) {
        receiver_signal.set(true);
        return {};
    }
};

}  // namespace nano
