// pr_search.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WIN32_DCOM
#define UNICODE
#include <iostream>

//#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "credui.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"winspool")

#include <wincred.h>
#include <strsafe.h>
#include <windows.h>
#include <stdio.h>
#include <winspool.h>
#include <string>
using namespace std;

// Did initialize WinSock
BOOL bWSA;

BOOL EnumJobsForPrinterFunctionAllocatedMemory(wchar_t* PrinterName, JOB_INFO_2W** jobsInformation, int* jobsInformationCount, wchar_t *IPserv);
int getHostIP(wchar_t *compName, wchar_t *IPHost);

void PrintPrinters()
{
	DWORD  Flags = PRINTER_ENUM_FAVORITE | PRINTER_ENUM_LOCAL | PRINTER_ENUM_NETWORK; //network printers

	DWORD cbBuf;
	DWORD pcReturned;
	DWORD index;
	DWORD Level = 1;
	TCHAR Name[500];
	LPPRINTER_INFO_1W pPrinterEnum1 = NULL;

	// Name parameter vanishes
	memset( Name, 0, sizeof( TCHAR) * 500);

	// Ask for the size of the structure, which we will return
	::EnumPrinters( Flags, NULL, Level, NULL, 0, &cbBuf, &pcReturned);

	// Order a memory
	pPrinterEnum1 = ( LPPRINTER_INFO_1W) LocalAlloc( LPTR, cbBuf + 4);

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
	wchar_t szIP[256];
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

BOOL EnumJobsForPrinterFunctionAllocatedMemory(wchar_t* PrinterName, JOB_INFO_2W** jobsInformation, int* jobsInformationCount, wchar_t * IPserv)
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
	
	wchar_t serv[200];
	lstrcpy(serv, printerInfo->pServerName);
	lstrcat( serv,	L"\\root\\cimv2");
	if(getHostIP(serv, IPserv))
		lstrcpy(IPserv,L"IP=???");
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

//Need wmi access
int getHostIP(wchar_t *compName, wchar_t *IPHost)
{
	//wchar_t compName[100] = L"\\\\COMPUTERNAME\\root\\cimv2";
    HRESULT hres;
    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    if (FAILED(hres))
    {
        cout << "Failed to initialize COM library. Error code = 0x" 
            << hex << hres << endl;
        return 1;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    hres =  CoInitializeSecurity(
        NULL, 
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IDENTIFY,    // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
        );

                      
    if (FAILED(hres))
    {
        cout << "Failed to initialize security. Error code = 0x" 
            << hex << hres << endl;
        CoUninitialize();
        return 1;                    // Program has failed.
    }
    
    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator *pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (FAILED(hres))
    {
        cout << "Failed to create IWbemLocator object."
            << " Err code = 0x"
            << hex << hres << endl;
        CoUninitialize();
        return 1;                 // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices *pSvc = NULL;

    // Get the user name and password for the remote computer
    CREDUI_INFO cui;
    bool useToken = false;
    bool useNTLM = true;
    wchar_t pszName[CREDUI_MAX_USERNAME_LENGTH+1] = {0};
    wchar_t pszPwd[CREDUI_MAX_PASSWORD_LENGTH+1] = {0};
    wchar_t pszDomain[CREDUI_MAX_USERNAME_LENGTH+1];
    wchar_t pszUserName[CREDUI_MAX_USERNAME_LENGTH+1];
    wchar_t pszAuthority[CREDUI_MAX_USERNAME_LENGTH+1];
    BOOL fSave;
    DWORD dwErr;

    memset(&cui,0,sizeof(CREDUI_INFO));
    cui.cbSize = sizeof(CREDUI_INFO);
    cui.hwndParent = NULL;
    // Ensure that MessageText and CaptionText identify
    // what credentials to use and which application requires them.
    cui.pszMessageText = TEXT("Press cancel to use process token");
    cui.pszCaptionText = TEXT("Enter Account Information");
    cui.hbmBanner = NULL;
    fSave = FALSE;

    dwErr = CredUIPromptForCredentials( 
        &cui,                             // CREDUI_INFO structure
        TEXT(""),                         // Target for credentials
        NULL,                             // Reserved
        0,                                // Reason
        pszName,                          // User name
        CREDUI_MAX_USERNAME_LENGTH+1,     // Max number for user name
        pszPwd,                           // Password
        CREDUI_MAX_PASSWORD_LENGTH+1,     // Max number for password
        &fSave,                           // State of save check box
        CREDUI_FLAGS_GENERIC_CREDENTIALS |// flags
        CREDUI_FLAGS_ALWAYS_SHOW_UI |
        CREDUI_FLAGS_DO_NOT_PERSIST);  

    if(dwErr == ERROR_CANCELLED)
    {
        useToken = true;
    }
    else if (dwErr)
    {
        cout << "Did not get credentials " << dwErr << endl;
        pLoc->Release();     
        CoUninitialize();
        return 1;      
    }

    // change the computerName strings below to the full computer name
    // of the remote computer
    if(!useNTLM)
    {
        StringCchPrintf(pszAuthority, CREDUI_MAX_USERNAME_LENGTH+1, L"kERBEROS:%s", L"COMPUTERNAME");
    }

    // Connect to the remote root\cimv2 namespace
    // and obtain pointer pSvc to make IWbemServices calls.
    //---------------------------------------------------------
   
    hres = pLoc->ConnectServer(
		BSTR(compName),
        BSTR(useToken?NULL:pszName),    // User name
        BSTR(useToken?NULL:pszPwd),     // User password
        NULL,                              // Locale             
        NULL,                              // Security flags
        BSTR(useNTLM?NULL:pszAuthority),// Authority        
        NULL,                              // Context object 
        &pSvc                              // IWbemServices proxy
        );
    
    if (FAILED(hres))
    {
        cout << "Could not connect. Error code = 0x" 
             << hex << hres << endl;
        pLoc->Release();     
        CoUninitialize();
        return 1;                // Program has failed.
    }

    cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


    // step 5: --------------------------------------------------
    // Create COAUTHIDENTITY that can be used for setting security on proxy

    COAUTHIDENTITY *userAcct =  NULL ;
    COAUTHIDENTITY authIdent;

    if( !useToken )
    {
        memset(&authIdent, 0, sizeof(COAUTHIDENTITY));
        authIdent.PasswordLength = wcslen (pszPwd);
        authIdent.Password = (USHORT*)pszPwd;

        LPWSTR slash = wcschr (pszName, L'\\');
        if( slash == NULL )
        {
            cout << "Could not create Auth identity. No domain specified\n" ;
            pSvc->Release();
            pLoc->Release();     
            CoUninitialize();
            return 1;               // Program has failed.
        }

        StringCchCopy(pszUserName, CREDUI_MAX_USERNAME_LENGTH+1, slash+1);
        authIdent.User = (USHORT*)pszUserName;
        authIdent.UserLength = wcslen(pszUserName);

        StringCchCopyN(pszDomain, CREDUI_MAX_USERNAME_LENGTH+1, pszName, slash - pszName);
        authIdent.Domain = (USHORT*)pszDomain;
        authIdent.DomainLength = slash - pszName;
        authIdent.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        userAcct = &authIdent;

    }

    // Step 6: --------------------------------------------------
    // Set security levels on a WMI connection ------------------

    hres = CoSetProxyBlanket(
       pSvc,                           // Indicates the proxy to set
       RPC_C_AUTHN_DEFAULT,            // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_DEFAULT,            // RPC_C_AUTHZ_xxx
       COLE_DEFAULT_PRINCIPAL,         // Server principal name 
       RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE,    // RPC_C_IMP_LEVEL_xxx
       userAcct,                       // client identity
       EOAC_NONE                       // proxy capabilities 
    );

    if (FAILED(hres))
    {
        cout << "Could not set proxy blanket. Error code = 0x" 
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 7: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        BSTR("WQL"), 
        BSTR("Select Name, HostAddress from Win32_TCPIPPrinterPort"),
		//BSTR("Select * from Win32_OperatingSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);
    
    if (FAILED(hres))
    {
        cout << "Query for operating system name failed."
            << " Error code = 0x" 
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 8: -------------------------------------------------
    // Secure the enumerator proxy
    hres = CoSetProxyBlanket(
        pEnumerator,                    // Indicates the proxy to set
        RPC_C_AUTHN_DEFAULT,            // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_DEFAULT,            // RPC_C_AUTHZ_xxx
        COLE_DEFAULT_PRINCIPAL,         // Server principal name 
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE,    // RPC_C_IMP_LEVEL_xxx
        userAcct,                       // client identity
        EOAC_NONE                       // proxy capabilities 
        );

    if (FAILED(hres))
    {
        cout << "Could not set proxy blanket on enumerator. Error code = 0x" 
             << hex << hres << endl;
        pEnumerator->Release();
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // When you have finished using the credentials,
    // erase them from memory.
    SecureZeroMemory(pszName, sizeof(pszName));
    SecureZeroMemory(pszPwd, sizeof(pszPwd));
    SecureZeroMemory(pszUserName, sizeof(pszUserName));
    SecureZeroMemory(pszDomain, sizeof(pszDomain));


    // Step 9: -------------------------------------------------
    // Get the data from the query in step 7 -------------------
 
    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
   
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
            &pclsObj, &uReturn);

        if(0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        // Get the value of the Name property
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        wcout << " Name Printer: " << vtProp.bstrVal << endl;

        // Get the value of the FreePhysicalMemory property
        hr = pclsObj->Get(L"HostAddress",
            0, &vtProp, 0, 0);
        wcout << " HostAddress of Printer: "
            << vtProp.bstrVal << endl;
		IPHost = vtProp.bstrVal;
        VariantClear(&vtProp);

        pclsObj->Release();
        pclsObj = NULL;
    }

    // Cleanup
    // ========
    
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    if( pclsObj )
    {
        pclsObj->Release();
    }
    
    CoUninitialize();

	
    return 0; 

}
