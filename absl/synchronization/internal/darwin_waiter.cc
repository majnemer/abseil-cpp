// Copyright 2023 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "absl/synchronization/internal/darwin_waiter.h"

#ifdef ABSL_INTERNAL_HAVE_DARWIN_WAITER
#include <os/os_sync_wait_on_address.h>

#include <atomic>
#include <cerrno>
#include <cstdint>

#include "absl/base/config.h"
#include "absl/base/internal/raw_logging.h"
#include "absl/base/internal/thread_identity.h"
#include "absl/base/optimization.h"
#include "absl/synchronization/internal/kernel_timeout.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace synchronization_internal {

#ifdef ABSL_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr char DarwinWaiter::kName[];
#endif

int DarwinWaiter::WaitUntil(std::atomic<int32_t>* v, int32_t val,
                            KernelTimeout t) {
  int ret;
  if (t.has_timeout()) {
    uint64_t timeout_ns = static_cast<uint64_t>(
        std::max<int64_t>(t.ToChronoDuration().count(), 1));
    ret = os_sync_wait_on_address_with_timeout(
        v, static_cast<uint64_t>(val), sizeof(*v), OS_SYNC_WAIT_ON_ADDRESS_NONE,
        OS_CLOCK_MACH_ABSOLUTE_TIME, timeout_ns);
  } else {
    ret = os_sync_wait_on_address(v, static_cast<uint64_t>(val), sizeof(*v),
                                  OS_SYNC_WAIT_ON_ADDRESS_NONE);
  }
  if (ret < 0) {
    return -errno;
  }
  return 0;
}

bool DarwinWaiter::Wait(KernelTimeout t) {
  // Loop until we can atomically decrement a wakeup from a positive
  // value, waiting on its address while we believe it is zero.
  // Note that, since the thread ticker is just reset, we don't need to check
  // whether the thread is idle on the very first pass of the loop.
  bool first_pass = true;
  while (true) {
    int32_t x = wakeup_count_.load(std::memory_order_relaxed);
    while (x != 0) {
      if (!wakeup_count_.compare_exchange_weak(
              x, x - 1, std::memory_order_acquire, std::memory_order_relaxed)) {
        continue;  // Raced with someone, retry.
      }
      return true;  // Consumed a wakeup, we are done.
    }

    if (!first_pass) MaybeBecomeIdle();
    const int err = WaitUntil(&wakeup_count_, 0, t);
    if (err != 0) {
      if (err == -EINTR || err == -EFAULT) {
        // Do nothing, the loop will retry.
      } else if (err == -ETIMEDOUT) {
        return false;
      } else {
        ABSL_RAW_LOG(FATAL,
                     "os_sync_wait_on_address operation failed with error %d\n",
                     err);
      }
    }
    first_pass = false;
  }
}

void DarwinWaiter::Post() {
  if (wakeup_count_.fetch_add(1, std::memory_order_release) == 0) {
    // We incremented from 0, need to wake a potential waiter.
    Poke();
  }
}

void DarwinWaiter::Poke() {
  // Wake one thread waiting on the address.
  int ret = os_sync_wake_by_address_any(&wakeup_count_, sizeof(wakeup_count_),
                                        OS_SYNC_WAKE_BY_ADDRESS_NONE);
  if (ABSL_PREDICT_FALSE(ret < 0 && errno != ENOENT)) {
    ABSL_RAW_LOG(FATAL,
                 "os_sync_wake_by_address_any operation failed with error %d\n",
                 ret);
  }
}

}  // namespace synchronization_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_INTERNAL_HAVE_DARWIN_WAITER
