/*
    adding h264 decoder video by wang yingying at 2013-02-25
*/

#include "common.h"
#include "debug.h"
#include "utils.h"
#include "decoder.h"
typedef struct _DecType{
     AVCodecID type;
     bool top_down;
}DECTYPE;

DECTYPE DecType[SPICE_VIDEO_CODEC_TYPE_ENUM_END] = 
     {{AV_CODEC_ID_NONE,false}, 
     {AV_CODEC_ID_MJPEG,false},
     {AV_CODEC_ID_H264, true}};

Decoder::Decoder(uint32_t codec_type, int32_t width, int32_t height, int32_t stride, uint8_t *frame,
                bool back_compat) :
    _codec_context(NULL)
    , _width(width)
    , _height(height)
    , _stride(stride)
    , _frame(frame)
    , _codec_type(codec_type)
{

    if (_codec_type == 0 || _codec_type >= SPICE_VIDEO_CODEC_TYPE_ENUM_END) {
   
        THROW("invalid video codec type %u", codec_type);
    }

    AVCodec *codec = NULL;

    codec = avcodec_find_decoder(DecType[_codec_type].type);
    if (!codec) {
        THROW("decode type %u not found", _codec_type);
        return;
    }

    _codec_context = avcodec_alloc_context3(codec); //函数用于分配一个AVCodecContext并设置默认值，如果失败返回NULL，并可用av_free()进行释放。    

    if (codec->capabilities & CODEC_CAP_TRUNCATED) {   
      
        _codec_context->flags|= CODEC_FLAG_TRUNCATED;  
    }  
    /* open it */
    if (avcodec_open2(_codec_context, codec, NULL) < 0) {  //函数用给定的AVCodec来初始化AVCodecContext
    
        THROW("could not open codec");
    }

    _rgb_src[0] = _frame;
    _rgb_src[1] = NULL;
    _rgb_src[2] = NULL;
    _rgb_stride[0] = 4*_width;
    _rgb_stride[1] = 0;
    _rgb_stride[2] = 0;
    _sws = sws_getContext(_width, _height, AV_PIX_FMT_YUV420P, _width, 
                          _height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

}

Decoder::~Decoder()
{
    sws_freeContext(_sws);
    avcodec_close(_codec_context);
    av_free(_codec_context);
}

bool Decoder::decode_data(uint8_t *data, size_t length)
{
    int len, got_picture;
    AVFrame *picture;
    bool ret = false;
    AVPacket avpkt;

    picture = avcodec_alloc_frame(); //函数用于分配一个AVFrame并设置默认值，如果失败返回NULL，并可用av_free()进行释放。
    if (!picture) {
        THROW("could not alloc frame");
        return ret;
    }
    av_init_packet(&avpkt);
    avpkt.data = data;
    avpkt.size = length;
    /* 解码 */
    len = avcodec_decode_video2(_codec_context, picture, &got_picture, &avpkt);
    if (len < 0) {
        LOG_WARN("Error while decoding frame");
        return ret;
    }
    if (got_picture) {
        if (DecType[_codec_type].top_down) {
            picture->data[0] += picture->linesize[0] * (_codec_context->height - 1);
            picture->linesize[0] *= -1;                      
            picture->data[1] += picture->linesize[1] * (_codec_context->height / 2 - 1);
            picture->linesize[1] *= -1;
            picture->data[2] += picture->linesize[2] * (_codec_context->height / 2 - 1);
            picture->linesize[2] *= -1;
        }
        sws_scale(_sws, picture->data, picture->linesize, 0, _height, _rgb_src, _rgb_stride);
        ret = true;
    }

    av_free(picture);
    av_free_packet(&avpkt);

    return ret;
}
