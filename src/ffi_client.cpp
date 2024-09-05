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

#include <cassert>
#include <cstdint>
#include <memory>

#include "ffi.pb.h"
#include "handle.pb.h"
#include "livekit/ffi_client.h"
#include "livekit_ffi.h"
#include "room.pb.h"

namespace livekit {

FfiClient::FfiClient() : eventThread_(&FfiClient::ProcessEvents, this) {
  livekit_ffi_initialize(&LivekitFfiCallback, false);
}

FfiClient::~FfiClient() {
  {
    std::lock_guard<std::mutex> guard(lock_);
    shouldStop_ = true;
  }
  eventCV_.notify_one();
  if (eventThread_.joinable()) {
    eventThread_.join();
  }
}

FfiClient::ListenerId FfiClient::AddListener(
    const FfiClient::AsyncEventCallback& listener) {
  std::lock_guard<std::mutex> guard(lock_);
  FfiClient::ListenerId id = nextListenerId++;
  listeners_[id] = listener;
  return id;
}

void FfiClient::RemoveListener(ListenerId id) {
  std::lock_guard<std::mutex> guard(lock_);
  listeners_.erase(id);
}

proto::FfiResponse FfiClient::SendRequest(
    const proto::FfiRequest& request) const {
  size_t len = request.ByteSizeLong();
  uint8_t* buf = new uint8_t[len];
  assert(request.SerializeToArray(buf, len));

  const uint8_t** res_ptr = new const uint8_t*;
  size_t* res_len = new size_t;

  auto handle = livekit_ffi_request(buf, len, res_ptr, res_len);

  delete[] buf;
  if (handle == INVALID_HANDLE) {
    delete res_ptr;
    delete res_len;
    throw std::runtime_error(
        "failed to send request, received an invalid handle");
  }
  FfiHandle _handle(handle);

  proto::FfiResponse response;
  assert(response.ParseFromArray(*res_ptr, *res_len));
  delete res_ptr;
  delete res_len;

  return response;
}

proto::FfiResponse FfiClient::SendAsyncRequest(const proto::FfiRequest& request,
                                               const AsyncEventCallback& listener) {
  proto::FfiResponse response = this->SendRequest(request);
  AsyncId listenerId = 0;

  switch (response.message_case()) {
    case proto::FfiResponse::kDispose:
      listenerId = response.dispose().async_id();
      break;
    case proto::FfiResponse::kConnect:
      listenerId = response.connect().async_id();
      break;
    case proto::FfiResponse::kDisconnect:
      listenerId = response.disconnect().async_id();
      break;
    case proto::FfiResponse::kPublishTrack:
      listenerId = response.publish_track().async_id();
      break;
    case proto::FfiResponse::kUnpublishTrack:
      listenerId = response.unpublish_track().async_id();
      break;
    case proto::FfiResponse::kPublishData:
      listenerId = response.publish_data().async_id();
      break;
    case proto::FfiResponse::kSetLocalMetadata:
      listenerId = response.set_local_metadata().async_id();
      break;
    case proto::FfiResponse::kSetLocalName:
      listenerId = response.set_local_name().async_id();
      break;
    case proto::FfiResponse::kSetLocalAttributes:
      listenerId = response.set_local_attributes().async_id();
      break;
    case proto::FfiResponse::kGetSessionStats:
      listenerId = response.get_session_stats().async_id();
      break;
    case proto::FfiResponse::kPublishTranscription:
      listenerId = response.publish_transcription().async_id();
      break;
    case proto::FfiResponse::kPublishSipDtmf:
      listenerId = response.publish_sip_dtmf().async_id();
      break;
    case proto::FfiResponse::kGetStats:
      listenerId = response.get_stats().async_id();
      break;
    case proto::FfiResponse::kCaptureAudioFrame:
      listenerId = response.capture_audio_frame().async_id();
      break;
    case proto::FfiResponse::kSetSubscribed:
    case proto::FfiResponse::kCreateVideoTrack:
    case proto::FfiResponse::kCreateAudioTrack:
    case proto::FfiResponse::kLocalTrackMute:
    case proto::FfiResponse::kEnableRemoteTrack:
    case proto::FfiResponse::kNewVideoStream:
    case proto::FfiResponse::kNewVideoSource:
    case proto::FfiResponse::kCaptureVideoFrame:
    case proto::FfiResponse::kVideoConvert:
    case proto::FfiResponse::kNewAudioStream:
    case proto::FfiResponse::kNewAudioSource:
    case proto::FfiResponse::kNewAudioResampler:
    case proto::FfiResponse::kRemixAndResample:
    case proto::FfiResponse::kE2Ee:
    case proto::FfiResponse::MESSAGE_NOT_SET:
      return response;
  }

  if (listenerId && listener) {
    std::lock_guard<std::mutex> guard(lock_);
    asyncListeners_[listenerId] = listener;
  }

  return response;
}

void FfiClient::PushEvent(FfiEventPtr event) {
  std::lock_guard<std::mutex> guard(lock_);
  eventQueue_.push(std::move(event));
  eventCV_.notify_one();
}

void LivekitFfiCallback(const uint8_t* buf, size_t len) {
  auto event = std::make_unique<proto::FfiEvent>();
  assert(event->ParseFromArray(buf, len));
  FfiClient::getInstance().PushEvent(std::move(event));
}

std::optional<uint64_t> GetEventAsyncId(const FfiClient::FfiEventPtr& event) {
  switch (event->message_case()) {
    case proto::FfiEvent::kRoomEvent:
      switch (event->room_event().message_case()) {
        case proto::RoomEvent::kParticipantConnected:
          break;
        default:
          break;
      }
      return std::nullopt;
    case proto::FfiEvent::kConnect:
      return event->connect().async_id();
    case proto::FfiEvent::kDisconnect:
      return event->disconnect().async_id();
    case proto::FfiEvent::kDispose:
      return event->dispose().async_id();
    case proto::FfiEvent::kPublishTrack:
      return event->publish_track().async_id();
    case proto::FfiEvent::kUnpublishTrack:
      return event->unpublish_track().async_id();
    case proto::FfiEvent::kPublishData:
      return event->publish_data().async_id();
    case proto::FfiEvent::kPublishTranscription:
      return event->publish_transcription().async_id();
    case proto::FfiEvent::kCaptureAudioFrame:
      return event->capture_audio_frame().async_id();
    case proto::FfiEvent::kSetLocalMetadata:
      return event->set_local_metadata().async_id();
    case proto::FfiEvent::kSetLocalName:
      return event->set_local_name().async_id();
    case proto::FfiEvent::kSetLocalAttributes:
      return event->set_local_attributes().async_id();
    case proto::FfiEvent::kGetStats:
      return event->get_stats().async_id();
    case proto::FfiEvent::kGetSessionStats:
      return event->get_session_stats().async_id();
    case proto::FfiEvent::kPublishSipDtmf:
      return event->publish_sip_dtmf().async_id();

    case proto::FfiEvent::kTrackEvent:
    case proto::FfiEvent::kVideoStreamEvent:
    case proto::FfiEvent::kAudioStreamEvent:
    case proto::FfiEvent::kLogs:
    case proto::FfiEvent::kPanic:
    case proto::FfiEvent::MESSAGE_NOT_SET:
      return std::nullopt;
  }
  return std::nullopt;
}

void FfiClient::ProcessEvents() {
  while (!shouldStop_) {
    FfiEventPtr event;
    {
      std::unique_lock<std::mutex> lock(lock_);
      eventCV_.wait(lock,
                    [this] { return !eventQueue_.empty() || shouldStop_; });
      if (shouldStop_ && eventQueue_.empty()) {
        break;
      }
      event = std::move(eventQueue_.front());
      eventQueue_.pop();
    }

    if (event) {
      auto listenerId = GetEventAsyncId(event);
      if (listenerId) {
        decltype(asyncListeners_)::node_type listener;
        {
          std::lock_guard<std::mutex> guard(lock_);
          listener = asyncListeners_.extract(listenerId.value());
        }
        if (!listener.empty()) {
          listener.mapped()(event);
        }
      }

      for (const auto& [_, listener] : listeners_) {
        if (event) {
          listener(event);
        } else {
          break;
        }
      }
    }
  }
}

FfiHandle::FfiHandle(FfiHandleId id)
    : handleId_(new FfiHandleId(id), [](uintptr_t* ptr) {
        // Only destroy the handle if it's valid
        if (ptr && *ptr != INVALID_HANDLE) {
          assert(livekit_ffi_drop_handle(*ptr));
        }
        delete ptr;  // Delete the pointer after calling the deleter function
      }) {}

}  // namespace livekit
