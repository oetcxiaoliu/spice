

#include <malloc.h>
#include "stdio.h"
#include <string.h>
#include "x264_encode.h"


CH264Encode::CH264Encode()
{
	pX264Handle   = NULL; 
	pPicIn        = NULL;
	pPicOut       = NULL;
	pX264Param    = NULL;

	
}
// 初始化x264的相关参数

bool CH264Encode::x264_Init(int width,int height)
{

	pPicIn = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	pPicOut = (x264_picture_t*)malloc(sizeof(x264_picture_t));  

	pX264Param =(x264_param_t*)malloc(sizeof(x264_param_t));
	if(pPicIn==NULL||pPicOut==NULL||pX264Param==NULL)
	{
		return false;
	}
	
	//* 配置参数  
     //* 使用默认参数，在这里因为我的是实时网络传输，所以我使用了zerolatency的选项，
	//使用这个选项之后就不会有delayed_frames，如果你使用的不是这样的话，还需要在编码完成之后得到缓存的编码帧  
     if(x264_param_default_preset(pX264Param, "veryfast", "zerolatency")!=0)
	 {
		 return false;  
	 }
     //* cpuFlags  
     pX264Param->i_threads  = X264_SYNC_LOOKAHEAD_AUTO;//* 取空缓冲区继续使用不死锁的保证.  
     //* 视频选项  
     pX264Param->i_width   = width; //* 要编码的图像宽度.  
     pX264Param->i_height  = height; //* 要编码的图像高度  
     pX264Param->i_frame_total = 0; //* 编码总帧数.不知道用0. （根据摄像头数据填充最大帧数，我们这里选择20帧，以20帧为一个包） 
     pX264Param->i_keyint_max = 1;  
	 pX264Param->i_keyint_min = 1;
     //* 流参数  
     pX264Param->i_bframe  = 5;  
     pX264Param->b_open_gop  = 0;  
     pX264Param->i_bframe_pyramid = 0;  
     pX264Param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;  
     //* Log参数，不需要打印编码信息时直接注释掉就行  
     //pX264Param->i_log_level  = X264_LOG_DEBUG;  
     //* 速率控制参数  
     pX264Param->rc.i_bitrate = 1024 * 25;//* 码率(比特率,单位Kbps)  
     //* muxing parameters  
     pX264Param->i_fps_den  = 1; //* 帧率分母  
     pX264Param->i_fps_num  = 25;//* 帧率分子  
     pX264Param->i_timebase_den = pX264Param->i_fps_num;  
     pX264Param->i_timebase_num = pX264Param->i_fps_den;  
     //* 设置Profile.使用Baseline profile  
     if(x264_param_apply_profile(pX264Param, x264_profile_names[0])!=0)
	 {
		 return false;
	 }
	   //* 打开编码器句柄,通过x264_encoder_parameters得到设置给X264  
     //* 的参数.通过x264_encoder_reconfig更新X264的参数  
     pX264Handle = x264_encoder_open(pX264Param);
	 if(pX264Handle==NULL)
	 {
		 return false;
	 }
	 //图像区域初始化以及分配空间
	 x264_picture_init(pPicOut);  
	 if(x264_picture_alloc(pPicIn, X264_CSP_I420, pX264Param->i_width, pX264Param->i_height)!=0)
	 {
		 printf("alloc failure!\n");
		 return false;
	 } 
	 pPicIn->img.i_csp = X264_CSP_I420;  
	 pPicIn->img.i_plane = 3;

	 return true;
}

void CH264Encode::RGB2YUV(unsigned char *RGB, unsigned char *YUV[], unsigned int WIDTH, unsigned int HEIGHT)
{
	//变量声明
	unsigned int i,x,y,j;
	unsigned char *Y = NULL;
	unsigned char *U = NULL;
	unsigned char *V = NULL;

	Y = YUV[0];
	U = YUV[2];
	V = YUV[1];

	for(y=0; y < HEIGHT; y++)
	{
		for(x=0; x < WIDTH; x++)
		{
			j = y*WIDTH + x;
			i = j*3;
			Y[j] = (unsigned char)(DY(RGB[i], RGB[i+1], RGB[i+2]));
			if(x%2 == 1 && y%2 == 1)
			{
				j = (WIDTH>>1) * (y>>1) + (x>>1);
				//上面i仍有效
				U[j] = (unsigned char)
					((DU(RGB[i], RGB[i+1], RGB[i+2]) + 
					DU(RGB[i-3], RGB[i-2], RGB[i-1]) +
					DU(RGB[i-WIDTH*3], RGB[i+1-WIDTH*3], RGB[i+2-WIDTH*3]) +
					DU(RGB[i-3-WIDTH*3], RGB[i-2-WIDTH*3], RGB[i-1-WIDTH*3]))/4);
				V[j] = (unsigned char)
					((DV(RGB[i], RGB[i+1], RGB[i+2]) + 
					DV(RGB[i-3], RGB[i-2], RGB[i-1]) +
					DV(RGB[i-WIDTH*3], RGB[i+1-WIDTH*3], RGB[i+2-WIDTH*3]) +
					DV(RGB[i-3-WIDTH*3], RGB[i-2-WIDTH*3], RGB[i-1-WIDTH*3]))/4);
			}
		}
	}
}

//
//YV12 ： 亮度（行×列） ＋ V（行×列/4) + U（行×列/4）

//I420 ： 亮度（行×列） + U（行×列/4) + V（行×列/4）

//YUY2(YUYV): YUYV YUYV YUYV
#if 0
void CH264Encode::YUY2toI420(int inWidth, int inHeight, uint8_t *pSrc, uint8_t *pDest)
{
	int i, j;

	uint8_t *u = pDest + (inWidth * inHeight);
	uint8_t *v = u + (inWidth * inHeight) / 4;

	for (i = 0; i < inHeight/2; i++)
	{

		uint8_t *src_l1 = pSrc + inWidth*2*2*i;
		uint8_t *src_l2 = src_l1 + inWidth*2;
		uint8_t *y_l1 = pDest + inWidth*2*i;
		uint8_t *y_l2 = y_l1 + inWidth;
		for (j = 0; j < inWidth/2; j++)
		{
			// two pels in one go
			*y_l1++ = src_l1[0];
			*u++ = src_l1[1];
			*y_l1++ = src_l1[2];
			*v++ = src_l1[3];
			*y_l2++ = src_l2[0];
			*y_l2++ = src_l2[2];
			src_l1 += 4;
			src_l2 += 4;
		}
	}
}
#endif

void CH264Encode::YUV422To420(uint8_t* pYUV, uint8_t *I420[], int lWidth, int lHeight)
{

	int i,j;
	uint8_t *pY = NULL;
	uint8_t *pU = NULL;
	uint8_t *pV = NULL;

	pY = I420[0];
	pU = I420[1];
	pV = I420[2];
	uint8_t* pYUVTemp = pYUV;
	uint8_t* pYUVTempNext = pYUV+lWidth*2;

	for(i=0; i<lHeight; i+=2)
	{
		for(j=0; j<lWidth; j+=2)
		{
			pY[j] = *pYUVTemp++;
			pY[j+lWidth] = *pYUVTempNext++;

			pU[j/2] =(*(pYUVTemp) + *(pYUVTempNext))/2;
			pYUVTemp++;
			pYUVTempNext++;

			pY[j+1] = *pYUVTemp++;
			pY[j+1+lWidth] = *pYUVTempNext++;

			pV[j/2] =(*(pYUVTemp) + *(pYUVTempNext))/2;
			pYUVTemp++;
			pYUVTempNext++;
		}
		pYUVTemp+=lWidth*2;
		pYUVTempNext+=lWidth*2;
		pY+=lWidth*2;
		pU+=lWidth/2;
		pV+=lWidth/2;
	}

	//return TRUE;
} 
//YV12 ： 亮度（行×列） ＋ V（行×列/4) + U（行×列/4）
//I420 ： 亮度（行×列） ＋ U（行×列/4) + V（行×列/4）
void CH264Encode::YUY2toI420(uint8_t *YUY2, uint8_t *I420[],int inWidth, int inHeight)
{
	int i,j;
	uint8_t *Y = NULL;
	uint8_t *U = NULL;
	uint8_t *V = NULL;

	Y = I420[0];
	V = I420[1];
	U = I420[2];

	for (i = 0; i < inHeight/2; i++)
	{

		uint8_t *src_l1 = YUY2 + inWidth*2*2*i;
		uint8_t *src_l2 = src_l1 + inWidth*2;
		uint8_t *y_l1 = Y + inWidth*2*i;
		uint8_t *y_l2 = y_l1 + inWidth;
		for (j = 0; j < inWidth/2; j++)
		{
			// two pels in one go
			*y_l1++ = src_l1[0];
			*U++ = src_l1[1];
			*y_l1++ = src_l1[2];
			*V++ = src_l1[3];
			*y_l2++ = src_l2[0];
			*y_l2++ = src_l2[2];
			src_l1 += 4;
			src_l2 += 4;
		}
	}
}

void YUY2toI422(uint8_t *YUY2, uint8_t *I422[],int inWidth, int inHeight)
{
	int i,j;
	uint8_t *Y = NULL;
	uint8_t *U = NULL;
	uint8_t *V = NULL;

	Y = I422[0];
	U = I422[1];
	V = I422[2];
	for (i = 0; i < inHeight/2; i++)
	{

		uint8_t *src_l1 = YUY2 + inWidth*2*2*i;
		uint8_t *src_l2 = src_l1 + inWidth*2;
		uint8_t *y_l1 = Y + inWidth*2*i;
		uint8_t *y_l2 = y_l1 + inWidth;
		for (j = 0; j < inWidth/2; j++)
		{
			// two pels in one go
			*y_l1++ = src_l1[0];
			*U++ = src_l1[1];
			*y_l1++ = src_l1[2];
			*V++ = src_l1[3];
			*y_l2++ = src_l2[0];
			*y_l2++ = src_l2[2];
			src_l1 += 4;
			src_l2 += 4;
		}
	}

}
//编码
int CH264Encode::EncodeData(uint8_t *YUY2,uint8_t * H264Data,int H264DataMaxLen)
{

	int iNal=0;
	x264_nal_t* pNals=NULL;
	int frame_size=0;

	if( pX264Handle == NULL )
	{
		return 0;
	}
	//printf("picin->width=%d ,picin->height=%d \n",pPicIn->param->i_width,pPicIn->param->i_height);
	YUV422To420(YUY2, pPicIn->img.plane,pX264Param->i_width,pX264Param->i_height);
	frame_size = x264_encoder_encode(pX264Handle,&pNals,&iNal,pPicIn,pPicOut);

	if(frame_size >0)  
	{  
		if(H264DataMaxLen<frame_size)
		{
			return 0;
		}
		else
		{
			memcpy(H264Data,pNals[0].p_payload,frame_size);
			return frame_size;
		}	
	}
	else
	{
		return 0;
	}
	
}

//参数重设
bool CH264Encode::x264_Param_Retset(ResetParams Param)
{
	x264_encoder_parameters(pX264Handle,pX264Param);
	if(pX264Param==NULL||pX264Handle==NULL)
	{
		return false;
	}
	pX264Param->i_width   = Param.width; //* 要编码的图像宽度.  
    pX264Param->i_height  = Param.height; //* 要编码的图像高度  
	if(x264_encoder_reconfig(pX264Handle, pX264Param)!=0)
	{
		return false;
	}
	//首先清除原来的图像区域
	 x264_picture_clean(pPicIn); 
	 //初始化图像区域
	 x264_picture_init(pPicOut);  //X264_CSP_I422  X264_CSP_I420
	 if(x264_picture_alloc(pPicIn, X264_CSP_I420, pX264Param->i_width, pX264Param->i_height)!=0)
	 {
		 printf("alloc failure!\n");
		 return false;
	 } 
	 pPicIn->img.i_csp = X264_CSP_I420;  
	 pPicIn->img.i_plane = 3;
	 return true;
}

 //x264 数据清除
CH264Encode::~CH264Encode()
{
	//* 清除图像区域  
	if(pPicIn!=NULL)
	{
		x264_picture_clean(pPicIn); 
		free(pPicIn);  
		pPicIn = NULL;
	}
     //* 关闭编码器句柄  
	 if(pX264Handle!=NULL)
	 {
		x264_encoder_close(pX264Handle);  
		pX264Handle = NULL;  
	 }
    /* if(pPicIn!=NULL)
	 {
		 free(pPicIn);  
		 pPicIn = NULL; 
	 }*/
     if(pPicOut!=NULL)
	 {
		 free(pPicOut);  
		 pPicOut = NULL; 
	 }
	 if(pX264Param!=NULL)
	 {
		 free(pX264Param);  
		 pX264Param = NULL;  
	 }
}
void CH264Encode::x264_Clear()
{
	
	//* 清除图像区域  
	if(pPicIn!=NULL)
	{
		x264_picture_clean(pPicIn); 
		free(pPicIn);  
		pPicIn = NULL;
	}
      
     //* 关闭编码器句柄  
	 if(pX264Handle!=NULL)
	 {
		x264_encoder_close(pX264Handle);  
		pX264Handle = NULL;  
	 }
   /*  if(pPicIn!=NULL)
	 {
		 free(pPicIn);  
		 pPicIn = NULL; 
	 }*/
     if(pPicOut!=NULL)
	 {
		 free(pPicOut);  
		 pPicOut = NULL; 
	 }
	 if(pX264Param!=NULL)
	 {
		 free(pX264Param);  
		 pX264Param = NULL;  
	 }
}
void CH264Encode::Write_H264_File(uint8_t * H264Data,int H264DataLen,int width,int height)
{
	char filename[60]={0};
	FILE* H264File;
	sprintf(filename,"%d_%d.h264",width,height);
	if((H264File = fopen(filename, "ab+"))==NULL)
	{
		return ;
	}
	fwrite(H264Data, H264DataLen, 1, H264File); 
	fclose(H264File);
}

int CH264Encode::Read_YUY2_File(uint8_t * YUY2Data,int YUY2DataMaxLen,int width,int height)
{
	//char filename[60]={0};
	FILE* YUY2File;
	//sprintf(filename,"%d_%d.YUY2",width,height);
	if((YUY2File = fopen("YUY2", "rb"))==NULL)
	{
		return 0;
	}
	while(!feof(YUY2File))
	{
		if(YUY2DataMaxLen< width *height*2)
		{
			fclose(YUY2File);
			return 0;
		}
		fread(YUY2Data,width *height*2,1,YUY2File);
	}
	fclose(YUY2File);
	return width *height*2;
}
