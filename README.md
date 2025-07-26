# SDL/FFmpeg Media Player
A basic video player built with SDL3, OpenGL, and FFmpeg for learning multimedia programming.

## Overview
This project was one of several media player attempts I built early in my programming journey. It successfully plays video files with synchronized audio using FFmpeg for decoding and SDL3/OpenGL for rendering.

## Features
- **Video Playback**: Decodes and displays video frames using FFmpeg
- **Audio Playback**: Synchronized audio output through SDL3
- **Format Support**: Handles various video formats supported by FFmpeg
- **Hardware Accelerated**: OpenGL 4.6 for efficient video rendering

## Technical Implementation

### Video Pipeline
1. **VideoDecoder**: FFmpeg-based decoder that:
   - Opens video streams
   - Decodes frames from YUV to RGB
   - Handles frame timing
   - Uses libswscale for color conversion

2. **VideoRenderer**: OpenGL renderer that:
   - Uploads RGB frames as textures
   - Renders fullscreen quad with video texture
   - Uses modern OpenGL (4.6) with shaders

### Audio Pipeline
1. **AudioDecoder**: FFmpeg audio decoder that:
   - Extracts audio streams
   - Resamples to 48kHz stereo (SDL's preferred format)
   - Uses libswresample for format conversion

2. **Audio Playback**: SDL3 audio callback system with:
   - Mutex-protected audio buffer
   - Continuous decoding and playback

## Architecture
- Modular design with separate decoder and renderer classes
- Proper synchronization between audio/video threads
- Frame timing based on video framerate
- Double buffering for smooth playback

## Dependencies
- SDL3 (window, input, audio)
- FFmpeg libraries (libavformat, libavcodec, libswscale, libswresample)
- OpenGL 4.6
- GLAD (OpenGL loader)

## Learning Notes
This project taught me:
- FFmpeg's complex API and media handling
- Audio/video synchronization challenges
- OpenGL texture streaming
- Multi-threaded programming with mutexes
- Working with C libraries in C++

## Why I Moved On
While functional, I set this aside because:
- FFmpeg's C API was challenging to work with cleanly
- Media playback became tangential to my game development focus
- Wanted to focus on physics engines and game systems
- The fundamentals learned here (texture streaming, audio handling) carried forward to game projects
