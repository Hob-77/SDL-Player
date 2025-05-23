#include "AudioDecoder.h"
#include "AudioUtils.h"
#include <iostream>
#include <stdexcept>

AudioDecoder::AudioDecoder()
    : fmt_ctx(nullptr), codec_ctx(nullptr), swr_ctx(nullptr),
    packet(nullptr), frame(nullptr), audio_stream_index(-1) {
}

AudioDecoder::~AudioDecoder() {
    av_packet_free(&packet);
    av_frame_free(&frame);
    swr_free(&swr_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
}

bool AudioDecoder::openFile(const std::string& filename) {
    if (avformat_open_input(&fmt_ctx, filename.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open input file: " << filename << "\n";
        return false;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        std::cerr << "Failed to find stream info\n";
        return false;
    }

    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }

    if (audio_stream_index == -1) {
        std::cerr << "No audio stream found\n";
        return false;
    }

    AVCodecParameters* codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Audio decoder not found\n";
        return false;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) return false;

    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
        std::cerr << "Failed to copy codec parameters\n";
        return false;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Failed to open codec\n";
        return false;
    }

    std::cout << "Audio stream opened successfully!\n";
    std::cout << "Sample rate: " << codec_ctx->sample_rate << "\n";

    // Number of channels
    std::cout << "Channels: " << codec_ctx->ch_layout.nb_channels << "\n";

    // Print human-readable channel layout
    char layoutStr[256];
    if (av_channel_layout_describe(&codec_ctx->ch_layout, layoutStr, sizeof(layoutStr)) >= 0) {
        std::cout << "Channel layout: " << layoutStr << "\n";
    }
    else {
        std::cout << "Channel layout: (unknown)\n";
    }

    // Print sample format name
    std::cout << "Sample format: " << av_get_sample_fmt_name(codec_ctx->sample_fmt) << "\n";

    packet = av_packet_alloc();
    frame = av_frame_alloc();

    // Get channel layout from codec context
    if (!codec_ctx->ch_layout.nb_channels) {
        std::cerr << "Invalid number of audio channels\n";
        return false;
    }

    // SDL wants stereo, 48000Hz, 16-bit signed
    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, 2);

    // Initialize swr
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        std::cerr << "Failed to allocate SwrContext\n";
        return false;
    }

    if (swr_alloc_set_opts2(
        &swr_ctx,
        &out_layout,
        AV_SAMPLE_FMT_S16,
        48000,
        &codec_ctx->ch_layout,
        codec_ctx->sample_fmt,
        codec_ctx->sample_rate,
        0,
        nullptr) < 0) {
        std::cerr << "Failed to configure resampler\n";
        return false;
    }

    if (swr_init(swr_ctx) < 0) {
        std::cerr << "Failed to initialize resampler\n";
        return false;
    }

    return true;
}

bool AudioDecoder::decodeNextFrame(std::vector<uint8_t>& out_buffer) {
    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            if (avcodec_send_packet(codec_ctx, packet) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    int out_channels = 2;
                    int bytes_per_sample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
                    int max_dst_nb_samples = av_rescale_rnd(
                        swr_get_delay(swr_ctx, codec_ctx->sample_rate) + frame->nb_samples,
                        48000, codec_ctx->sample_rate, AV_ROUND_UP);

                    int total_size = max_dst_nb_samples * out_channels * bytes_per_sample;
                    out_buffer.resize(total_size);

                    uint8_t* out_ptrs[1] = { out_buffer.data() };

                    int converted = swr_convert(
                        swr_ctx,
                        out_ptrs,
                        max_dst_nb_samples,
                        (const uint8_t**)frame->data,
                        frame->nb_samples
                    );

                    if (converted < 0) {
                        std::cerr << "Failed to convert audio samples\n";
                        av_packet_unref(packet);
                        return false;
                    }

                    // Resize buffer to match actual output size
                    int used_size = converted * out_channels * bytes_per_sample;
                    out_buffer.resize(used_size);

                    av_packet_unref(packet);
                    return true;
                }
            }
        }
        av_packet_unref(packet);
    }

    return false;
}

int AudioDecoder::getSampleRate() const {
    return 48000;
}

int AudioDecoder::getChannels() const {
    return 2;
}

AVSampleFormat AudioDecoder::getSampleFormat() const {
    return AV_SAMPLE_FMT_S16;
}