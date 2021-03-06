#pragma once
#include "../TinyEngineBase.h"
#include <Tlhelp32.h>
#include "dbghelp.h"
#include <tchar.h>

class MiniDumper
{
public:
	MiniDumper(bool headless);

private:
	static MiniDumper *gpDumper;
	static LONG WINAPI Handler( struct _EXCEPTION_POINTERS *pExceptionInfo );

	_EXCEPTION_POINTERS *m_pExceptionInfo;
	time_t m_lTime;

	TCHAR m_szDumpPath[_MAX_PATH];
	TCHAR m_szAppPath[_MAX_PATH];
	TCHAR m_szAppBaseName[_MAX_PATH];
	LONG WriteMiniDump(_EXCEPTION_POINTERS *pExceptionInfo );
	BOOL m_bHeadless;

	virtual void VSetDumpFileName(void);
	virtual MINIDUMP_USER_STREAM_INFORMATION *VGetUserStreamArray() { return NULL; }
	virtual const TCHAR *VGetUserMessage() { return _T(""); }
};

