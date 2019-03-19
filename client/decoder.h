/*
    adding h264 decoder video by wang yingying at 2013-02-25
*/

#ifndef _H_DECODER
#define _H_DECODER

#include "common.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};

class Decoder {
public:
    Decoder(uint32_t codec_type, int32_t width, int32_t height, int32_t stride,
                uint8_t *frame, bool back_compat);
    ~Decoder();
    bool decode_data(uint8_t *data, size_t length);

    AVCodecContext *_codec_context;
private:
    int32_t _width;
    int32_t _height;    
    int32_t _stride;
    uint8_t *_frame;

    uint8_t *_rgb_src[3];
    int _rgb_stride[3];
    uint32_t _codec_type;
    struct SwsContext *_sws;
};

#endif


