#include <new>
#include <windows.h>
#include <windowsx.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <Wmcodecdsp.h>
#include <mferror.h>

#include <strsafe.h>
#include <assert.h>

#include <ks.h>
#include <ksmedia.h>
#include <Dbt.h>

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

template <class T>
inline void SAFE_RELEASE(T*& p)
{
	if (p)
	{
		p->Release();
		p = NULL;
	}
}

#define IF_FAILED_GOTO(hr, label) if (FAILED(hr)) { goto label; }
#define CHECK_HR(hr) IF_FAILED_GOTO(hr, done)