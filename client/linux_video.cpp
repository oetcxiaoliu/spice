#include "linux_video.h"
#define DEFAULT_DEVICE "/dev/video0" 
#define DEFAULT_DEVICE1 "/dev/video1"
#define DEFAULT_DEVICE2 "/dev/video2"

FRAME g_fram = {0,320,240};

void saveYuvToBmpFile(char * pData, int iDataLenth,int width,int hight)
{
	static int indx = 0;

	char filename[200];
	
	sprintf(filename,"img%d.yuv",indx);
	
	FILE *fpw = fopen(filename,"wb"); 

	fwrite(pData, 1,iDataLenth,fpw);

	fclose(fpw);

	indx ++;
}



CVideo::CVideo()
{
	m_nDevice			= -1;
	m_pBuffer			= NULL;
	m_bRun				= false;
	m_bDeviceError			= false;
	m_bDeleteDevice			= false;
    	m_bInitMap			= false;
}


CVideo::~CVideo()
{
	m_bRun = true;

	if (m_nDevice != -1)
	{
        printf("delete m_nDevice\n");
		close(m_nDevice);
	}

	if (m_pBuffer)
	{
        if(m_bInitMap)
        {
            for (int i = 0 ;i<4;i++)
            {
                munmap(m_pBuffer[i].start,m_pBuffer[i].length);

                printf("munmap length=%d\n",m_pBuffer[i].length);
            }
        }
		delete[] m_pBuffer;
	}
}


bool CVideo::OpenDevice(int Pid, int Vid)
{
	
    for(int i = 0; i<5;i++)
    {
        if((m_nDevice=open(DEFAULT_DEVICE,O_RDWR|O_NONBLOCK,0))<0)
        {
            perror("v4l2_open fail\n");

           //leep(2);

            if((m_nDevice=open(DEFAULT_DEVICE1,O_RDWR|O_NONBLOCK,0))<0)
      		  {
            		perror("v4l2_open fail\n");

            		//eep(2);
			if((m_nDevice=open(DEFAULT_DEVICE2,O_RDWR|O_NONBLOCK,0))<0)
      		  	{
            			perror("v4l2_open fail\n");
            			sleep(2);
						continue;
				}
           	}
    
        }
        return true;

    }
    printf("open device fail\n");
    return false;
}


bool CVideo::GetFram()
{
	printf("GetFram\n");
	struct v4l2_frmsizeenum fsize;
	v4l2_fmtdesc			fmt;
	FRAM					tmpl;
	int						index	 = 0;
	int						ret      = 0;
    int                     nCount   = 0;
    PACKHEADER              header;

    if (m_nDevice == -1)
	{
		perror("in fuction GetFram m_pDevice == NULL\n");
        header.Cmd = 10;
        header.Len = 0;
		return false;
	}
	
	fmt.index=0;
	fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if(ioctl(m_nDevice,VIDIOC_ENUM_FMT,&fmt)== -1)
	{
		perror("in fuction GetFramVIDIOC_ENUM_FMT\n");

        header.Cmd = 10;
        header.Len = 0;
		return false;
	}

	fsize.index = 0;
	fsize.pixel_format = fmt.pixelformat;

	while ((ret = ioctl(m_nDevice, VIDIOC_ENUM_FRAMESIZES, &fsize)) == 0)
	{
		if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) 
		{
			if((fsize.discrete.width==320) && (fsize.discrete.height == 240))
			{

				tmpl.index		= index;
				tmpl.bWidth		= fsize.discrete.width;
				tmpl.bHeight	= fsize.discrete.height;

				printf("%d*%d\n",tmpl.bWidth,tmpl.bHeight);

				m_nSoulation.push_back(tmpl);
			}
			printf("%d*%d\n",fsize.discrete.width,fsize.discrete.height);

		}
		fsize.index++;
	}
    return true;
}

bool CVideo::SetFram(FRAME nSoulation)
{
	struct v4l2_format fmt;
    struct v4l2_format fmt1;
	
	g_fram = nSoulation;
    if (m_nDevice == -1)
    {
        OpenDevice(0,0);
    }

    fmt.type				= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    fmt.fmt.pix.height		= nSoulation.bHeight;
    fmt.fmt.pix.width       = nSoulation.bWidth;

    fmt1.type				= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt1.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt1.fmt.pix.field = V4L2_FIELD_INTERLACED;


     if(ioctl(m_nDevice, VIDIOC_G_FMT, &fmt1) == -1)
    {
        perror("get resolution fail\n");

    }


    if(fmt1.fmt.pix.height==nSoulation.bHeight&& fmt1.fmt.pix.width==nSoulation.bWidth)
    {
            InitMap();
            return true;
    }

    close(m_nDevice);
    sleep(1);
    OpenDevice(0,0);
    printf("pix.width:\t\t%d\n",fmt.fmt.pix.width);
    printf("pix.height:\t\t%d\n",fmt.fmt.pix.height);
    printf("%d*%d\n",nSoulation.bWidth,nSoulation.bHeight);
    if(ioctl(m_nDevice, VIDIOC_S_FMT, &fmt) == -1)
    {
        perror("set resolution fail\n");

        return false;
    }

    if (m_pBuffer)
    {
        delete[] m_pBuffer;
        m_pBuffer = NULL;
    }

	if (!InitMap())
	{
		perror("InitMap error\n");
		return false;
	}

	return true;
}

bool CVideo::StartDevice(bool start)
{
	enum v4l2_buf_type        type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    printf("input start device\n");
	if (m_nDevice == -1)
	{
        OpenDevice(0,0);

        printf("startdevice open device\n");
	}
	
	if (!m_bRun)
	{
		if (ioctl(m_nDevice,VIDIOC_STREAMON, &type) == -1)
		{
			perror("VIDIOC_STREAMON fail\n");

			return false;
		}

		m_bRun = !m_bRun;
	}
	

	return true;
}

void CVideo::WriteErrfun(int len)
{
	//return false;
}

bool CVideo::StopDevice()
{
	enum v4l2_buf_type        type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (m_nDevice == -1)
	{
		return false;
	}
	
    //rintf("munmap memory input\n");

     printf("m_bRUn =%d\n",m_bRun);
	if (m_bRun)
	{
		if (ioctl(m_nDevice, VIDIOC_STREAMOFF, &type) == -1)
		{
			perror("VIDIOC_STREAMOFF fail\n");

			return false;
		}
		
      //printf("munmap memory\n");
        if(m_bInitMap)
        {
                for (int i = 0 ;i<4;i++)
                {
                    munmap(m_pBuffer[i].start,m_pBuffer[i].length);
                }

                m_bInitMap = false;
        }
        delete[] m_pBuffer;

        m_pBuffer = NULL;
		printf("m_nDevice =%d\n",m_nDevice);
		m_bRun = !m_bRun;

		return true;
	}
	
	printf("m_bRun == fales\n");
	return false;
}


bool CVideo::GetVideoData(char* VideoBuf,int& BufSize)
{
	struct v4l2_buffer buf;
	
	bzero(&buf,sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;


	if (m_nDevice == -1)
	{
		printf("m_nDevice = -1;\n");
		return false;
	}
	
	if (!m_pBuffer)
	{
        //perror("GetVideoData fun m_pBuffer== Null\n");
		printf("GetVideoData fun m_pBuffer== Null\n");
		return false;
	}
	while(true)
	{
		fd_set fds;
		struct timeval tv;
		int r;


		FD_ZERO(&fds);
		FD_SET(m_nDevice,&fds);


		/*Timeout*/
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		r = select(m_nDevice + 1,&fds,NULL,NULL,&tv);


		if(-1 == r)
		{
			if(EINTR == errno)
				continue;
		
			perror("Fail to select");
			//exit(EXIT_FAILURE);
			return false;
			//continue;
		}


		if(0 == r)
		{
			fprintf(stderr,"select Timeout\n");
			//exit(EXIT_FAILURE);
			continue;
		}
		break;
	}
	if (ioctl(m_nDevice, VIDIOC_DQBUF, &buf)<0)
	{
		printf("GetVideoData VIDIOC_DQBUF failed\n");
		return false;
	}

	printf("GetVideoData  lenth = %d  index =%d\n",m_pBuffer[buf.index].length,buf.index);



	memcpy(VideoBuf,m_pBuffer[buf.index].start,m_pBuffer[buf.index].length);	
	
//	saveYuvToBmpFile(VideoBuf,m_pBuffer[buf.index].length,g_fram.bWidth,g_fram.bHeight);


    BufSize = m_pBuffer[buf.index].length;
	if (ioctl(m_nDevice, VIDIOC_QBUF, &buf)<0)
	{
		printf("GetVideoData VIDIOC_QBUF fail\n");
		return false;
	}
	return true;
}

bool CVideo::InitMap()
{
	struct v4l2_requestbuffers req;

    printf("input InitMap\n");
	int			nbuffers	= 0;
	bzero(&req,sizeof(req));
    req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	if (m_nDevice == -1)
	{
		perror("StartDevice function _pDevice == NULL\n");
		return false;
	}


	if(ioctl(m_nDevice,VIDIOC_REQBUFS,&req)==-1)
	{
		perror("ioctl(fd,VIDIOC_REQBUFS,&req)==-1\n");

		return false;
	}

	m_nBuffer - req.count;
	m_pBuffer = new buffer[req.count*sizeof(buffer)];

	if (m_pBuffer  == NULL)
	{
		perror("out memory buffers\n");

        return false;
	}

	for (nbuffers = 0; nbuffers< req.count; nbuffers++)
	{
		struct v4l2_buffer buf;

		bzero(&buf,sizeof(buf));
		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory  = V4L2_MEMORY_MMAP;
		buf.index   = nbuffers;
														
		if (-1 == ioctl(m_nDevice, VIDIOC_QUERYBUF, &buf))
		{
			perror("m_pDevice->fd, VIDIOC_QUERYBUF, &buf\n");
			return false;
		}
		
		m_pBuffer[nbuffers].length = buf.length;

		m_pBuffer[nbuffers].start  =  mmap (NULL ,buf.length,PROT_READ | PROT_WRITE ,MAP_SHARED ,m_nDevice, buf.m.offset);

        printf("InitMap buf.length=%d\n",buf.length);
		if (MAP_FAILED == m_pBuffer[nbuffers].start)
		{
			perror("MAP_FAILED == buffers[nbuffers].start\n");

			return false;
		}
		
		buf.index = nbuffers;
		ioctl(m_nDevice, VIDIOC_QBUF, &buf);
	}

    m_bInitMap	= true;
    return true;
	//m_bInitMap = true;
}


/* for test
int main()
{
	CVideo *m_pVideo = new CVideo;
	m_pVideo->OpenDevice(0,0);
	
	FRAM frame;
	frame.bWidth	= 320;
	frame.bHeight	= 240;
	
	m_pVideo->GetFram();
	
	m_pVideo->SetFram(frame);
	
	m_pVideo->StartDevice(TRUE);
	
	int bufsize = 1;
	
	char *pVideoBufer = new char[1];
	
	for(int i=0; i< 100; i++)
	{	
		m_pVideo->GetVideoData(pVideoBufer,bufsize);		
		sleep(2);
	}
	
	m_pVideo->StopDevice();
	
	delete pVideoBufer;
	
	delete m_pVideo;
	
	return 0;
}
*/
