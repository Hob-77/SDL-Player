# SDL/FFmpeg Media Player (Failed Attempt)
A non-functional video player attempt built with SDL3, OpenGL, and FFmpeg while learning multimedia programming.

## Overview
This was one of several failed media player attempts early in my programming journey. While the video playback works, the audio is broken (screechy/distorted) and lacks proper synchronization.

## What Works
- Video decoding and display via FFmpeg/OpenGL
- Basic frame timing for video
- File opening and stream detection

## What's Broken
- **Audio Playback**: Produces screechy/distorted sound
- **No Audio Master Clock**: Missing proper A/V synchronization
- **Buffer Management**: Poor audio buffer handling causing artifacts
- **Timing Issues**: No proper PTS-based synchronization

## Technical Issues
The main problems stem from:
- Incorrect audio resampling or format conversion
- No audio master clock implementation
- Poor understanding of FFmpeg's audio pipeline at the time
- Mismatched audio buffer sizes or sample rates
- Missing proper audio/video synchronization logic

## Why This Failed
- FFmpeg's audio API is complex and I didn't fully understand it
- Tried to implement without understanding PTS (Presentation Time Stamps)
- No proper audio clock to sync video against
- Buffer underruns/overruns causing the screech

## Lessons Learned
This failure taught me:
- Audio programming is harder than video
- Synchronization requires a master clock (usually audio)
- Buffer management is critical for real-time audio
- Need to understand the entire pipeline before implementing
