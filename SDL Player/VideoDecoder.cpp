#include "VideoDecoder.h"
#include <iostream>

VideoDecoder::VideoDecoder() {}

VideoDecoder::~VideoDecoder() {
	av_packet_free(&packet);
	av_frame_free(&yuv_frame);
	av_frame_free(&rgb_frame);
	avcodec_free_context(&codec_ctx);
	avformat_close_input(&fmt_ctx);
	sws_freeContext(sws_ctx);
	av_free(rgb_buffer);
}

bool VideoDecoder::openFile(const std::string& filepath) {
	if (avformat_open_input(&fmt_ctx, filepath.c_str(), nullptr, nullptr) != 0) {
		std::cerr << "Failed to open file: " << filepath << "\n";
		return false;
	}

	if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
		std::cerr << "Failed to find stream info\n";
		return false;
	}

	for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;

			// frame delay
			AVRational frame_rate = fmt_ctx->streams[video_stream_index]->avg_frame_rate;
			if (frame_rate.num != 0 && frame_rate.den != 0) {
				frame_delay = 1.0 / av_q2d(frame_rate);
			}
			else {
				std::cerr << "Warning: Unknown frame rate, defaulting to 30 FPS\n";
				frame_delay = 1.0 / 30.0;
			}

			break;
		}
	}

	if (video_stream_index == -1) {
		std::cerr << "No video stream found\n";
		return false;
	}

	AVCodecParameters* codecpar = fmt_ctx->streams[video_stream_index]->codecpar;
	const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
	if (!codec) {
		std::cerr << "Decoder not found \n";
		return false;
	}

	codec_ctx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codec_ctx, codecpar);
	if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
		std::cerr << "Could not open codec\n";
		return false;
	}

	packet = av_packet_alloc();
	yuv_frame = av_frame_alloc();
	rgb_frame = av_frame_alloc();

	setupSwsContext();

	return true;
}

void VideoDecoder::setupSwsContext() {
	if (sws_ctx) {
		sws_freeContext(sws_ctx);
		sws_ctx = nullptr;
	}
	if (rgb_buffer) {
		av_free(rgb_buffer);
		rgb_buffer = nullptr;
	}

	int width = codec_ctx->width;
	int height = codec_ctx->height;
	AVPixelFormat src_fmt = codec_ctx->pix_fmt;
	AVPixelFormat dst_fmt = AV_PIX_FMT_RGB24;

	sws_ctx = sws_getContext(
		width, height, src_fmt,
		width, height, dst_fmt,
		SWS_BILINEAR, nullptr, nullptr, nullptr
	);

	int num_bytes = av_image_get_buffer_size(dst_fmt, width, height, 1);
	rgb_buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
	av_image_fill_arrays(
		rgb_frame->data, rgb_frame->linesize,
		rgb_buffer, dst_fmt,
		width, height, 1
	);
}

bool VideoDecoder::decodeNextFrame() {
	// Loop until a decoded frame is ready or no more packets
	while (true) {
		// Try to receive a frame already buffered in the decoder
		int ret = avcodec_receive_frame(codec_ctx, yuv_frame);
		if (ret == 0) {
			// Successfully got a frame
			return true;
		}
		else if (ret == AVERROR(EAGAIN)) {
			// Need to send more packets to decoder

			// Read the next packet from the stream
			if (av_read_frame(fmt_ctx, packet) < 0) {
				// No more packets available (end of file)
				// Flush decoder by sending a null packet
				avcodec_send_packet(codec_ctx, nullptr);

				// Try to receive remaining frames from decoder buffer
				ret = avcodec_receive_frame(codec_ctx, yuv_frame);
				if (ret == 0) {
					return true;
				}
				else {
					// No more frames
					return false;
				}
			}

			// Is this packet from our video stream?
			if (packet->stream_index == video_stream_index) {
				// Send packet to decoder
				ret = avcodec_send_packet(codec_ctx, packet);
				av_packet_unref(packet); // Unref packet immediately after sending

				if (ret < 0) {
					std::cerr << "Error sending packet to decoder\n";
					return false;
				}
				// Loop will continue and try to receive frame again
			}
			else {
				// Not our video stream, just unref and continue
				av_packet_unref(packet);
			}
		}
		else if (ret == AVERROR_EOF) {
			// Decoder has been fully flushed, no more frames
			return false;
		}
		else {
			// Other error
			std::cerr << "Error during decoding: " << ret << "\n";
			return false;
		}
	}
}

AVFrame* VideoDecoder::getRGBFrame() {
	if (!decodeNextFrame()) {
		return nullptr;
	}

	// Convert YUV -> RGB
	sws_scale(
		sws_ctx,
		yuv_frame->data, yuv_frame->linesize,
		0, codec_ctx->height,
		rgb_frame->data, rgb_frame->linesize
	);

	return rgb_frame;
}

int VideoDecoder::getWidth() const {
	return codec_ctx ? codec_ctx->width : 0;
}

int VideoDecoder::getHeight() const {
	return codec_ctx ? codec_ctx->height : 0;
}

double VideoDecoder::getFrameDelay() const {
	return frame_delay;
}