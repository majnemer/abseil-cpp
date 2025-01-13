// Copyright 2018 The Abseil Authors.
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
// This file is a Darwin-specific part of spinlock_wait.cc
#include <os/os_sync_wait_on_address.h>
#include <unistd.h>

#include <atomic>
#include <climits>
#include <cstdint>
#include <ctime>

#include "absl/base/attributes.h"
#include "absl/base/internal/errno_saver.h"

extern "C" {

ABSL_ATTRIBUTE_WEAK void ABSL_INTERNAL_C_SYMBOL(AbslInternalSpinLockDelay)(
    std::atomic<uint32_t> *w, uint32_t value, int,
    absl::base_internal::SchedulingMode) {
  absl::base_internal::ErrnoSaver errno_saver;
  os_sync_wait_on_address(w, static_cast<uint64_t>(value), sizeof(*w),
                          OS_SYNC_WAIT_ON_ADDRESS_NONE);
}

ABSL_ATTRIBUTE_WEAK void ABSL_INTERNAL_C_SYMBOL(AbslInternalSpinLockWake)(
    std::atomic<uint32_t> *w, bool all) {
  if (all) {
    os_sync_wake_by_address_all(w, sizeof(*w), OS_SYNC_WAKE_BY_ADDRESS_NONE);
  } else {
    os_sync_wake_by_address_any(w, sizeof(*w), OS_SYNC_WAKE_BY_ADDRESS_NONE);
  }
}

}  // extern "C"
