//~ #include "stdafx.h"

#include "VideoProcess.h"
#include "DebugPrinter.hpp"

using namespace VideoProcess;

static void saveFrame(AVFrame *frame, int width, int height, int it);
static AVFrame * get_next_frame(void *opaque);

//~ #define TESTVIDEO "bluescreen.avi"
std::string TESTVIDEO = "bluescreen.avi";

int main(int argc, char* argv[])
{

	Decoder decoder = Decoder(TESTVIDEO);

	int width = decoder.getWidth();
	int height = decoder.getHeight();
	PixelFormat pix_fmt = decoder.getPixelFormat();
	AVRational time_base = decoder.getTimeBase();
	AVRational aspect_ratio = decoder.getAspectRatio();

dcout(_HERE_);
	Filter filter = Filter("yadif=1:-1", width, height, pix_fmt,
		time_base, aspect_ratio, &get_next_frame, &decoder);
dcout(_HERE_);

	Scaler scaler = Scaler(width, height, pix_fmt, width, height, PIX_FMT_RGB24);

	//Encoder encoder = Encoder("out.avi", width, height, time_base);

	for (int it = 0; it < 5; it++)
	{
		AVFrame *read_frame = filter.getNextFilteredFrame();
		if (read_frame == NULL)
			break;

		AVFrame *rgb_frame = scaler.scaleFrame(read_frame, 0, height);

		saveFrame(rgb_frame, width, height, it);

		/* Encoder test

		AVFrame *read_frame = decoder.getNextFrame();
		if (read_frame == NULL)
			break;

		encoder.appendFrame(read_frame);
		*/
	}
		
	return 0;
}

void saveFrame(AVFrame *frame, int width, int height, int it)
{
    FILE *file;
    char filename[32];
    int  y;

    // Open file
    sprintf(filename, "frame%d.ppm", it);
    file=fopen(filename, "wb");
    if(file==NULL)
        return;

    // Write header
    fprintf(file, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(frame->data[0]+y * frame->linesize[0], 1, width*3, file);

    // Close file
    fclose(file);
}

AVFrame * get_next_frame(void *opaque)
{
	return ((Decoder *)opaque)->getNextFrame();
}
