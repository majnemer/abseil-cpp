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
//

#ifndef ABSL_SYNCHRONIZATION_INTERNAL_DARWIN_WAITER_H_
#define ABSL_SYNCHRONIZATION_INTERNAL_DARWIN_WAITER_H_

#include <atomic>
#include <cstdint>

#include "absl/base/config.h"
#include "absl/synchronization/internal/kernel_timeout.h"
#include "absl/synchronization/internal/waiter_base.h"

#ifdef ABSL_HAVE_OS_SYNC_ADDRESS

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace synchronization_internal {

#define ABSL_INTERNAL_HAVE_DARWIN_WAITER 1

class DarwinWaiter : public WaiterCrtp<DarwinWaiter> {
 public:
  DarwinWaiter() : wakeup_count_(0) {}

  bool Wait(KernelTimeout t);
  void Post();
  void Poke();

  static constexpr char kName[] = "DarwinWaiter";

 private:
  // Atomically check that `*v == val`, and if it is, then sleep until the
  // timeout `t` has been reached, or until woken by `Wake()`.
  static int WaitUntil(std::atomic<int32_t>* v, int32_t val, KernelTimeout t);

  std::atomic<int32_t> wakeup_count_;
};

}  // namespace synchronization_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_HAVE_OS_SYNC_ADDRESS

#endif  // ABSL_SYNCHRONIZATION_INTERNAL_DARWIN_WAITER_H_
