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
#include <optional>

namespace nano {

template <typename T>
class event {
   private:
    detail::signal receiver_signal_{detail::signal::make_detached()};
    std::optional<T> result_{std::nullopt};

   public:
    class awaiter_type {
       private:
        detail::optional_view<T> result_;

       public:
        [[nodiscard]] constexpr bool await_ready() const noexcept { return result_.get().has_value(); }
        [[nodiscard]] constexpr T await_resume() const noexcept { return result_.get().value(); }
        constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}

        awaiter_type(detail::optional_view<T> result_view) noexcept : result_{result_view} {}
    };

    template <typename U, std::enable_if_t<std::is_same_v<std::decay_t<U>, T>, T>* = nullptr>
    constexpr void send(U&& value) noexcept {
        receiver_signal_.set(true);
        result_ = std::move(value);
    }

    [[nodiscard]] awaiter_type awaiter(detail::signal receiver_signal) {
        receiver_signal.set(result_.has_value());
        receiver_signal_ = receiver_signal;

        auto result_view = detail::view(result_);
        return awaiter_type{result_view};
    }

    event(const event<T>& other) = delete;
    event<T>& operator=(const event<T>& other) = delete;

    event() = default;
};

}  // namespace nano
