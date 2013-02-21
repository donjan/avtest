//~ #include "stdafx.h"

#include <exception>
#include "DebugPrinter.hpp"

extern "C" {
//~ #include <libavformat/avformat.h>
//~ #include <libavcodec/avcodec.h>
//~ #include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
//~ #include <libavfilter/avfiltergraph.h>
#include <libavfilter/vsrc_buffer.h>
#include <libavutil/mathematics.h>
}

#include "VideoProcess.h"

#define STREAM_PIX_FMT PIX_FMT_YUV420P
#define BITRATE 10000000
#define GOPSIZE 12
#define CODECID CODEC_ID_MPEG2VIDEO
#define VIDEOSTREAMID 0
#define OUTPUTBUFFERSIZE 200000

namespace VideoProcess
{
	class LibavInit
	{
	public:
		LibavInit()
		{
			// Register all formats and codecs
			av_register_all();

			// Register all filters
			avfilter_register_all();
		}
	};

	static LibavInit libavInit;

	//~ Decoder::Decoder(const string &filename)
	Decoder::Decoder(const string filename)
	{


		_format_ctx = NULL;	// kinda important... TODO: add all NULL inits.
		//~ if (av_open_input_file(&_format_ctx, filename.c_str(), NULL, 0, NULL) != 0)
		if(avformat_open_input(&_format_ctx, filename.c_str(), NULL, NULL) != 0)
			throw std::exception();

		// Retrieve stream information
		//~ if (av_find_stream_info(_format_ctx) < 0)
		if (avformat_find_stream_info(_format_ctx, NULL) < 0)
			throw std::exception();

		av_dump_format(_format_ctx, 0, filename.c_str(), false);

		// Find the first video stream
		_video_stream = NULL;
		for(int it = 0; it < (int)_format_ctx->nb_streams; it++) {
			_video_stream = _format_ctx->streams[it];
			//~ if (_video_stream->codec->codec_type == CODEC_TYPE_VIDEO) {
			if (_video_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {

				// Get a pointer to the codec context for the video stream
				_codec_ctx = _video_stream->codec;
				break;
			}
		}

		if(_video_stream == NULL)
			throw std::exception();

		// Find the decoder for the video stream
		AVCodec *codec = avcodec_find_decoder(_codec_ctx->codec_id);
		if (codec == NULL)
			throw std::exception();

		int rv;

		//~ rv = avcodec_open(_codec_ctx, codec);
		rv = avcodec_open2(_codec_ctx, codec, NULL);
		if (rv != 0)
			throw std::exception();

		// Init the packet that will be used during decoding
		av_init_packet(&_packet);

		// Init the frame that will be returned when asked for
		avcodec_get_frame_defaults(&_frame);
	}

	Decoder::~Decoder()
	{
		// Close the codec
		avcodec_close(_codec_ctx);

		// Close the video file
		//~ av_close_input_file(_format_ctx);
		avformat_close_input(&_format_ctx);
	}

	AVFrame * Decoder::getNextFrame()
	{
		while (true)
		{
			int rv = av_read_frame(_format_ctx, &_packet);

			switch (rv)
			{
			case AVERROR_EOF:
				return NULL;
			case 0:
				break;
			default:
				throw std::exception();
			}
			
			if (_packet.stream_index == _video_stream->index)
				break;
		}

		int got_frame;
		int rv =  avcodec_decode_video2(_codec_ctx, &_frame, &got_frame, &_packet);

		if (rv < 0  && got_frame == 0)
			throw std::exception();

		// Needed. The pts is not set automatically to the packet
		//~ _frame.pts = _codec_ctx->pts_correction_last_pts;

		return &_frame;
	}

	int Decoder::getWidth()
	{
		return _codec_ctx->width;
	}

	int Decoder::getHeight()
	{
		return _codec_ctx->height;
	}

	PixelFormat Decoder::getPixelFormat()
	{
		return _codec_ctx->pix_fmt;
	}

	AVRational Decoder::getTimeBase()
	{
		return _codec_ctx->time_base;
	}

	AVRational Decoder::getAspectRatio()
	{
		if (_video_stream->sample_aspect_ratio.num)
			return _video_stream->sample_aspect_ratio;
		else
			return _codec_ctx->sample_aspect_ratio;
	}

	int64_t Decoder::getDuration()
	{
		//~ return _format_ctx->timestamp;
		return _format_ctx->duration;
	}

	Scaler::Scaler(int src_width, int src_height, PixelFormat src_pix_fmt,
		int dst_width, int dst_height, PixelFormat dst_pix_fmt)
	{
		_img_convert_ctx = sws_getContext(src_width, src_height, src_pix_fmt,
			dst_width, dst_height, dst_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
		if (_img_convert_ctx == NULL)
			throw std::exception();

		// Init the frame that will be returned when asked for
		avcodec_get_frame_defaults(&_frame);

		int rv;

		rv = avpicture_alloc((AVPicture *)&_frame, dst_pix_fmt, dst_width, dst_height);
		if (rv != 0)
			throw std::exception();
	}

	Scaler::~Scaler()
	{
		avpicture_free((AVPicture *)&_frame);
	}

	AVFrame * Scaler::scaleFrame(AVFrame *src_frame, int src_slice_y,
		int src_slice_height)
	{
        sws_scale(_img_convert_ctx, src_frame->data, src_frame->linesize,
			src_slice_y, src_slice_height, _frame.data, _frame.linesize);

		return &_frame;
	}

	Filter::Filter(const std::string &filter_description, int width, int height,
		PixelFormat pix_fmt, AVRational time_base, AVRational aspect_ratio,
		AVFrame *(*next_frame_getter)(void *opaque), void *opaque)
	{
		get_next_frame = next_frame_getter;
		_opaque = opaque;
		_aspect_ratio = aspect_ratio;

		// Init the frame that will be returned when asked for
		avcodec_get_frame_defaults(&_frame);

		_filter_graph = avfilter_graph_alloc();
		if (_filter_graph == NULL)
			throw std::exception();

		AVFilter *buffer_filter = avfilter_get_by_name("buffer");
		if (buffer_filter == NULL)
			throw std::exception();

		char args[256];
		//~ sprintf_s(args, sizeof(args), "%d:%d:%d:%d:%d:%d:%d", width, height, pix_fmt,
dcout(_HERE_);
		sprintf(args, (const char*)sizeof(args), "%d:%d:%d:%d:%d:%d:%d", width, height, pix_fmt,
			time_base.num, time_base.den, aspect_ratio.num, aspect_ratio.den);
dcout(_HERE_);

		int rv;

		// Buffer video source: the decoded frames from the codec will be inserted here
		rv = avfilter_graph_create_filter(&_buffer_filter_ctx, buffer_filter, "src", args,
			NULL, _filter_graph);
		if (rv != 0)
			throw std::exception();

		AVFilter *sink_filter = avfilter_get_by_name("nullsink");
		if (sink_filter == NULL)
			throw std::exception();

		// Null video sink: to terminate the filter chain
		rv = avfilter_graph_create_filter(&_sink_filter_ctx, sink_filter, "out",
			NULL, NULL, _filter_graph);
		if (rv != 0)
			throw std::exception();

		// Endpoints for the filter graph

		AVFilterInOut *buffer_filter_outputs =
			(AVFilterInOut *)av_malloc(sizeof(AVFilterInOut));
		if (buffer_filter_outputs == NULL)
			throw std::exception();

		buffer_filter_outputs->name = av_strdup("in");
		buffer_filter_outputs->filter_ctx = _buffer_filter_ctx;
		buffer_filter_outputs->pad_idx = 0;
		buffer_filter_outputs->next = NULL;

		AVFilterInOut *sink_filter_inputs =
			(AVFilterInOut *)av_malloc(sizeof(AVFilterInOut));
		if (sink_filter_inputs == NULL)
			throw std::exception();

		sink_filter_inputs->name = av_strdup("out");
		sink_filter_inputs->filter_ctx = _sink_filter_ctx;
		sink_filter_inputs->pad_idx = 0;
		sink_filter_inputs->next = NULL;
	
		rv = avfilter_graph_parse(_filter_graph, filter_description.c_str(),
			sink_filter_inputs, buffer_filter_outputs, NULL);
		if (rv != 0)
			throw std::exception();

		rv = avfilter_graph_config(_filter_graph, NULL);
		if (rv != 0)
			throw std::exception();

		AVFrame *first_frame = get_next_frame(opaque);
		av_vsrc_buffer_add_frame(_buffer_filter_ctx, first_frame, first_frame->pts,
			aspect_ratio);
	}

	Filter::~Filter()
	{
		avfilter_graph_free(&_filter_graph);
	}

	AVFrame * Filter::getNextFilteredFrame()
	{
		AVFilterLink *output_link = _sink_filter_ctx->inputs[0];

		int rv = avfilter_poll_frame(output_link);

		if (rv < 0)
			throw std::exception();

		while (rv == 0)
		{
			AVFrame *new_src_frame = get_next_frame(_opaque);
			if (new_src_frame == NULL)
				return NULL;

			av_vsrc_buffer_add_frame(_buffer_filter_ctx, new_src_frame,
				new_src_frame->pts, _aspect_ratio);

			rv = avfilter_poll_frame(output_link);
			if (rv < 0)
				throw std::exception();
		}

		rv = avfilter_request_frame(output_link);

		AVFilterBufferRef *filter_buffer = output_link->cur_buf;
		if (filter_buffer == NULL)
			throw std::exception();

		memcpy(_frame.data, filter_buffer->data, sizeof(_frame.data));
		memcpy(_frame.linesize, filter_buffer->linesize, sizeof(_frame.linesize));

		return &_frame;
	}

	Encoder::Encoder(const string &filename, int width, int height,
		AVRational time_base)
	{
		int rv;

		_output_format = av_guess_format(NULL, filename.c_str(), NULL);
		if (_output_format == NULL)
			throw std::exception();

		_format_ctx = avformat_alloc_context();
		if (_format_ctx == NULL)
			throw std::exception();

		_format_ctx->oformat = _output_format;
		//~ sprintf_s(_format_ctx->filename, sizeof(_format_ctx->filename), "%s", filename);
		// TODO: make sense of this
		//~ sprintf(_format_ctx->filename, (const char*)sizeof(_format_ctx->filename), "%s", filename);
		char * filename2;
		sprintf(_format_ctx->filename, (const char*)sizeof(_format_ctx->filename), "%s", filename2);
		//~ filename = _format_ctx->filename;

		//~ _stream = av_new_stream(_format_ctx, VIDEOSTREAMID);
		_stream = avformat_new_stream(_format_ctx, VIDEOSTREAMID);
		if (_stream == NULL)
			throw std::exception();

		_codec_ctx = _stream->codec;
		_codec_ctx->codec_id = CODECID;
		_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

		// put sample parameters
		_codec_ctx->bit_rate = BITRATE;
		// resolution must be a multiple of two
		_codec_ctx->width = width;
		_codec_ctx->height = height;
		// time base: this is the fundamental unit of time (in seconds) in terms
		// of which frame timestamps are represented. for fixed-fps content,
		// timebase should be 1/framerate and timestamp increments should be
		// identically 1.
		_codec_ctx->time_base = time_base;
		_codec_ctx->gop_size = GOPSIZE; // emit one intra frame every GOPSIZE frames at most
		_codec_ctx->pix_fmt = STREAM_PIX_FMT;

		// some formats want stream headers to be separate
		if(_output_format->flags & AVFMT_GLOBALHEADER)
			_codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;

		rv = av_set_parameters(_format_ctx, NULL);
		if (rv != 0)
			throw std::exception();

		// find the video encoder
		AVCodec *codec = avcodec_find_encoder(_codec_ctx->codec_id);
		if (codec == NULL)
			throw std::exception();

		// open the codec
		//~ rv = avcodec_open(_codec_ctx, codec);
		rv = avcodec_open2(_codec_ctx, codec, NULL);
		if (rv != 0)
			throw std::exception();

		_output_buffer = NULL;
		if (!(_format_ctx->oformat->flags & AVFMT_RAWPICTURE)) {
			// Allocate output buffer
			// WARNING: API change will be done
			// buffers passed into lav* can be allocated any way you prefer,
			// as long as they're aligned enough for the architecture, and
			// they're freed appropriately (such as using av_free for buffers
			// allocated with av_malloc)
			_output_buffer = (uint8_t *)av_malloc(OUTPUTBUFFERSIZE);
			if (_output_buffer == NULL)
				throw std::exception();
		}

		av_dump_format(_format_ctx, 0, filename.c_str(), 1);

		// open the output file, if needed
		if (!(_output_format->flags & AVFMT_NOFILE)) {
			rv = avio_open(&_format_ctx->pb, filename.c_str(), URL_WRONLY);
			if (rv != 0)
				throw std::exception();
        }

		//~ av_write_header(_format_ctx);
		avformat_write_header(_format_ctx, NULL);
	}

	Encoder::~Encoder()
	{
		int rv;

		rv = av_write_trailer(_format_ctx);

		if (rv != 0)
			throw std::exception();

		rv = avcodec_close(_codec_ctx);
		if (rv != 0)
			throw std::exception();

		if (!(_output_format->flags & AVFMT_NOFILE)) {
			// close the output file
			rv = avio_close(_format_ctx->pb);
		}

		av_free(_output_buffer);

		avformat_free_context(_format_ctx);
	}

	void Encoder::appendFrame(AVFrame *frame)
	{
		int rv = 0;

		int out_size = 0;
		if (_output_format->flags & AVFMT_RAWPICTURE) {
			// Raw video case. The API will change slightly in the near future for that
			AVPacket packet;
			av_init_packet(&packet);

			packet.flags |= AV_PKT_FLAG_KEY;
			packet.stream_index= VIDEOSTREAMID;
			packet.data= (uint8_t *)frame;
			packet.size= sizeof(AVPicture);

			rv = av_interleaved_write_frame(_format_ctx, &packet);
		} else {
			// encode the image
			out_size = avcodec_encode_video(_codec_ctx, _output_buffer,
				OUTPUTBUFFERSIZE, frame);
			// if zero size, it means the image was buffered
			if (out_size > 0) {
				AVPacket packet;
				av_init_packet(&packet);

				if (_codec_ctx->coded_frame->pts != AV_NOPTS_VALUE)
					packet.pts= av_rescale_q(_codec_ctx->coded_frame->pts,
					_codec_ctx->time_base, _stream->time_base);
				if(_codec_ctx->coded_frame->key_frame)
					packet.flags |= AV_PKT_FLAG_KEY;
				packet.stream_index= VIDEOSTREAMID;
				packet.data= _output_buffer;
				packet.size= out_size;

				// write the compressed frame in the media file
				rv = av_interleaved_write_frame(_format_ctx, &packet);
			}
		}

		if (rv != 0 || out_size < 0)
			throw std::exception();
	}
}
