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

#ifndef LIVEKIT_TRACK_H
#define LIVEKIT_TRACK_H

#include <iostream>

#include "livekit/ffi_client.h"
#include "track.pb.h"
// #include "video_source.h"
// #include "audio_source.h"

namespace livekit {
class LocalParticipant;

class Track {
 public:
  // Use a move constructor to avoid copying the parameters
  Track(FfiHandle& ffiHandle, const proto::TrackInfo& info)
      : ffiHandle_(std::move(ffiHandle)), info_(info) {
    std::cout << "Track::Track" << std::endl;
  }

  ~Track() { std::cout << "Track::~Track" << std::endl; }

  std::string GetSid() const { return info_.sid(); }
  std::string GetName() const { return info_.name(); }
  proto::TrackKind Getkind() const { return info_.kind(); }
  proto::StreamState GetStreamState() const { return info_.stream_state(); }
  bool IsMuted() const { return info_.muted(); }

 private:
  friend LocalParticipant;

  FfiHandle ffiHandle_;
  proto::TrackInfo info_;
};

class LocalAudioTrack : public Track {};

class LocalVideoTrack : public Track {
 public:
  // Use a move constructor to avoid copying the parameters
  LocalVideoTrack(FfiHandle& ffiHandle, const proto::TrackInfo& info)
      : Track(ffiHandle, info) {}

  // static std::unique_ptr<LocalVideoTrack> CreateVideoTrack(
  //     const std::string& name,
  //     const VideoSource& source);
};

class RemoteVideoTrack : public Track {
  RemoteVideoTrack(FfiHandle& ffiHandle, const proto::TrackInfo& info)
      : Track(ffiHandle, info) {}
};
}  // namespace livekit

#endif /* LIVEKIT_TRACK_H */
