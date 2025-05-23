#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <string>

class VideoDecoder {
public:
	VideoDecoder();
	~VideoDecoder();

	bool openFile(const std::string& filepath); // gets our file from path
	AVFrame* getRGBFrame(); // gives us a rgb-converted frame

	int getWidth() const;
	int getHeight() const;
	double getFrameDelay() const;

private:
	bool decodeNextFrame();
	void setupSwsContext();

	AVFormatContext* fmt_ctx = nullptr;
	AVCodecContext* codec_ctx = nullptr;
	SwsContext* sws_ctx = nullptr;

	AVPacket* packet = nullptr;
	AVFrame* yuv_frame = nullptr;
	AVFrame* rgb_frame = nullptr;

	uint8_t* rgb_buffer = nullptr;

	int video_stream_index = -1;
	double frame_delay = 0.0;
};