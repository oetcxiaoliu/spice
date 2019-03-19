/*
   Copyright (C) 2009 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
/* Code by neo. Jul 22, 2013 19:59 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cups/cups.h>
#include <cups/language.h>
#include <cups/http.h>
#include <cups/ipp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common.h"
#include "red_client.h"
#include "utils.h"
#include "devredir_printer.h"


DevredirPrinter::DevredirPrinter(void)
{
    sprintf(printer_opt_scaling, "100");
    sprintf(printer_opt_landscape, "no");
    sprintf(printer_opt_copies, "1");
    sprintf(printer_image_path, "/etc/printer.jpg");
    m_pPrinterParam = NULL;
    m_bPrintFileRecvEnd = true;
    m_iNowRecvLenth = 0;
    m_fP = NULL;
}

DevredirPrinter::~DevredirPrinter(void)
{
}

void DevredirPrinter::msg_deal(void *pHead, void *pData)
{
    VDAgentMessage *msg = (VDAgentMessage *)pHead;
    size_t writeSize = msg->size;
    char *pchData = (char *)pData;
    
    switch (msg->type)
    {
    case AGENT_MSG_DEVICE_START:
        {
            m_bPrintFileRecvEnd = false;
            m_iNowRecvLenth = 0;

            if (m_fP != NULL)
            {
                fclose(m_fP);
                m_fP = NULL;
            }

            if (m_pPrinterParam != NULL)
            {
                delete m_pPrinterParam;
                m_pPrinterParam = NULL;
            }
            m_pPrinterParam = new FREEPRINT_PDU_HEAD;
            memset(m_pPrinterParam, 0, sizeof(FREEPRINT_PDU_HEAD));
            create_uuid_filename();

            FREEPRINT_PDU_HEAD *psFileHead = (FREEPRINT_PDU_HEAD *)pData;
            m_pPrinterParam->FpVersion = psFileHead->FpVersion;
            m_pPrinterParam->Orientation = psFileHead->Orientation;
            m_pPrinterParam->PaperSize = psFileHead->PaperSize;
            m_pPrinterParam->Copies = psFileHead->Copies;
            m_pPrinterParam->ImageSize = psFileHead->ImageSize;

#if 0
            printf("FpVersion: %d, Orientation: %d, PaperSize: %d, Copies: %d, ImageSize: %d \n", 
                    psFileHead->FpVersion, 
                    psFileHead->Orientation, 
                    psFileHead->PaperSize, 
                    psFileHead->Copies, 
                    psFileHead->ImageSize);
#endif

#if 0
            sprintf(printer_opt_scaling, "100");
            sprintf(printer_opt_landscape, psFileHead->Orientation == 2 ? "yes": "no");
            sprintf(printer_opt_copies, m_pPrinterParam->Copies);
#endif
            writeSize = msg->size - sizeof(FREEPRINT_PDU_HEAD);
            pchData = (char *)pData + sizeof(FREEPRINT_PDU_HEAD);
        }
    case AGENT_MSG_DEVICE_DATA:
        {
            write_jpeg_file(pchData, writeSize);
        }
        break;
    case AGENT_MSG_DEVICE_COMPLATE:
        {
            if (m_fP != NULL)
            {
                m_iNowRecvLenth = 0;
                fclose(m_fP);
                m_fP = NULL;
            }
        }
        break;
    default:
        break;
    }
    return;
}

int DevredirPrinter::create_uuid_filename()
{
    char commandstr[80] = {'\0'};
    char filestr[100] = {'\0'};
    FILE *fpcommand = NULL;

    sprintf(commandstr,"cat /proc/sys/kernel/random/uuid");
    fpcommand = popen(commandstr, "r");
    if (fpcommand != NULL)
    {
        fgets(filestr, 36, fpcommand);
        pclose(fpcommand);
    }
    if (!(strcmp(filestr, "") == 0))
    {
       sprintf(printer_image_path, "/etc/%s.jpg", filestr);    
    }
    else
    {
        sprintf(printer_image_path, "/etc/printer.jpg");
    }

    return 0;
}

void *DevredirPrinter::printer_thread(void *lpParameter)
{
    DevredirPrinter* devPrinter = (DevredirPrinter*)lpParameter;
    devPrinter->cups_print_file(devPrinter->printer_image_path);
    remove(devPrinter->printer_image_path);
    pthread_detach(pthread_self());
    return 0;
}

int DevredirPrinter::write_jpeg_file(char *pData, int iLength)
{
    size_t iWrite = 0;    

    if (m_fP == NULL && m_iNowRecvLenth == 0)
    {
        if ((m_fP = fopen(printer_image_path, "wb")) == NULL)
        {
            printf("%s: create file error! \n", __FUNCTION__);
            return -1;
        }
    }
    iWrite = fwrite(pData, 1, iLength, m_fP);
    if (iWrite != iLength)
    {
        fclose(m_fP);
        m_fP = NULL;
        return -1;
    }
    m_iNowRecvLenth += iWrite;
    
    if (m_iNowRecvLenth == m_pPrinterParam->ImageSize)
    {
        fclose(m_fP);
        m_bPrintFileRecvEnd = true;
        m_fP = NULL;
        printf("[%s %s] %s: receive file data over \n", __DATE__, __TIME__, __FUNCTION__);
#if 1
        pthread_create(&thread_id, NULL, printer_thread, this);
        pthread_join(thread_id, NULL);
#endif
    }
    
    return 0;
}

int DevredirPrinter::cups_print_file(char *filepath)
{
    char *file_to_print = filepath;
    cups_dest_t *dest = NULL;
    cups_dest_t *dests;
    int job_id;
    int num_dests;
    const char *value;
    
    num_dests = cupsGetDests(&dests);
    if (num_dests == 0) return -1;
    
    int i;
    cups_dest_t *dest_t;
    for (i = num_dests, dest_t = dests; i > 0; i--, dest_t++)
    {
        if (dest_t->instance)
        {
            printf("%s/%s\n", dest_t->name, dest_t->instance);
        }
        else
        {
            puts(dest_t->name);
            dest = dest_t;
        }
    }
    if (dest)
    {
        value = cupsGetOption("printer-state", dest->num_options, dest->options);
        
        dest->num_options = cupsAddOption("scaling", printer_opt_scaling, dest->num_options, &dest->options);    
        dest->num_options = cupsAddOption("landscape", printer_opt_landscape, dest->num_options, &dest->options);
        dest->num_options = cupsAddOption("copies", printer_opt_copies, dest->num_options, &dest->options);
#if 0
        printf("num_options: %d \n", dest->num_options);
        int j;
        for (j = 0; j < dest->num_options; j++)
        {
            printf("name: %s,  value: %s \n", dest->options[j].name, dest->options[j].value);
        }
#endif    
        if (strtol(value, NULL, 10) < IPP_PRINTER_STOPPED)
        {
            job_id = cupsPrintFile(dest->name, file_to_print, "cloud job", dest->num_options, dest->options);
            printf("status: %s \n", cupsLastErrorString());
        }
        else
        {
            printf("Printer is stopped. Check printer. \n");
            return -1;
        }
    }
    else
    {
        printf("No default printer. \n");
        return -1;
    }
    cupsFreeDests(num_dests, dests);
    
    return 0;
}
