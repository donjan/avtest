#include <cstdlib>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
}

std::string filename = "bluescreen.avi";
AVFormatContext * av_input;
AVStream * vstream;  // video stream
AVStream * astream;  // audio stream


int main() {

    av_register_all();

    if(avformat_open_input(&av_input, filename.c_str(), NULL, NULL) != 0)
        std::cout << "open input error";
    if(avformat_find_stream_info(av_input, NULL) < 0)
        std::cout << "stream info error";
    for (int i=0; i < av_input->nb_streams; i++) {
        if (av_input->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            vstream = av_input->streams[i];
            break;
        }
    }
    if(!vstream)
        std::cout << "no video stream!";
    for (int i=0; i < av_input->nb_streams; i++) {
        if (av_input->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            astream = av_input->streams[i];
            break;
        }
    } // don't check if present because it might be audioless


    AVCodec *codec;
    codec = avcodec_find_decoder(vstream->codec->codec_id);
    if(!codec)
        std::cout << "unusable codec!";
    else std::cout << "codec is: " << codec->name;



    return EXIT_SUCCESS;
}
