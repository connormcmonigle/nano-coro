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
#include <nano/execution.hpp>
#include <nano/executor.hpp>

#include <coroutine>
#include <optional>

namespace nano {

template <typename T, typename E = execution::eager>
class continuation {
   public:
    using return_type = T;

    class promise_type;
    class awaiter_type;

    using handle_type = std::coroutine_handle<promise_type>;

   private:
    handle_type handle_;

   public:
    class promise_type {
       private:
        coroutine_context context_;
        std::optional<T> result_;

        detail::signal receiver_signal_;
        detail::coroutine_stack_frame_header_view frame_data_view_;

       public:
        [[nodiscard]] constexpr std::optional<T>& get_result() noexcept { return result_; }

        void attach_receiver_signal(detail::signal receiver_signal) noexcept {
            receiver_signal_ = receiver_signal;
            receiver_signal_.set(result_.has_value());
        }

        [[nodiscard]] continuation<T, E> get_return_object() {
            auto coroutine_handle = handle_type::from_promise(*this);
            return continuation<T, E>{coroutine_handle};
        }

        void return_value(T value) {
            receiver_signal_.set(true);
            result_ = std::move(value);
            context_.stack.get().pop();
        }

        template <typename O>
        [[nodiscard]] typename std::decay_t<O>::awaiter_type await_transform(O&& object) noexcept {
            const auto ready_signal = frame_data_view_.get().data().ready_signal();
            return object.awaiter(ready_signal);
        }

        template <typename T1, typename E1>
        [[nodiscard]] typename continuation<T1, E1>::awaiter_type await_transform(continuation<T1, E1>& object) =
            delete;

        template <typename T1, typename E1>
        [[nodiscard]] typename continuation<T1, E1>::awaiter_type await_transform(const continuation<T1, E1>& object) =
            delete;

        constexpr void unhandled_exception() {}
        [[nodiscard]] constexpr execution::await_expression<E>::type initial_suspend() { return {}; }
        [[nodiscard]] constexpr std::suspend_always final_suspend() noexcept { return {}; }

        promise_type(promise_type&& other) = delete;
        promise_type(const promise_type& other) = delete;
        promise_type& operator=(promise_type&& other) = delete;
        promise_type& operator=(const promise_type& other) = delete;

        [[nodiscard]] void* operator new(std::size_t n, coroutine_context context, auto&&...) {
            return context.stack.get().push(n, std::alignment_of_v<promise_type>);
        }

        constexpr promise_type(coroutine_context context, auto&&...) noexcept
            : context_{context},
              result_{std::nullopt},
              receiver_signal_{detail::signal::make_detached()},
              frame_data_view_{context.stack.get().peek_frame_header_view()} {
            const auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
            frame_data_view_.get().data().ready.store(true, std::memory_order_relaxed);
            frame_data_view_.get().data().handle = handle;
        }

        void operator delete(void*) {}
    };

    class awaiter_type {
       private:
        detail::optional_view<T> result_;

       public:
        [[nodiscard]] constexpr bool await_ready() const noexcept { return result_.get().has_value(); }

        [[nodiscard]] constexpr T&& await_resume() noexcept {
            auto& result = result_.get().value();
            return std::move(result);
        }

        constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}

        awaiter_type(detail::optional_view<T> result_view) noexcept : result_{result_view} {}
    };

    [[nodiscard]] awaiter_type awaiter(detail::signal receiver_signal) {
        handle_.promise().attach_receiver_signal(receiver_signal);
        auto result_view = detail::view(handle_.promise().get_result());
        return awaiter_type{result_view};
    }

    continuation(continuation<T, E>&& other) = delete;
    continuation(const continuation<T, E>& other) = delete;

    continuation<T>& operator=(continuation<T, E>&& other) = delete;
    continuation<T>& operator=(const continuation<T, E>& other) = delete;

    continuation(handle_type coroutine_handle) noexcept : handle_{coroutine_handle} {}
    ~continuation() noexcept { handle_.destroy(); }
};

}  // namespace nano
