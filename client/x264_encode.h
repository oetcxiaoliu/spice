
//      x264 ENCODE
 // make by yangyijing 



#ifndef X264_ENCODE_H
#define X264_ENCODE_H


#include "stdint.h"
extern "C"
{
#include "x264.h"
#include "x264_config.h"
};

#define STREAM_BUFFER_SIZE (5*1024*1024)
#define H264DATA_BUFFER_SIZE (10*1024*1024)

//转换矩阵
#define MY(a,b,c) (( a*  0.2989  + b*  0.5866  + c*  0.1145))
#define MU(a,b,c) (( a*(-0.1688) + b*(-0.3312) + c*  0.5000 + 128))
#define MV(a,b,c) (( a* 0.5000 + b*(-0.4184) + c*(-0.0816) + 128))
//大小判断
#define DY(a,b,c) (MY(a,b,c) > 255 ? 255 : (MY(a,b,c) < 0 ? 0 : MY(a,b,c)))
#define DU(a,b,c) (MU(a,b,c) > 255 ? 255 : (MU(a,b,c) < 0 ? 0 : MU(a,b,c)))
#define DV(a,b,c) (MV(a,b,c) > 255 ? 255 : (MV(a,b,c) < 0 ? 0 : MV(a,b,c)))
typedef struct PackData
{
	int          Num;
	int		Len[20];
	//uint8_t* Data;

}PACKDATA;
typedef struct ResetParams{
	int width;
	int height;
}ResetParams;
class CH264Encode
{
public:
	CH264Encode();
	~CH264Encode();
	bool x264_Init(int width,int height);
	//int GetYUY2Data(uint8_t *YUY2);
	void YUV422To420(uint8_t* pYUV, uint8_t *I420[], int lWidth, int lHeight);
	void YUY2toI420(uint8_t *YUY2, uint8_t *I420[],int inWidth, int inHeight);
	void RGB2YUV(unsigned char *RGB, unsigned char *YUV[], unsigned int WIDTH, unsigned int HEIGHT);
	//编码数据
	int EncodeData(uint8_t *YUY2,uint8_t * H264Data,int H264DataMaxLen);

	void x264_Clear();
	bool x264_Param_Retset(ResetParams Param);
	void Write_H264_File(uint8_t * H264Data,int H264DataLen,int width,int height);
	int Read_YUY2_File(uint8_t * YUY2Data,int YUY2DataMaxLen,int width,int height);
private:
	x264_param_t* pX264Param;
	x264_picture_t* pPicIn;
	x264_picture_t* pPicOut;
	x264_t* pX264Handle;

};


#endif