#include "AudioUtils.h"
extern "C" {
#include <libavutil/channel_layout.h>
}

uint64_t GetDefaultChannelLayout(int channels) {
    switch (channels) {
    case 1: return AV_CH_LAYOUT_MONO;
    case 2: return AV_CH_LAYOUT_STEREO;
    case 3: return AV_CH_LAYOUT_2_1;
    case 4: return AV_CH_LAYOUT_QUAD;
    case 5: return AV_CH_LAYOUT_5POINT0_BACK;
    case 6: return AV_CH_LAYOUT_5POINT1_BACK;
        // Add more as needed
    default: return AV_CH_LAYOUT_STEREO;
    }
}
