#pragma once

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VIDEOPROCESS_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VIDEOPROCESS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VIDEOPROCESS_EXPORTS
#define VIDEOPROCESS_API __declspec(dllexport)
#else
#define VIDEOPROCESS_API __declspec(dllimport)
#endif

#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfiltergraph.h>
}

namespace VideoProcess
{
	using std::string;

	//~ class VIDEOPROCESS_API Decoder {
	class Decoder {
	private:
		AVPacket _packet;
		AVFrame _frame;
		AVFormatContext *_format_ctx;
		AVCodecContext *_codec_ctx;
		AVStream *_video_stream;
	public:
		//~ Decoder(const string &filename);
		Decoder(const string filename);
		~Decoder();
		AVFrame * getNextFrame();
		AVRational getTimeBase();
		AVRational getAspectRatio();
		PixelFormat getPixelFormat();
		int getWidth();
		int getHeight();
		int64_t getDuration();
	};

	//~ class VIDEOPROCESS_API Scaler
	class Scaler
	{
	private:
		SwsContext *_img_convert_ctx;
		AVFrame _frame;
	public:
		Scaler(int src_width, int src_height, PixelFormat src_pix_fmt,
			int dst_width, int dst_height, PixelFormat dst_pix_fmt);
		~Scaler();
		AVFrame * scaleFrame(AVFrame *src_frame, int src_slice_y, int src_slice_height);
	};

	//~ class VIDEOPROCESS_API Filter
	class Filter
	{
	private:
		AVFilterGraph *_filter_graph;
		AVFilterContext *_buffer_filter_ctx;
		AVFilterContext *_sink_filter_ctx;
		AVFrame *(*get_next_frame)(void *opaque);
		AVFrame _frame;
		void *_opaque;
		AVRational _aspect_ratio;
	public:
		Filter(const std::string &filter_description, int width, int height,
			PixelFormat pix_fmt, AVRational time_base, AVRational aspect_ratio,
			AVFrame *(*next_frame_getter)(void *opaque), void *opaque);
		~Filter();
		AVFrame * getNextFilteredFrame();
	};

	//~ class VIDEOPROCESS_API Encoder
	class Encoder
	{
	private:
		static const char *FORMATNAME;
		static const char *CODECNAME;
		AVOutputFormat *_output_format;
		AVFormatContext *_format_ctx;
		AVCodecContext *_codec_ctx;
		AVStream *_stream;
		uint8_t *_output_buffer;
	public:
		Encoder(const string &filename, int width, int height, AVRational time_base);
		~Encoder();
		void appendFrame(AVFrame *frame);
	};
}
