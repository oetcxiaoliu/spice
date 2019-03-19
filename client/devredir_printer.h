#ifndef _H_DEVREDIR_PRINTER
#define _H_DEVREDIR_PRINTER

#include "red_channel.h"
#include "debug.h"

typedef struct {
	short FpVersion;
	short Orientation;
	short PaperSize;
	short Copies;
	int ImageSize;
} FREEPRINT_PDU_HEAD, *PFREEPRINT_PDU_HEAD;

class DevredirPrinter{
public:
    DevredirPrinter(void);
    ~DevredirPrinter(void);
	
public:
	void msg_deal(void *pHead, void *pData);
	int create_uuid_filename();
	static void *printer_thread(void *lpParameter);
	int write_jpeg_file(char *pData, int iLength);
	int cups_print_file(char *filepath);

private:
	FREEPRINT_PDU_HEAD *m_pPrinterParam;
	char printer_opt_scaling[10];
	char printer_opt_landscape[10];
	char printer_opt_copies[10];
	char printer_image_path[150];
	bool m_bPrintFileRecvEnd;
	long m_iNowRecvLenth;
	FILE *m_fP;
	pthread_t thread_id;

};

#endif