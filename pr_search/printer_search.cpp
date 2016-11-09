// pr_search.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"winspool")

#include <windows.h>
#include <stdio.h>
#include <winspool.h>


// Did initialize WinSock
BOOL bWSA;

BOOL EnumJobsForPrinterFunctionAllocatedMemory(char* PrinterName, JOB_INFO_2W** jobsInformation, int* jobsInformationCount);

void PrintPrinters()
{
	DWORD Flags = PRINTER_ENUM_NETWORK; //network printers

	DWORD cbBuf;
	DWORD pcReturned;
	DWORD index;
	DWORD Level = 2;
	TCHAR Name[500];
	LPPRINTER_INFO_2 pPrinterEnum2 = NULL;

	// Name parameter vanishes
	memset( Name, 0, sizeof( TCHAR) * 500);

	// Ask for the size of the structure, which we will return
	::EnumPrinters( Flags, Name, Level, NULL, 0, &cbBuf, &pcReturned);

	// Order a memory
	pPrinterEnum2 = ( LPPRINTER_INFO_2) LocalAlloc( LPTR, cbBuf + 4);

	// If it does not order
	if( !pPrinterEnum2) 
	{
		goto clean_up;
	}

	//Ask for information about the network printers
	if(!EnumPrinters(  Flags,                  // DWORD Flags, printer object types
		NULL,                    // LPTSTR Name, name of printer object
		Level,                   // DWORD Level, information level
		(LPBYTE)pPrinterEnum2,   // LPBYTE pPrinterEnum, printer information buffer
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
			if( pPrinterEnum2) 
			{
				struct hostent *hp = gethostbyname(pPrinterEnum2->pServerName);
				if(!hp)
					strcpy(szIP, "ip=???");
				else
				{
					
					strcpy(szIP,inet_ntoa(*((struct in_addr *)hp->h_addr)));
				
					JOB_INFO_2W* jobs;
					int jobCount;
					if (EnumJobsForPrinterFunctionAllocatedMemory(pPrinterEnum2->pPrinterName, &jobs, &jobCount))
					{
						for (int i = 0; i < jobCount; i++)
						{
							printf("%s, %d", szIP, jobCount);
						}
						free(jobs);
					}
				}
			}
		}
	}

clean_up:

	// Release ordered memory
	if( pPrinterEnum2)
		LocalFree( LocalHandle( pPrinterEnum2));

}
BOOL EnumJobsForPrinterFunctionAllocatedMemory(char* PrinterName, JOB_INFO_2W** jobsInformation, int* jobsInformationCount)
{
	//Return if out parameters aren't available
	if (!jobsInformation || !jobsInformationCount) return FALSE; 

	//Open the printer
	HANDLE printerHandle;
	if (!OpenPrinter(PrinterName, &printerHandle, NULL)) return FALSE;

	//Get the total number of jobs
	PRINTER_INFO_2W* printerInfo;
	DWORD sizeNeededPlaceholder = 0;
	GetPrinter(printerHandle, 2, NULL, 0, &sizeNeededPlaceholder);
	if (sizeNeededPlaceholder == 0)
	{
		ClosePrinter(printerHandle);
		return FALSE;
	}
	printerInfo = (PRINTER_INFO_2W*)malloc(sizeNeededPlaceholder);
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
	JOB_INFO_2W* jobsInformationArray = (JOB_INFO_2W*)malloc(sizeNeededPlaceholder);
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

	// Release WinSock
	if(bWSA)
		WSACleanup();

	return 0;
}

