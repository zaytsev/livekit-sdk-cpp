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

#include <functional>
#include <iostream>

#include "ffi.pb.h"
#include "handle.pb.h"
#include "livekit/ffi_client.h"
#include "livekit/room.h"
#include "livekit_ffi.h"
#include "room.pb.h"

namespace livekit {
Room::Room() {
  std::cout << "Room::Room" << std::endl;
}

Room::~Room() {
  std::cout << "Room::~Room" << std::endl;
  FfiClient::getInstance().RemoveListener(listenerId_);
}

std::shared_ptr<Room> Room::Create() {
  std::shared_ptr<Room> ptr = std::make_shared<Room>();

  ptr->listenerId_ = FfiClient::getInstance().AddListener(
      std::bind(&Room::OnEvent, ptr, std::placeholders::_1));

  return ptr;
}

void Room::Connect(const std::string& url, const std::string& token) {
  proto::FfiRequest request;
  proto::ConnectRequest* connectRequest = request.mutable_connect();
  connectRequest->set_url(url);
  connectRequest->set_token(token);

  FfiClient::getInstance().SendAsyncRequest(
      request, std::bind(&Room::OnConnect, this, std::placeholders::_1));
}

void Room::Disconnect() {
  proto::FfiRequest request;
  proto::DisconnectRequest* disconnectRequest = request.mutable_disconnect();
  disconnectRequest->set_room_handle(handle_.GetHandleId());

  FfiClient::getInstance().SendAsyncRequest(request,
                                            [this](FfiClient::FfiEventPtr&) {
                                              this->handle_ = INVALID_HANDLE;
                                              this->lpHandle_ = INVALID_HANDLE;
                                              this->localParticipant_.reset();
                                            });
}

void Room::OnConnect(FfiClient::FfiEventPtr& event) {
  assert(event->has_connect());
  auto cb = std::move(event)->connect();

  std::cout << "Received ConnectCallback" << std::endl;

  if (!cb.has_error()) {
    handle_ = FfiHandle(cb.room().handle().id());
    info_ = cb.room().info();
    try {
      lpHandle_ = FfiHandle(cb.local_participant().handle().id());
      localParticipant_ = std::make_shared<LocalParticipant>(
          lpHandle_, cb.local_participant().info());
    } catch (std::exception& e) {
      std::cerr << "Failed to create local participant: " << e.what()
                << std::endl;
      return;
    }

    std::cout << "Connected to room" << std::endl;
    std::cout << "Room SID: " << cb.room().info().sid() << std::endl;
  } else {
    std::cerr << "Failed to connect to room: " << cb.error() << std::endl;
  }
}

void Room::OnEvent(FfiClient::FfiEventPtr& event) {
  std::cout << "Room: processing event: " << event->Utf8DebugString()
            << std::endl;
  if (event->has_room_event() &&
      event->room_event().room_handle() == handle_.GetHandleId()) {
    auto roomEvent = std::move(event)->room_event();
    switch (roomEvent.message_case()) {
      case proto::RoomEvent::kParticipantConnected: {
        if (eventHandler_) {
          const auto& participantEvent = roomEvent.participant_connected().info();
          const auto& info = participantEvent.info();
          const auto participantHandle =
              FfiHandle(roomEvent.participant_connected().info().handle().id());
          eventHandler_->OnParticipantConnected(*this, participantHandle, info);
        }
      } break;
      case proto::RoomEvent::kParticipantDisconnected:
      case proto::RoomEvent::kLocalTrackPublished:
      case proto::RoomEvent::kLocalTrackUnpublished:
      case proto::RoomEvent::kLocalTrackSubscribed:
      case proto::RoomEvent::kTrackPublished:
      case proto::RoomEvent::kTrackUnpublished:
      case proto::RoomEvent::kTrackSubscribed:
      case proto::RoomEvent::kTrackUnsubscribed:
      case proto::RoomEvent::kTrackSubscriptionFailed:
      case proto::RoomEvent::kTrackMuted:
      case proto::RoomEvent::kTrackUnmuted:
      case proto::RoomEvent::kActiveSpeakersChanged:
      case proto::RoomEvent::kRoomMetadataChanged:
      case proto::RoomEvent::kRoomSidChanged:
        info_.set_sid(roomEvent.room_sid_changed().sid());
        break;

      case proto::RoomEvent::kParticipantMetadataChanged:
      case proto::RoomEvent::kParticipantNameChanged:
      case proto::RoomEvent::kParticipantAttributesChanged:
      case proto::RoomEvent::kConnectionQualityChanged:
      case proto::RoomEvent::kConnectionStateChanged:
      case proto::RoomEvent::kDisconnected:
      case proto::RoomEvent::kReconnecting:
      case proto::RoomEvent::kReconnected:
      case proto::RoomEvent::kE2EeStateChanged:
      case proto::RoomEvent::kEos:
        break;

      case proto::RoomEvent::kDataPacketReceived: {
        const auto& dataPacket = roomEvent.data_packet_received();
        switch (dataPacket.value_case()) {
          case proto::DataPacketReceived::kUser: {
            const auto& userData = dataPacket.user();
            const auto& handle = userData.data().handle();
            const auto& data = userData.data().data();

            if (eventHandler_) {
              DataPacket packet{handle, data.data_ptr(), data.data_len()};
              eventHandler_->OnDataPacketReceived(
                  *this, userData.topic(), dataPacket.participant_identity(),
                  packet);
            }
          } break;

          case proto::DataPacketReceived::kSipDtmf:
            // Handle DTMF digit if needed
            // char digit = dataPacket.sip_dtmf().digit();
            break;

          case proto::DataPacketReceived::VALUE_NOT_SET:
            // Handle unset value case if needed
            break;
        }
      } break;
      case proto::RoomEvent::kTranscriptionReceived:
      case proto::RoomEvent::MESSAGE_NOT_SET:
        break;
    }
  }
}
}  // namespace livekit
