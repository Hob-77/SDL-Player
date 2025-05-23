#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/mem.h>
}

#include <string>
#include <vector>

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    bool openFile(const std::string& filename);
    bool decodeNextFrame(std::vector<uint8_t>& out_buffer);

    int getSampleRate() const;
    int getChannels() const;
    AVSampleFormat getSampleFormat() const;

private:
    AVFormatContext* fmt_ctx;
    AVCodecContext* codec_ctx;
    SwrContext* swr_ctx;
    AVPacket* packet;
    AVFrame* frame;
    int audio_stream_index;
};

#endif // AUDIO_DECODER_H