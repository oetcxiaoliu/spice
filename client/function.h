#ifndef __FUNCTION_H__
#define __FUNCTION_H__



#pragma pack(1)

enum PlayCMD{
	// play cmd
	StartPlayCmd = 0,				// Play
	StopPlayCmd,					// 
	PausePlayCmd,					// pause
	ForwardPlayCmd,				// ���
	RewindPlayCmd,					// ����
	PosPlayCmd,						//��������Ϣ
	MinisizeCmd,
	RestoreCmd,
	

	//postion cmd
	FrameRectCmd,
	SeekFileCmd,
	ReadDataCmd,//

    RoleCommand, 				// 0����ѧ����1������ʦ

	CloseCmd,
	SeekOKCommand,
	VideoRectCmd,
	VoiceMuteCmd,
	VoiceNormalCmd,
	VoiceSeekCmd,
	VideoSeekCmd,
	ResumePlayCmd,
	SeekCountCmd,
	StopRead,
	ResumeRead,
	ResetSeekCount,
	ResetSeekCountOK,

	StuConnLateInfo,
	StuConnLateStartPlay,
	
	BroadcastVideoInfo,
	StudentConnectLate,
		
	StuConnLateStartRecvBroadcast,
	FullScreen,
	ExitFullScreen,
	SendBroadcastAgain,
	VideoExit,
};

// header
typedef struct Headertag{
	int Length;
     int CMD;
}PlayerCMDHeader;

typedef struct PlayerCMDPtrRead
{
	PlayerCMDHeader Header;
	unsigned int pos_read;
}PlayerCMDPtrRead;


typedef struct PlayerCMDData
{
	PlayerCMDHeader Header;
	unsigned int pos_write;
	int seek_count;
}PlayerCMDData;

// StartPlayCmd / ForwardPlayCmd / RewindPlayCmd / PosPlayCmd / SeekFileCmd
typedef struct PlayPostiontag{
	PlayerCMDHeader Header;
	unsigned int Postion;
}PlayPostion;
typedef struct VideoRectRositiontag{
	PlayerCMDHeader Header;
	 int weight;
	 int height;
}VideoRectRosition;
typedef struct SoundLevertag{
	PlayerCMDHeader Header;
	unsigned int Lever;
}SoundLever;

#if 0
	// Player picture Rect / FrameRectCmd
	typedef struct PlayerFrameRecttag{
		PlayerCMDHeader Header;
		//RECT FrameRect;
	//	char filename[MAX_URL_LENGTH];
		int Left;
		int Top;
	}PlayerFrameRect;
#endif
typedef struct PlayerFrameRecttag{
	int left;
	int top;
	int width;
	int height;
	int p2p_bordcast;
	long long file_size;
	//int decode_type;
}PlayerFrameRect;



#pragma pack()

#endif
