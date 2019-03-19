#include <pthread.h>
#include "red_client.h"
#include "devredir_cam.h"
#include <libusb.h>

DelVideoMessage::DelVideoMessage()
{
    m_bAddDevice        = false;
    m_bRunDevice        = false;
    m_bDeletDevice        = false;
    m_bData                = false;
    m_bDeviceError        = false;
    m_bGetVideoData        = false;
    m_bThread            = false;
    m_pVideo            = NULL;
    m_pVideoBufer        = NULL;
    m_nVideoDataSize    = 0;
    m_nPid                = -1;
    m_nVid                = -1;
    m_pVideoBufer        = new char[BUFSIZE];
//begin  wxr add 2014-2-10
#ifdef _H264_ENCODE_CAM_DATA_
    m_H264Data = new uint8_t[H264DATA_BUFFER_SIZE];

#endif
//end    

    libusb_init(NULL);
}

DelVideoMessage::~DelVideoMessage()
{
    libusb_exit(NULL);
    if (m_pVideo)
    {
        delete m_pVideo;
    }

    if (m_pVideoBufer)
    {
        delete[] m_pVideoBufer;
    }

#ifdef _H264_ENCODE_CAM_DATA_

    if(m_H264Data)
    {
        H264Encode.x264_Clear();
        delete m_H264Data;
    }
#endif
}

DelVideoMessage* DelVideoMessage::m_pDeVideoMessage = NULL;
DelVideoMessage* DelVideoMessage::getInstance()
{
    if (m_pDeVideoMessage == NULL)
    {
        m_pDeVideoMessage = new DelVideoMessage;
    }
    return m_pDeVideoMessage;
}

void DelVideoMessage::VideoMessgDel(void* msg, void* data,void* parm)
{
    bool        bResult = false;
    RedClient*  pThis;
    PACKHEADER    *header;
    PACKHEADER    nheader;
    int            iCount = 0;
    int         bWidth;
    int            bHeight;
    FRAME       frame;
    
    frame.bWidth  = 640;
    frame.bHeight = 480;
    frame.index    = 0;
    
    pThis = (RedClient*)parm;

    m_pRedClient = (RedClient*)parm;
    header = (PACKHEADER*)data;

    switch (header->Cmd)
    {
    case VideoDeviceADD:
        if (!m_bAddDevice)
        {
            m_pVideo = new CVideo;

            printf("add device\n");
            if (m_pVideo)
            {
                bResult = m_pVideo->OpenDevice(0,0);
                printf("add device bResult= %d\n",bResult);
                if (bResult)
                {
                    nheader.Cmd = VideoDeviceStat_OK;
                    nheader.Len = 0;
                    pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
                    m_bAddDevice = true;
                    printf("add device bResult\n");
                    return;
                }

                nheader.Cmd = VideoDeviceStat_Fail;
                nheader.Len = 0;
                pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));

                delete m_pVideo;

                m_pVideo = NULL;
                break;
            }
        }

        nheader.Cmd = VideoDeviceStat_Fail;
        nheader.Len = 0;
        pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
        break;
    case VideoDeviceGetFram:
        printf("VideoDeviceGetFram\n");
        if (m_bAddDevice)
        {
            bResult = m_pVideo->GetFram();
            if (!bResult)
            {
                nheader.Cmd = VideoDeviceStat_Fail;
                nheader.Len = 0;
                pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));

                m_bAddDevice = false;
                
                printf("VideoDeviceGetFram delete m_pVideo\n");
                delete m_pVideo;

                m_pVideo = NULL;

                return;
            }

            nheader.Cmd = VideoDeviceStat_OK;
            nheader.Len = 0;
            pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));

            iCount = m_pVideo->m_nSoulation.size();

            pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&iCount,sizeof(int));
            for (int i = 0; i<m_pVideo->m_nSoulation.size();i++)
            {
                pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&m_pVideo->m_nSoulation.at(i),sizeof(FRAME));
            }

            return;
        }

        nheader.Cmd = VideoDeviceStat_Fail;
        nheader.Len = 0;
        pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
        break;
    case VideoDeviceFram:

            printf("VideoDeviceSetFram\n");
        if (m_bAddDevice)
        {
            bWidth            = *((int*)(data)+2);
            bHeight            = *((int*)(data)+3);
            frame.bWidth    = bWidth;
            frame.bHeight    = bHeight;
            bResult = m_pVideo->SetFram(frame);
            if (bResult)
            {
                nheader.Cmd = VideoDeviceStat_OK;
                nheader.Len = 0;
                pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
//begin  wxr add 2014-2-10
#ifdef _H264_ENCODE_CAM_DATA_
                H264Encode.x264_Clear();
                H264Encode.x264_Init(bWidth,bHeight);
#endif
//end
                return;
            }
            nheader.Cmd = VideoDeviceStat_Fail;
            nheader.Len = 0;
            pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
            m_bAddDevice = false;
            printf("VideoDeviceSetFram delete m_pVideo\n");
            delete m_pVideo;
            m_pVideo = NULL;
            return;
        }

        nheader.Cmd = VideoDeviceStat_Fail;
        nheader.Len = 0;
        pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
        break;
    case VideoDeviceRun:
        if (m_bAddDevice)
        {
            bResult = m_pVideo->StartDevice(TRUE);
            if (!bResult)
            {
                nheader.Cmd = VideoDeviceStat_Fail;
                nheader.Len    = 0;
                pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(PACKHEADER));
                delete m_pVideo;
                printf("VideoDeviceRun delete m_pVideo\n");
                m_pVideo = NULL;
                return;
            }
            nheader.Cmd = VideoDeviceStat_OK;
            nheader.Len    = 0;
            m_bRunDevice = true;
            pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(PACKHEADER));

            return;
        }
        nheader.Cmd = VideoDeviceStat_Fail;
        nheader.Len = 0;
        pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
        break;
    case  VideoDeviceStop:
        if (m_bRunDevice)
        {
            bResult = m_pVideo->StopDevice();

            nheader.Cmd = VideoDeviceStat_OK;
            nheader.Len = 0;
            pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));

            m_bRunDevice = false;

            return;
        }

        nheader.Cmd = VideoDeviceStat_Fail;
        nheader.Len = 0;
        pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
        break;
    case VideoDeviceGetData:
        if (m_bDeviceError)
        {
            nheader.Cmd  = VideoDeviceDelete;
            nheader.Len  = 0;
            pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
            delete m_pVideo;

            m_bAddDevice        = false;
            m_bRunDevice        = false;
            m_bDeviceError        = false;
            m_pVideo            = NULL;
        }
        if (m_bRunDevice)
        {
            if(!m_bThread)
            {
                pthread_t dwThread_id;
    
                pthread_create (&dwThread_id, NULL, &GetVideoDataThread, this);
                m_bThread = true;
            }
            if (m_pVideoBufer != NULL)
            {
                m_bGetVideoData = true;
                
            }
        }

        break;
    case  VideoDeviceStat_Test:
        if (m_bDeviceError)
        {
            nheader.Cmd  = VideoDeviceDelete;
            nheader.Len  = 0;
            pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
            delete m_pVideo;

            m_bAddDevice        = false;
            m_bRunDevice        = false;
            m_bDeviceError        = false;
            m_pVideo            = NULL;
            
            printf("m_bDeviceError=%d\n",m_bDeviceError);
            return;
        }
        nheader.Cmd = VideoDeviceStat_OK;
        nheader.Len = 0;
        pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));

        break;
    default:
        break;
    }
}

bool DelVideoMessage::GetData()
{
    bool            bResult;
    PACKHEADER        nheader;    

    if (m_pVideo == NULL)
    {
        return false;
    }

    m_nVideoDataSize = BUFSIZE;
    m_bGetVideoData  = false;

    if (m_pVideoBufer == NULL)
    {
        return false;
    }

    bResult = m_pVideo->GetVideoData(m_pVideoBufer,m_nVideoDataSize);

    if (!bResult)
    {
        nheader.Cmd = VideoDeviceStat_Fail;
        nheader.Len = 0;
        RedClient::get()->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(nheader));
    }

    return bResult;

}

void* DelVideoMessage::GetVideoDataThread(void* parm)
{
        DelVideoMessage*    pThis;
        bool                bReult;
        pThis = (DelVideoMessage*)parm;

        while (1)
        {
            if (pThis->m_bGetVideoData)
            {    
                
                bReult = pThis->GetData(); 

                if (bReult)
                {
//begin  wxr add 2014-2-10
#ifdef _H264_ENCODE_CAM_DATA_
                
                pThis->SendVideoDataByH264((uint8_t*)pThis->m_pVideoBufer,pThis->m_nVideoDataSize,pThis->m_pRedClient);
#else
                pThis->SendVideoData((uint8_t*)pThis->m_pVideoBufer,pThis->m_nVideoDataSize,pThis->m_pRedClient);
#endif
                }            
            }
            usleep(1);
        }
        return 0;
}

//begin  wxr add 2014-2-11
#ifdef _H264_ENCODE_CAM_DATA_
int DelVideoMessage::SendVideoDataByH264(uint8_t *pVideoData,uint32_t VideoDataSize,void* pClient)
{
    int H264DataLen = H264Encode.EncodeData(pVideoData,m_H264Data,H264DATA_BUFFER_SIZE);    
    if(H264DataLen <= 0 )
    {
        printf("H264Encode.EncodeData error!");
    }
    return SendVideoData(m_H264Data,H264DataLen,pClient);
}
#endif

int DelVideoMessage::SendVideoData(uint8_t *pVideoData,uint32_t VideoDataSize,void* pClient)
{
    int        nRet    = 0;
    int        nLen    = 0;
    int        nCount     = 1;
    RedClient* pThis =  (RedClient*)pClient; 
    nLen   = VideoDataSize;
    char    buf[SENDBUFSIZE];
    PACKHEADER    nheader;


    nheader.Len = VideoDataSize;
    nheader.Cmd = VideoDeviceStat_OK;
    pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)&nheader,sizeof(PACKHEADER));

    while (1)
    {
        memset(buf,0,SENDBUFSIZE);
        if (nLen -nRet > SENDBUFSIZE)
        {
            memcpy(buf,pVideoData+nRet,SENDBUFSIZE);
            pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)buf,SENDBUFSIZE);
            nRet = SENDBUFSIZE*nCount;
            usleep(3000);
            nCount++;
        }
        else
        {
            memcpy(buf,pVideoData+nRet,nLen-nRet);
            pThis->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,0,(uint8_t*)buf,nLen-nRet);
            m_bData = false;
            return 0;
        }        
    }
    return 0;
}

void DelVideoMessage::AddUsbVideodevices()
{
    libusb_device *dev;
    libusb_device **devs;
    int i = 0;
    int m = 0;
    int cnt;
    
    printf("add video device\n");
    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0)
    {
        libusb_free_device_list(devs, 1);
        
        THROW("retrive usb device list error.");
    }

    char devName[200] = {0};
    while ((dev = devs[i++]) != NULL)
    {
        struct libusb_device_descriptor desc;
        struct libusb_config_descriptor *conf;
        int rValue = libusb_get_device_descriptor(dev, &desc);
        if (rValue < 0) {            
            libusb_free_device_list(devs, 1);
            return;
        }

        m = libusb_get_config_descriptor(dev, 0, &conf);
        if (m < 0)
        {
            LOG_INFO("failed to get config descirptor");
        } 
        else
        {
            memset(devName,0, sizeof(devName));
            printf("bInterfaceClass: %d, bInterfaceProtocol: %d, idProduct: %d, idVendor: %d \n", conf->interface->altsetting->bInterfaceClass, conf->interface->altsetting->bInterfaceProtocol,    desc.idProduct, desc.idVendor);
            LOG_INFO("bInterfaceClass: %d, bInterfaceProtocol: %d, idProduct: %d, idVendor: %d", conf->interface->altsetting->bInterfaceClass, conf->interface->altsetting->bInterfaceProtocol,    desc.idProduct, desc.idVendor);


            uint8_t idev_class = conf->interface->altsetting->bInterfaceClass;
            switch(idev_class)
            {
                case 0x0e:
                    m_nPid = desc.idProduct;
                    m_nVid = desc.idVendor;
                    printf("m_nPid =%d,m_nVid=%d\n",m_nPid,m_nVid);
                    break;
                default:
                    break;
            }
        }
    }
    libusb_free_device_list(devs, 1);
}

void DelVideoMessage::SendMesgAgentAddDevice(int nPid, int nVid)
{
        m_nPid = nPid;
        m_nVid = nVid;
#ifdef USE_DEVREDIR
        {
            printf("SendMesgAgentAddDevice input\n");
            RedClient::get()->send_msg_to_device(VD_AGENT_MSG_DEVICE_FOR_VIDEO,VD_AGENT_DEVICE,1,NULL,0);
        }
#endif        

}

void DelVideoMessage::RemoveUsbVideoDevice(int nPid, int nVid)
{
    printf("input RemoveUsbVideoDevice nPid=%d,nVid=%d\n",nPid,nVid);
    if (m_nPid == nPid && m_nVid == nVid)
    {
        m_bDeviceError = true;
        m_nPid = -1;
        m_nVid = -1;
    }
}
