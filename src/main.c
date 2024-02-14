#include <math.h>
#include <stdarg.h>

#include "imgbuffer.h"
#include "videodecoder.h"

static int _w = 0;
static int _h = 0;
static int _framecount = 0;
static int _fps = 0;

static int _s = 0;
static int _sizebytes = 0;

static int _i = 0;
static int _j = 0;

static IMGBUFFER _backbuffer;

static BOOL _first;
static VIDEODECODER _viddec;


void draw(char *text)
{	
	AVFrame *frame;

	if(_first)
	{
		if(videodecoder_create(&_viddec, text) == CFAILED)
		{
			return;
		}
		_first = FALSE;
	}
	
	frame = videodecoder_getframe(&_viddec);
	if(frame)
	{
		imgbuffer_convertfromyuv420(&_backbuffer, frame);
		// av_free(frame); !! not needed
	}
}

int filtercreate(int fps)
{
	_fps = fps;

	_first = TRUE;
	
	return 1;
}

void filterdestroy()
{
	if(!_first)
	{
		videodecoder_destroy(&_viddec);
	}
}


int filtervideo(unsigned char *buffer, int w, int h, unsigned int color, char *text, int64_t framecount)
{
    _w = w;
    _h = h;
    _framecount = framecount;

    _s = _w * _h;
    _sizebytes = _s * 4;

    _backbuffer.data = buffer;
    _backbuffer.w = _w;
    _backbuffer.h = _h;
    _backbuffer.s = _sizebytes;

    srand(_framecount);

    draw(text);

    return 1;
}
