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

#include <nano/continuation.hpp>
#include <nano/detail.hpp>
#include <nano/executor.hpp>
#include <nano/yield.hpp>

namespace nano {

template <typename T>
class lock_guard {
   private:
    T* mutex_;

   public:
    lock_guard(lock_guard<T>&& other) {
        mutex_ = other.mutex_;
        other.mutex_ = nullptr;
    }

    lock_guard<T>& operator=(lock_guard<T>&& other) {
        mutex_ = other.mutex_;
        other.mutex_ = nullptr;
        return *this;
    }

    lock_guard<T>& operator=(const lock_guard<T>& other) = delete;
    lock_guard(lock_guard<T>& other) = delete;

    lock_guard(T* mutex) noexcept : mutex_{mutex} {
        if (mutex_ != nullptr) { mutex_->available_ = false; }
    }

    ~lock_guard() noexcept {
        if (mutex_ != nullptr) { mutex_->available_ = true; }
    }
};

template <typename T>
continuation<lock_guard<T>> lock(coroutine_context, T& mutex) {
    while (!mutex.available_) { co_await yield(); }

    mutex.available_ = false;
    co_return lock_guard(&mutex);
}

class mutex {
   private:
    bool available_{true};

    friend class lock_guard<mutex>;
    friend continuation<lock_guard<mutex>> lock<mutex>(coroutine_context, mutex&);
};

}  // namespace nano
