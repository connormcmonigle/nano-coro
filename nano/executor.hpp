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

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <nano/detail.hpp>

namespace nano {

struct coroutine_context {
    detail::coroutine_stack_view stack;
};

template <typename B, std::size_t N>
class executor {
   private:
    static constexpr std::size_t context_stack_count = N;
    using buffer_type = B;

    std::array<detail::buffer_and_coroutine_stack<B>, N> stacks_;

    [[nodiscard]] constexpr bool execution_complete_() const noexcept {
        return std::all_of(stacks_.begin(), stacks_.end(), [](const auto& elem) { return elem.stack().empty(); });
    }

    constexpr void step_() noexcept {
        for (const auto& elem : stacks_) {
            const auto& stack = elem.stack();
            if (stack.empty() || !stack.peek_frame_header().data().ready.load(std::memory_order_relaxed)) { continue; }

            stack.peek_frame_header().data().handle.resume();
        }
    }

   public:
    [[nodiscard]] coroutine_context find_available_context() noexcept {
        const auto iter =
            std::find_if(stacks_.begin(), stacks_.end(), [](const auto& elem) { return elem.stack().empty(); });

        return coroutine_context{iter->stack_view()};
    }

    [[nodiscard]] constexpr bool execution_complete() const noexcept { return execution_complete_(); }

    constexpr void step() noexcept { step_(); }

    constexpr void wait() noexcept {
        for (;;) {
            if (execution_complete_()) { return; }
            step_();
        }
    }
};

}  // namespace nano
