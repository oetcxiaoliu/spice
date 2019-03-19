#ifndef _H_DEVREDIR_CAM
#define _H_DEVREDIR_CAM



#include "linux_video.h"
#include "stdint.h"


//begin  wxr add 2014-2-10
#define _H264_ENCODE_CAM_DATA_

#ifdef _H264_ENCODE_CAM_DATA_

#include "x264_encode.h"

#endif
//end

#define		SENDBUFSIZE 1024
#define     BUFSIZE		1024*1024*20


enum 
{
	VideoDeviceADD	 = 2,
	VideoDeviceDelete,
	VideoDeviceRun,
	VideoDeviceStop,
	VideoDeviceFram,
	VideoDeviceGetFram,
	VideoDeviceGetData,
	VideoDeviceStat_OK,
	VideoDeviceStat_Fail,
	VideoDeviceStat_Test,
	VideoDeviceStat_Fish,
};

class RedClient;

class DelVideoMessage
{
	//friend 
public:
	DelVideoMessage();
	virtual ~DelVideoMessage();

	void VideoMessgDel(void* msg, void* data,void* parm);
	
	static DelVideoMessage* getInstance();
	static void*  GetVideoDataThread(void* parm);
	
	void  AddUsbVideodevices();
	void  RemoveUsbVideoDevice(int nPid, int nVid);
	void  SendMesgAgentAddDevice(int nPid, int nVid);
private:
	bool GetData();	
	int  SendVideoData(uint8_t *pVideoData,uint32_t VideoDataSize,void* pClient);
	
	//void send_msg_to_device(uint32_t msg_source_flag, uint32_t msg_type, uint8_t *data, uint32_t data_size);
private:
	CVideo*		m_pVideo;
	bool		m_bAddDevice;
	bool		m_bRunDevice;
	char*		m_pVideoBufer;
	bool		m_bData;
	bool		m_bDeletDevice;
    bool        m_bDeviceError;
	bool		m_bGetVideoData;
	bool		m_bThread;
	int			m_nVideoDataSize;
	int			m_nPid;
	int			m_nVid;
	RedClient*  m_pRedClient;
	static DelVideoMessage*		m_pDeVideoMessage;

//begin  wxr add 2014-2-10
#ifdef _H264_ENCODE_CAM_DATA_

	CH264Encode  H264Encode;	
	
	uint8_t*    m_H264Data;

	int SendVideoDataByH264(uint8_t *pVideoData,uint32_t VideoDataSize,void* pClient);
	
	
//	int writefile(uint8_t *pData,uint32_t lenth, bool bIsEncode = false);

#endif

};





















#endif
