#include "../CMainServer/CMainServer.h"
#include "ExceptionHandler.h"
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include <DbgHelp.h>
#include <io.h>
#include <direct.h>
#include <Shlwapi.h>
#include "../Helper.h"
//////////////////////////////////////////////////////////////////////////
const char* g_pszExecuteFunc_WorldThread = NULL;
const char* g_pszExecuteFunc_ServerThread = NULL;
const char* g_pszExecuteFunc_DBThread = NULL;
const char* g_pszExecuteFunc_UIThread = NULL;

int g_nExecuteFunc_WorldThread = 0;
//////////////////////////////////////////////////////////////////////////
LONG WINAPI BM_UnhandledExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
	CMainServer::GetInstance()->SetAppException();

	char szBuf[MAX_PATH];
	sprintf(szBuf, "%s\\Dump",
		GetRootPath());

	if(!PathFileExists(szBuf))
	{
		mkdir(szBuf);
	}

	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(szBuf, "%s\\Dump\\%02d-%02d-%02d-%02d-%02d.dmp",
		GetRootPath(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);

	HANDLE hFile = ::CreateFile(  
		szBuf,   
		GENERIC_WRITE,   
		0,   
		NULL,   
		CREATE_ALWAYS,   
		FILE_ATTRIBUTE_NORMAL,   
		NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION einfo;
		einfo.ThreadId = GetCurrentThreadId();
		einfo.ExceptionPointers = pExceptionInfo;
		einfo.ClientPointers = FALSE;

		MiniDumpWriteDump(GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			MiniDumpWithFullMemory,
			&einfo,
			NULL,
			NULL);

		CloseHandle(hFile);
	}

	sprintf(szBuf, "Exception!Address:%08X Code:%d",
		(DWORD)pExceptionInfo->ExceptionRecord->ExceptionAddress,
		pExceptionInfo->ExceptionRecord->ExceptionCode);

	//LOG(ERROR) << "Address:" << (DWORD)pExceptionInfo->ExceptionRecord->ExceptionAddress << " CODE:" << pExceptionInfo->ExceptionRecord->ExceptionCode;
	LOG(ERROR) << szBuf;

	/*if(g_pszExecuteFunc_DBThread != NULL)
	{
		LOG(ERROR) << "FUNCTION IN DB THREAD:" << g_pszExecuteFunc_DBThread;
	}
	if(g_pszExecuteFunc_ServerThread != NULL)
	{
		LOG(ERROR) << "FUNCTION IN SERVER THREAD:" << g_pszExecuteFunc_ServerThread;
	}
	if(g_pszExecuteFunc_WorldThread != NULL)
	{
		LOG(ERROR) << "FUNCTION IN WORLD THREAD:" << g_pszExecuteFunc_WorldThread;
	}
	if(g_pszExecuteFunc_UIThread != NULL)
	{
		LOG(ERROR) << "FUNCTION IN UI THREAD:" << g_pszExecuteFunc_UIThread;
	}
	LOG(ERROR) << "FUNCTION LINE WORLD THREAD:" << g_nExecuteFunc_WorldThread;*/
	google::FlushLogFiles(google::GLOG_INFO);

	LOG(ERROR) << "Stack information generated by StackWalker:";
	StackWalkerLog sw;
	sw.ShowCallstack(GetCurrentThread(), pExceptionInfo->ContextRecord);

	MessageBox(NULL, szBuf, "EXCEPTION", MB_ICONERROR | MB_TASKMODAL);

	return EXCEPTION_EXECUTE_HANDLER;
}


//////////////////////////////////////////////////////////////////////////
void StackWalkerLog::OnOutput(LPCSTR szText)
{
	LOG(ERROR) << szText;
	google::FlushLogFiles(google::GLOG_INFO);
}