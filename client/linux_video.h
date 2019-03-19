#ifndef	LINUX_VIDEO_H
#define	LINUX_VIDEO_H

#include <stdio.h>  
#include <stdlib.h> //stdio.h and stdlib.h are needed by perror function  
#include <sys/stat.h>  
#include <errno.h>
#include <sys/types.h>  
#include <fcntl.h> //O_RDWR  
#include <unistd.h>  
#include <sys/mman.h> //unistd.h and sys/mman.h are needed by mmap function  
#include <stdbool.h>//false and true  
#include <sys/ioctl.h>
#include <vector>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/socket.h>
#include <linux/videodev2.h>//v4l2 API
using namespace std;


#ifndef TRUE
#define TRUE 1
#endif


#ifndef FALSE
#define FALSE 0
#endif

typedef struct FRAM
{
	int		index;
	int		bWidth;
	int		bHeight;
}FRAME;

typedef struct PackHeader
{
    int		Len;
    int     Cmd;
    //char    Data[4];
}PACKHEADER;


struct buffer 
{
	void *       start;
	size_t       length;
};

class CVideo
{

public:
	CVideo();
	virtual ~CVideo();

public:
	bool		OpenDevice(int Pid, int Vid);
	bool		StartDevice(bool Start);
    bool		GetFram();
	bool		SetFram(FRAM Parm);
	bool        StopDevice();
    bool		GetVideoData(char* VideoBuf,int& BufSize);

public:
    bool		m_bRun;
    bool		m_bDeviceError;
	vector<FRAM>		m_nSoulation;

private:
	void		WriteErrfun(int len);
	bool        InitMap();

private:
	int			m_nDevice;
	char		m_strErrMsg[256];
	bool		m_bDeleteDevice;
	buffer*     m_pBuffer;
	int 		m_nBuffer;
    bool        m_bInitMap;
};

#endif
