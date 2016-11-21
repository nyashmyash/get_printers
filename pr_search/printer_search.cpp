// pr_search.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WIN32_DCOM
//#define UNICODE
#include <windows.h>
#include <stdio.h>
#include <lm.h>
#include <iostream>

using namespace std;
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "mpr.lib")
#pragma comment(lib, "ws2_32.lib")
// Максимум ресурсов в container'е
#define MAX_NET_RESOURCES (1024)

// Рекурсивная функция, которая, собственно, и пишет всю инфу
int ResFunc(NETRESOURCE *pNr) ; // pNr - NETRESOURCE container'а, где смотреть содержимое

// Удалось ли проинициализировать WinSock
BOOL bWSA;

int ResFunc(NETRESOURCEA *pNr)
{
    HANDLE hEnum;
    DWORD dwRes;

     // Начнем перечисление сетевых ресурсов
    dwRes = WNetOpenEnum( RESOURCE_GLOBALNET,  // область видимости - вся сеть
                            RESOURCETYPE_PRINT,  // тип ресурсов - диски и принтеры
                            0,  // тип ресурсов - подключаемые и содержащие
                            pNr,  // начать с pNr
                            &hEnum);  // передать хендл в hEnum
    if(dwRes!=NO_ERROR)  // ошибка
    {
        printf("WNetOpenEnum failed with %d\n", dwRes);
        if(dwRes==ERROR_EXTENDED_ERROR)
        {
            DWORD dwError;
            char szErrorBuff[256], szNameBuff[256];

            WNetGetLastErrorA(&dwError, szErrorBuff, 256, szNameBuff, 256);
            printf("%s [%d]: %s\n", szNameBuff, dwError, szErrorBuff);
        }

        return 1;
    }

    NETRESOURCEA NetResource[MAX_NET_RESOURCES];  // буффер для хранения информации про ресурсы
    DWORD dwCount = 0xFFFFFFFF, dwSize = sizeof(NETRESOURCE)*MAX_NET_RESOURCES;

     // Перечислим ресурсы в текущем container'е
    dwRes = WNetEnumResource(hEnum,  // хендл на перечисление
                        &dwCount,  // количество ресурсов
                        (LPVOID*)&NetResource,  // буффер для хранения информации про ресурсы
                        &dwSize);

    if(dwRes!=NO_ERROR)  // ошибка
    {
        printf("WNetEnumResource failed with %d\n", dwRes);
        if(dwRes==ERROR_EXTENDED_ERROR)
        {
            DWORD dwError;
            char szErrorBuff[256], szNameBuff[256];

            WNetGetLastErrorA(&dwError, szErrorBuff, 256, szNameBuff, 256);
            printf("%s [%d]: %s\n", szNameBuff, dwError, szErrorBuff);
        }

        return 2;
    }

     // Будем перебирать все ресурсы
    DWORD dw;
    for(dw=0; dw < dwCount; dw++)
    {
        // Если ресурс - сервер
        if(NetResource[dw].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
        {
            char szIP[256];
            if(bWSA)  // Если WinSock доступен, то определим IP адрес
            {
                char szName[256];
                for(int i=0; NetResource[dw].lpRemoteName[i+1]!='\0'; i++)
                    szName[i]=NetResource[dw].lpRemoteName[i+2];

                struct hostent *hp = gethostbyname(szName);
                if(!hp)
                    strcpy(szIP, "ip=???");
                else
                {
                    DWORD dwIP = ((in_addr*)hp->h_addr_list[0])->S_un.S_addr;
                
                    int a = LOBYTE(LOWORD(dwIP));
                    int b = HIBYTE(LOWORD(dwIP));
                    int c = LOBYTE(HIWORD(dwIP));
                    int d = HIBYTE(HIWORD(dwIP));

                    printf("%s printer ip - %d.%d.%d.%d\n", szName, a, b, c, d);
                }
            }
            else
                printf("ip printer =???");
		}
         // Если ресурс является container'ом, то посмотрим, что в нем есть :-)
        if(NetResource[dw].dwUsage & RESOURCEUSAGE_CONTAINER)
            ResFunc(&NetResource[dw]);
    }

     // Закроем перечисление
    WNetCloseEnum(hEnum);

    return 0;
}

BOOL EnumJobsForPrinterFunctionAllocatedMemory(char* PrinterName, JOB_INFO_2** jobsInformation, int* jobsInformationCount, char *IPserv);

void PrintPrinters()
{
	DWORD  Flags = PRINTER_ENUM_NETWORK; //network printers

	DWORD cbBuf;
	DWORD pcReturned;
	DWORD index;
	DWORD Level = 1;
	TCHAR Name[500];
	LPPRINTER_INFO_1 pPrinterEnum1 = NULL;

	// Name parameter vanishes
	memset( Name, 0, sizeof( TCHAR) * 500);

	// Ask for the size of the structure, which we will return
	::EnumPrinters( Flags, NULL, Level, NULL, 0, &cbBuf, &pcReturned);

	// Order a memory
	pPrinterEnum1 = ( LPPRINTER_INFO_1) LocalAlloc( LPTR, cbBuf + 4);

	// If it does not order
	if( !pPrinterEnum1) 
	{
		goto clean_up;
	}

	//Ask for information about the network printers
	if(!EnumPrinters(  Flags,                  // DWORD Flags, printer object types
		NULL,                    // LPTSTR Name, name of printer object
		Level,                   // DWORD Level, information level
		(LPBYTE)pPrinterEnum1,   // LPBYTE pPrinterEnum, printer information buffer
		cbBuf,                   // DWORD cbBuf, size of printer information buffer
		&cbBuf,                  // LPDWORD pcbNeeded, bytes received or required
		&pcReturned))			   // LPDWORD pcReturned number of printers enumerated
	{          
		goto clean_up;
	}
	char szIP[256];
	// If the returned information to at least one printer
	if( pcReturned > 0) 
	{
		for( index = 0; index < pcReturned; index++) 
		{
			if( pPrinterEnum1+index) 
			{
				printf("%s: %s: %s: %x\n", (pPrinterEnum1+index)->pName,
                                   (pPrinterEnum1+index)->pDescription,
                                   (pPrinterEnum1+index)->pComment,
                                   (pPrinterEnum1+index)->Flags);

				JOB_INFO_2* jobs;
				int jobCount;
				if (EnumJobsForPrinterFunctionAllocatedMemory((pPrinterEnum1+index)->pName, &jobs, &jobCount, szIP))
				{
					int cntPages = 0;
					for (int i = 0; i < jobCount; i++)
					{
						cntPages +=(jobs+i)->TotalPages;
					}
					wprintf(L"%s, %d\n", szIP, cntPages);

					free(jobs);
				}

			}
		}
	}

clean_up:

	// Release ordered memory
	if( pPrinterEnum1)
		LocalFree( LocalHandle( pPrinterEnum1));

}

BOOL EnumJobsForPrinterFunctionAllocatedMemory(char* PrinterName, JOB_INFO_2** jobsInformation, int* jobsInformationCount, char * IPserv)
{
	//Return if out parameters aren't available
	if (!jobsInformation || !jobsInformationCount) return FALSE; 

	//Open the printer
	HANDLE printerHandle;
	if (!OpenPrinter(PrinterName, &printerHandle, NULL)) return FALSE;

	//Get the total number of jobs
	PRINTER_INFO_2* printerInfo;
	DWORD sizeNeededPlaceholder = 0;
	GetPrinter(printerHandle, 2, NULL, 0, &sizeNeededPlaceholder);
	if (sizeNeededPlaceholder == 0)
	{
		ClosePrinter(printerHandle);
		return FALSE;
	}
	printerInfo = (PRINTER_INFO_2*)malloc(sizeNeededPlaceholder);
	if (!printerInfo)
	{
		ClosePrinter(printerHandle);
		return FALSE;
	}
	if (!GetPrinter(printerHandle, 2, (BYTE*)printerInfo, sizeNeededPlaceholder, &sizeNeededPlaceholder))
	{
		free(printerInfo);
		ClosePrinter(printerHandle);
		return FALSE;
	}

	//Get the size of our jobs buffer
	DWORD sizeReturnedPlaceHolder;
	EnumJobs(printerHandle, 0, printerInfo->cJobs, 2, NULL, NULL, &sizeNeededPlaceholder, &sizeReturnedPlaceHolder);
	JOB_INFO_2* jobsInformationArray = (JOB_INFO_2*)malloc(sizeNeededPlaceholder);
	if (!jobsInformationArray)
	{
		free(printerInfo);
		ClosePrinter(printerHandle);
		return FALSE;
	}

	//Get the jobs
	if (!EnumJobs(printerHandle, 0, printerInfo->cJobs, 2, (BYTE*)jobsInformationArray, sizeNeededPlaceholder, &sizeNeededPlaceholder, &sizeReturnedPlaceHolder))
	{
		free(printerInfo);
		CloseHandle(printerHandle);
		free(jobsInformationArray);
		return FALSE;
	}

	*jobsInformation = jobsInformationArray;
	*jobsInformationCount = printerInfo->cJobs;
	free(printerInfo);
	return TRUE;    
}
int main()
{
	// To determine the IP addresses needed WinSock
// Let's try to initialize WinSock
	WORD wVersion = MAKEWORD(1, 1);
	WSADATA wsaData;
	DWORD dwRes = WSAStartup(wVersion, &wsaData);
	bWSA = (dwRes == 0);
	if(!bWSA)  // If the WinSock does not work, then the definition of IP will be available
		printf("WSAStartup() failed with %d, gethostbyname() unavailable\n", dwRes);

	// Get printers
	PrintPrinters();
	ResFunc(NULL);
	// Release WinSock
	if(bWSA)
		WSACleanup();

	return 0;
}

