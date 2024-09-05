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

#include <cmath>
#include <cstdio>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include "livekit/ffi_client.h"
#include "livekit/livekit.h"
#include "livekit/room.h"
#include "participant.pb.h"
#include "room.pb.h"
#include "track.pb.h"

using namespace livekit;

const std::string URL = "ws://localhost:7880";
const std::string TOKEN_TEST =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJleHAiOjE3MzEzODIyNTksImlzcyI6IkFQSXR2V1NzblpvYUQ5dyIsIm5hbWUiOiJ0ZXN0Ii"
    "wibmJmIjoxNzI1MzgyMjU5LCJzdWIiOiJ0ZXN0IiwidmlkZW8iOnsicm9vbSI6InRlc3QiLCJy"
    "b29tSm9pbiI6dHJ1ZX19.9kqBY8uWeAJhS49P1SrK110A48708_eH5jzrCw90O14";

const std::string TOKEN_FOO =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJleHAiOjE3MzE0MTY5NTcsImlzcyI6IkFQSXR2V1NzblpvYUQ5dyIsIm5hbWUiOiJmb28iLC"
    "JuYmYiOjE3MjU0MTY5NTcsInN1YiI6ImZvbyIsInZpZGVvIjp7InJvb20iOiJ0ZXN0Iiwicm9v"
    "bUpvaW4iOnRydWV9fQ.eCcbU7QwpGsacj9q1VYgWGSdHOpfY4R4_f1TOw2ICk0";

const std::string TOKEN_BAR =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJleHAiOjE3MzE0MTY5NzUsImlzcyI6IkFQSXR2V1NzblpvYUQ5dyIsIm5hbWUiOiJiYXIiLC"
    "JuYmYiOjE3MjU0MTY5NzUsInN1YiI6ImJhciIsInZpZGVvIjp7InJvb20iOiJ0ZXN0Iiwicm9v"
    "bUpvaW4iOnRydWV9fQ.72COoHOSj8ub3PI7KEIlP7PVkQ8USJG2OpxzRNdUQGA";

class RoomManager: public RoomEventHandler {
  public:
    void OnParticipantConnected(Room&, FfiHandle, const proto::ParticipantInfo& info) override {
      std::cout << ">>> New user " << info.name() << std::endl;
    }

    void OnDataPacketReceived(Room&, std::optional<const std::string>, const std::string& sender, const DataPacket& packet) override {
        const char* charPtr =
            reinterpret_cast<const char*>(packet.GetData().data());
        auto message = std::string(charPtr, packet.GetData().size());
      std::cout << sender << ": " << message << std::endl;
    }
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <argument>" << std::endl;
    return 1;
  }

  std::string arg = argv[1];

  auto token = TOKEN_TEST;
  if (arg == "foo") {
    token = TOKEN_FOO;
  } else if (arg == "bar") {
    token = TOKEN_BAR;
  } 

  std::shared_ptr<Room> room = Room::Create();

  auto roomManager = std::shared_ptr<RoomManager>();
  room->SetEventHandler(roomManager);

  room->Connect(URL, token);

  // TODO Non blocking ?
  while (!room->GetLocalParticipant()) {
  }

  std::string input;
  while (true) {
    std::cout << ": ";
    std::getline(std::cin, input);

    if (input == "/bye") {
      break;
    }

    auto data = std::make_shared<std::string>(input);
    room->GetLocalParticipant()->PublishData("test", data);
  }

  return 0;
}
