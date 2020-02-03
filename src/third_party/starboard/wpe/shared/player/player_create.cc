// Copyright 2016 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "starboard/player.h"
#include "third_party/starboard/wpe/shared/player/player_internal.h"
#include "third_party/starboard/wpe/shared/media/gst_media_utils.h"

SbPlayer SbPlayerCreate(SbWindow window,
                        SbMediaVideoCodec video_codec,
                        SbMediaAudioCodec audio_codec,
#if SB_API_VERSION < 10
                        SbMediaTime /* duration_pts */,
#endif  // SB_API_VERSION < 10
                        SbDrmSystem drm_system,
                        const SbMediaAudioSampleInfo* audio_sample_info,
#if SB_API_VERSION >= 11
                        const char* max_video_capabilities,
#endif  // SB_API_VERSION >= 11
                        SbPlayerDeallocateSampleFunc sample_deallocate_func,
                        SbPlayerDecoderStatusFunc decoder_status_func,
                        SbPlayerStatusFunc player_status_func,
                        SbPlayerErrorFunc player_error_func,
                        void* context,
                        SbPlayerOutputMode output_mode,
                        SbDecodeTargetGraphicsContextProvider* provider) {
  if (!sample_deallocate_func || !decoder_status_func || !player_status_func
#if SB_HAS(PLAYER_ERROR_MESSAGE)
      || !player_error_func
#endif  // SB_HAS(PLAYER_ERROR_MESSAGE)
      ) {
    return kSbPlayerInvalid;
  }

  if (audio_codec == kSbMediaAudioCodecNone &&
      video_codec == kSbMediaVideoCodecNone) {
    SB_LOG(ERROR) << "SbPlayerCreate() requires at least one audio track or"
                  << " one video track.";
    return kSbPlayerInvalid;
  }

  if (audio_codec != kSbMediaAudioCodecNone &&
      !third_party::starboard::wpe::shared::media::
      GstRegistryHasElementForMediaType(audio_codec)) {
    SB_LOG(ERROR) << "Unsupported audio codec " << audio_codec;
    return kSbPlayerInvalid;
  }

  if (video_codec != kSbMediaVideoCodecNone &&
      !third_party::starboard::wpe::shared::media::
      GstRegistryHasElementForMediaType(video_codec)) {
    SB_LOG(ERROR) << "Unsupported video codec " << video_codec;
    return kSbPlayerInvalid;
  }

  if (!SbPlayerOutputModeSupported(output_mode, video_codec, drm_system)) {
    SB_LOG(ERROR) << "Unsupported player output mode " << output_mode;
    return kSbPlayerInvalid;
  }

  return new SbPlayerPrivate(
      window, video_codec, audio_codec, drm_system, audio_sample_info,
#if SB_API_VERSION >= 11
      max_video_capabilities,
#endif  // SB_API_VERSION >= 11
      sample_deallocate_func, decoder_status_func, player_status_func,
      player_error_func, context, output_mode, provider);
}
