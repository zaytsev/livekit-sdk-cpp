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

#ifndef LIVEKIT_ROOM_H
#define LIVEKIT_ROOM_H

#include <cstdint>
#include <functional>
#include <mutex>

#include "ffi.pb.h"
#include "handle.pb.h"
#include "livekit/ffi_client.h"
#include "livekit/participant.h"
#include "livekit_ffi.h"
#include "participant.pb.h"
#include "room.pb.h"

namespace livekit {

class DataPacket;
class RoomEventHandler;

// Only create it with Room::Create()
class Room {
 public:
  static std::shared_ptr<Room> Create();

 public:
  Room();
  ~Room();

  void Connect(const std::string& url, const std::string& token);

  void Disconnect();

  void OnTrackPublished(const std::string& name,
                        const std::string& sid,
                        const std::string& inputTrackSid);

  const std::string& GetName() const { return info_.name(); }

  const std::string& GetSid() const { return info_.sid(); }

  const std::string& GetMetadata() const { return info_.metadata(); }

  bool IsConnected() const { return handle_.GetHandleId() != INVALID_HANDLE; }

  std::shared_ptr<LocalParticipant> GetLocalParticipant() const {
    return localParticipant_;
  }

  void SetEventHandler(std::shared_ptr<RoomEventHandler> handler) {
    eventHandler_ = handler;
  }

 private:
  // mutable std::mutex lock_;
  FfiHandle handle_{INVALID_HANDLE};
  FfiHandle lpHandle_{INVALID_HANDLE};  // LocalParticipant handle
  std::shared_ptr<LocalParticipant> localParticipant_{nullptr};
  proto::RoomInfo info_;

  std::shared_ptr<RoomEventHandler> eventHandler_{nullptr};

  FfiClient::ListenerId listenerId_{0};

  void OnConnect(FfiClient::FfiEventPtr& event);

  void OnEvent(FfiClient::FfiEventPtr& event);
};

class DataPacket {
 public:
  DataPacket(const proto::FfiOwnedHandle& owned_handle,
             uint64_t data_ptr,
             uint64_t data_len)
      : handle_{owned_handle.id()},
        data_{reinterpret_cast<std::byte*>(data_ptr),
              static_cast<std::size_t>(data_len)} {}

  const std::span<std::byte>& GetData() const { return data_; }

 private:
  FfiHandle handle_{INVALID_HANDLE};
  std::span<std::byte> data_;
};

class RoomEventHandler {
 public:
  // Connection events
  virtual void OnConnected(Room&) {};
  virtual void OnDisconnected(Room&) {};
  virtual void OnReconnecting(Room&) {};
  virtual void OnReconnected(Room&) {};
  virtual void OnConnectionStateChanged(Room&) {};
  virtual void OnConnectionQualityChanged(Room&) {};
  // Metadata
  virtual void OnRoomMetadataChanged(Room&) {};
  virtual void OnRoomSidChanged(Room&) {};
  // Participant events
  virtual void OnParticipantConnected(Room&, FfiHandle, const proto::ParticipantInfo&) {};
  virtual void OnParticipantDisconnected(Room&, const std::string&) {};
  virtual void OnParticipantMetadataChanged(Room&) {};
  virtual void OnParticipantNameChanged(Room&) {};
  virtual void OnParticipantAttributesChanged(Room&) {};
  virtual void OnActiveSpeakerChanged(Room&) {};
  // Local track events
  virtual void OnLocalTrackPublished(Room&) {};
  virtual void OnLocalTrackUnpublished(Room&) {};
  virtual void OnLocalTrackSubscribed(Room&) {};
  // Track events
  virtual void OnTrackPublished(Room&) {};
  virtual void OnTrackUnpublished(Room&) {};
  virtual void OnTrackSubscribed(Room&) {};
  virtual void OnTrackUnsubscribed(Room&) {};
  virtual void OnTrackSubscriptionFailed(Room&) {};
  virtual void OnTrackMuted(Room&) {};
  virtual void OnTrackUnmuted(Room&) {};
  // Data events
  virtual void OnDataPacketReceived(Room&,
                                    std::optional<const std::string>,
                                    const std::string&,
                                    const DataPacket&) {};
  virtual void OnSipDtfmReceived(Room&) {};
  virtual void OnTranscriptionReceived(Room&) {};
  // E2E encryption event
  virtual void OnE2EeStateChagned(Room&) {};
  //
  virtual void OnEndOfStream(Room&) {};
};
}  // namespace livekit

#endif /* LIVEKIT_ROOM_H */
