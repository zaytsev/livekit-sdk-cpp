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

#include "livekit/room.h"

#include <functional>
#include <iostream>

#include "ffi.pb.h"
#include "livekit/ffi_client.h"
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

  proto::FfiResponse response = FfiClient::getInstance().SendRequest(request);
  connectAsyncId_ = response.connect().async_id();
}

void Room::OnEvent(const proto::FfiEvent& event) {
  proto::ConnectCallback cb = event.connect();
  if (cb.async_id() != connectAsyncId_) {
    return;
  }

  std::cout << "Received ConnectCallback" << std::endl;

  if (!cb.has_error()) {
    handle_ = FfiHandle(cb.room().handle().id());
    info_ = cb.room();
    try {
      lpHandle_ = FfiHandle(cb.local_participant().handle().id());
      localParticipant_ = std::make_shared<LocalParticipant>(lpHandle_, cb.local_participant());
    }
    catch (std::exception& e) {
      std::cerr << "Failed to create local participant: " << e.what() << std::endl;
      return;
    }    

    std::cout << "Connected to room" << std::endl;
    std::cout << "Room SID: " << cb.room().sid() << std::endl;
  } else {
    std::cerr << "Failed to connect to room: " << cb.error() << std::endl;
  }
}

}  // namespace livekit
