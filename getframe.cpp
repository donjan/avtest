#include <cstdlib>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
}

std::string filename = "bluescreen.avi";
AVFormatContext * av_input;
AVStream * vstream;  // video stream
AVStream * astream;  // audio stream
AVFrame * rgbframe;  // AVFrame structure (video frame)

void die(std::string str) { std::cerr << str << std::endl; std::abort(); }
void init_stuff();
void cleanup_stuff();

int main() {

    init_stuff();


    cleanup_stuff();

    return EXIT_SUCCESS;
}


void init_stuff() {

    av_register_all();

    // open containers:

    if(avformat_open_input(&av_input, filename.c_str(), NULL, NULL) != 0) die("open input error");
    if(avformat_find_stream_info(av_input, NULL) < 0) die("stream info error");

    for(int i=0; i < av_input->nb_streams; i++) {
        if(av_input->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            vstream = av_input->streams[i];
            break;
        }
    }
    if(!vstream) die("no video stream!");

    for(int i=0; i < av_input->nb_streams; i++) {
        if(av_input->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            astream = av_input->streams[i];
            break;
        }
    } // don't check if present because it might be audioless


    // load codecs:

    AVCodec *codec;
    codec = avcodec_find_decoder(vstream->codec->codec_id);
    if(!codec) die("no known codec!");
    else std::cout << "codec is: " << codec->name << std::endl;

    if(codec->capabilities & CODEC_CAP_TRUNCATED)
        vstream->codec->flags |= CODEC_FLAG_TRUNCATED;
    if(avcodec_open2(vstream->codec, codec, NULL) < 0) die("cannot open codec!");


    double framerate = (double)vstream->r_frame_rate.num / (double)vstream->r_frame_rate.den;
    std::cout << "framerate (fps): " << framerate << std::endl;


    // prepare frame:

    rgbframe = avcodec_alloc_frame();
    if(!rgbframe) die("unable to allocate rgbframe");

    int size = avpicture_get_size(PIX_FMT_RGB24, vstream->codec->width, vstream->codec->height);
    uint8_t * frame_buf = new uint8_t[size];
    // TODO: use something "smart"-er than array pointer for buffer -> free at cleanup
    if(!frame_buf) { av_free(rgbframe); die("error allocating frame_buf"); }

    avpicture_fill((AVPicture *)rgbframe, frame_buf, PIX_FMT_RGB24, vstream->codec->width, vstream->codec->height);

}

void cleanup_stuff() {

    if(rgbframe) {
        av_free(rgbframe->data[0]);
        av_free(rgbframe);
        rgbframe = NULL;
    }

    if(vstream) if(vstream->codec->codec) avcodec_close(vstream->codec);

    // TODO: close/free output and video out buffer
    // TODO: close output files manually (no avformat_close_output)

    if(av_input) avformat_close_input(&av_input);
    else if(vstream) av_freep(&vstream);   

}
