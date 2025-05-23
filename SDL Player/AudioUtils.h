#pragma once
#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <cstdint>

/// Returns a default channel layout mask for the given number of channels.
/// Returns 0 if unknown or unsupported.
uint64_t GetDefaultChannelLayout(int channels);

#endif // AUDIO_UTILS_H
