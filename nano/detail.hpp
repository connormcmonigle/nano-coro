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

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <memory>
#include <new>
#include <optional>

namespace nano {

namespace detail {

template <typename T>
struct view {
    T* view_;

    [[nodiscard]] constexpr T& get() noexcept { return *view_; }
    [[nodiscard]] constexpr const T& get() const noexcept { return *view_; }

    constexpr view(T& ref) noexcept : view_{&ref} {}
};

template <typename T>
using optional_view = view<std::optional<T>>;

struct signal {
    static inline thread_local std::atomic_bool detached_target_ = false;

    std::atomic_bool* state_;

    void set(const bool value) noexcept { state_->store(value, std::memory_order_relaxed); }
    signal(std::atomic_bool& ref) noexcept : state_{&ref} {}

    static signal make_detached() { return signal(detached_target_); };
};

template <typename T>
struct data_stack_frame_header {
    using data_type = T;

    data_stack_frame_header<T>* ancestor_frame_header_;
    std::byte* ancestor_tail_{nullptr};

    T data_{};

    [[nodiscard]] const T& data() const noexcept { return data_; }
    [[nodiscard]] T& data() noexcept { return data_; }

    [[nodiscard]] std::byte* ancestor_tail() const noexcept { return ancestor_tail_; }
    [[nodiscard]] data_stack_frame_header<T>* ancestor_frame_header() const noexcept { return ancestor_frame_header_; }

    data_stack_frame_header(data_stack_frame_header<T>* ancestor_frame_header, std::byte* ancestor_tail) noexcept
        : ancestor_frame_header_{ancestor_frame_header}, ancestor_tail_{ancestor_tail} {}
};

template <typename T>
struct data_stack {
    data_stack_frame_header<T>* current_frame_header_{nullptr};

    std::byte* tail_;
    std::byte* end_;
    std::size_t space_;

    [[nodiscard]] void* allocate_(const std::size_t size, const std::size_t align) noexcept {
        void* unaligned_ptr = tail_;
        std::byte* aligned_ptr = reinterpret_cast<std::byte*>(std::align(align, size, unaligned_ptr, space_));

        if (!aligned_ptr) { return nullptr; }

        tail_ = aligned_ptr + size;
        space_ -= size;

        return std::launder(aligned_ptr);
    }

    [[nodiscard]] constexpr bool empty() const noexcept { return current_frame_header_ == nullptr; }
    [[nodiscard]] constexpr const data_stack_frame_header<T>& peek_frame_header() const noexcept {
        return *current_frame_header_;
    }

    [[nodiscard]] constexpr data_stack_frame_header<T>& peek_frame_header() noexcept { return *current_frame_header_; }

    [[nodiscard]] constexpr view<data_stack_frame_header<T>> peek_frame_header_view() noexcept {
        return view(*current_frame_header_);
    }

    [[nodiscard]] void* push(const std::size_t size, const std::size_t align) {
        constexpr std::size_t frame_header_size = sizeof(data_stack_frame_header<T>);
        constexpr std::size_t frame_header_align = std::alignment_of_v<data_stack_frame_header<T>>;

        data_stack_frame_header<T>* ancestor_frame_header = current_frame_header_;
        std::byte* ancestor_tail = tail_;

        void* header_place = allocate_(frame_header_size, frame_header_align);
        current_frame_header_ = new (header_place) data_stack_frame_header<T>(ancestor_frame_header, ancestor_tail);

        return allocate_(size, align);
    }

    void pop() {
        auto& header = *current_frame_header_;

        current_frame_header_ = header.ancestor_frame_header();
        tail_ = header.ancestor_tail();

        header.~data_stack_frame_header();
        space_ = std::distance(tail_, end_);
    }

    [[nodiscard]] view<data_stack<T>> view_of() noexcept { return view(*this); }

    data_stack<T>& operator=(const data_stack<T>& other) = delete;
    data_stack(const data_stack<T>& other) = delete;

    data_stack<T>& operator=(const data_stack<T>&& other) = delete;
    data_stack(const data_stack<T>&& other) = delete;

    data_stack(std::byte* data, const std::size_t n) noexcept : tail_{data}, end_{data + n}, space_{n} {}

    template <typename U>
    [[nodiscard]] static constexpr data_stack<T> from_buffer(U& buffer) {
        return data_stack(buffer.data(), buffer.size());
    }
};

struct coroutine_frame_data {
    std::atomic_bool ready;
    std::coroutine_handle<> handle;

    signal ready_signal() noexcept { return signal(ready); }
};

using coroutine_stack_frame_header = data_stack_frame_header<coroutine_frame_data>;
using coroutine_stack_frame_header_view = view<coroutine_stack_frame_header>;

using coroutine_stack = data_stack<coroutine_frame_data>;
using coroutine_stack_view = view<coroutine_stack>;

template <typename B>
struct buffer_and_coroutine_stack {
    using buffer_type = B;
    B buffer_;
    coroutine_stack stack_;

    [[nodiscard]] coroutine_stack_view stack_view() noexcept { return stack_.view_of(); }
    [[nodiscard]] coroutine_stack& stack() noexcept { return stack_; }
    [[nodiscard]] const coroutine_stack& stack() const noexcept { return stack_; }

    buffer_and_coroutine_stack() noexcept : buffer_{}, stack_(buffer_.data(), buffer_.size()) {}
};

}  // namespace detail

}  // namespace nano
