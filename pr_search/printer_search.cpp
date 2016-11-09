// pr_search.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"winspool")

#include <windows.h>
#include <stdio.h>
#include <winspool.h>
#include <string>

// Did initialize WinSock
BOOL bWSA;

BOOL EnumJobsForPrinterFunctionAllocatedMemory(char* PrinterName, JOB_INFO_2W** jobsInformation, int* jobsInformationCount, char *IPserv);

void PrintPrinters()
{
	DWORD  Flags = PRINTER_ENUM_FAVORITE | PRINTER_ENUM_LOCAL | PRINTER_ENUM_NETWORK; //network printers

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

				JOB_INFO_2W* jobs;
				int jobCount;
				if (EnumJobsForPrinterFunctionAllocatedMemory((pPrinterEnum1+index)->pName, &jobs, &jobCount, szIP))
				{
					int cntPages = 0;
					for (int i = 0; i < jobCount; i++)
					{
						cntPages +=(jobs+i)->TotalPages;
					}
					printf("%s, %d\n", szIP, cntPages);

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
//convert wstring to string
std::string wstrtostr(const std::wstring &wstr) 
{ 
	// Convert a Unicode string to an ASCII string 
	std::string strTo; 
	char *szTo = new char[wstr.length() + 1]; 
	szTo[wstr.size()] = '\0'; 
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL); 
	strTo = szTo; 
	delete[] szTo; 
	return strTo; 
} 
BOOL EnumJobsForPrinterFunctionAllocatedMemory(char* PrinterName, JOB_INFO_2W** jobsInformation, int* jobsInformationCount, char * IPserv)
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
	std::string serv_name;
	serv_name = wstrtostr(printerInfo->pServerName);
	struct hostent *hp = gethostbyname(serv_name.c_str());
	if(!hp)
		strcpy(IPserv, "ip=???");
	else
		strcpy(IPserv,inet_ntoa(*((struct in_addr *)hp->h_addr)));
	
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

