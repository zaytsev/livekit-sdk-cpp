/*
 * Copyright 2023 LiveKit
 *
 * Licensed under the Apache License, Version 2.0 (the “License”);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an “AS IS” BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIVEKIT_FFI_CLIENT_H
#define LIVEKIT_FFI_CLIENT_H

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include "ffi.pb.h"
#include "livekit/proto.h"
#include "livekit_ffi.h"

namespace livekit {
extern "C" void LivekitFfiCallback(const u_int8_t* buf, size_t len);

// The FfiClient is used to communicate with the FFI interface of the Rust SDK
// We use the generated protocol messages to facilitate the communication
class FfiClient {
 public:
  using FfiEventPtr = std::unique_ptr<proto::FfiEvent>;
  using ListenerId = int;
  using AsyncId = uint64_t;
  using AsyncEventCallback = std::function<void(FfiEventPtr&)>;

  FfiClient(const FfiClient&) = delete;
  FfiClient& operator=(const FfiClient&) = delete;

  static FfiClient& getInstance() {
    static FfiClient instance;
    return instance;
  }

  ListenerId AddListener(const AsyncEventCallback& listener);
  void RemoveListener(ListenerId id);

  proto::FfiResponse SendRequest(const proto::FfiRequest& request) const;
  proto::FfiResponse SendAsyncRequest(const proto::FfiRequest& request, const AsyncEventCallback& listener);
 private:
  std::unordered_map<ListenerId, AsyncEventCallback> listeners_;
  std::unordered_map<AsyncId, AsyncEventCallback> asyncListeners_;
  ListenerId nextListenerId = 1;
  mutable std::mutex lock_;

  std::queue<FfiEventPtr> eventQueue_;
  std::condition_variable eventCV_;
  bool shouldStop_ = false;
  std::thread eventThread_;

  FfiClient();
  ~FfiClient();

  void PushEvent(FfiEventPtr event);
  void ProcessEvents();
  friend void LivekitFfiCallback(const uint8_t* buf, size_t len);
};

struct FfiHandle {
 public:
  FfiHandle(FfiHandleId handleId);
  ~FfiHandle() = default;

  FfiHandleId GetHandleId() const { return *handleId_; }
  bool IsValid() { return handleId_ && *handleId_ != INVALID_HANDLE; }

 private:
  std::shared_ptr<FfiHandleId> handleId_;
};
}  // namespace livekit

#endif /* LIVEKIT_FFI_CLIENT_H */
