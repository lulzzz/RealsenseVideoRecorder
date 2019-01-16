#include "Common.h"
#include <shlwapi.h>
#include "Recorder.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>

HRESULT CRecorder::CreateInstance(CRecorder **ppPlayer)
{
	if (ppPlayer == NULL)
	{
		return E_POINTER;
	}

	CRecorder *pPlayer = new (std::nothrow) CRecorder();

	if (pPlayer == NULL)
	{
		return E_OUTOFMEMORY;
	}

	*ppPlayer = pPlayer;
	(*ppPlayer)->AddRef();

	SafeRelease(&pPlayer);
	return S_OK;
}

CRecorder::CRecorder() :
	m_pWriter(NULL),
	m_nRefCount(1),
	IsConnected(0)
{
	InitializeCriticalSection(&m_critsec);
}

CRecorder::~CRecorder()
{
	Disconnect();
	DeleteCriticalSection(&m_critsec);
}

HRESULT CRecorder::Disconnect()
{
	HRESULT hr = S_OK;
	EnterCriticalSection(&m_critsec);

	if (m_pWriter)
	{
		m_pWriter->Finalize();
	}

	SafeRelease(&m_pWriter);
	IsConnected = false;
	LeaveCriticalSection(&m_critsec);

	return hr;
}

ULONG CRecorder::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG CRecorder::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}

	return uCount;
}

HRESULT CRecorder::QueryInterface(REFIID riid, void** ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(CRecorder, IMFSourceReaderCallback),
		{ 0 },
	};

	return QISearch(this, qit, riid, ppv);
}

HRESULT CRecorder::OnReadSample(HRESULT hrStatus, DWORD /* dwStreamIndex */, DWORD /* dwStreamFlags */, LONGLONG llTimeStamp, IMFSample *pSample)
{	
	return S_OK;
}

HRESULT CRecorder::WriteImageSample(LONGLONG llTimeStamp, BITMAP *pImage)
{
	HRESULT hr = TRUE;
	IMFSample *pSample = NULL;

	if (m_pWriter)
	{
		hr = CreateMediaSample(llTimeStamp, pImage, &pSample);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pWriter->WriteSample(0, pSample);
	}

	SafeRelease(&pSample);
	return hr;
}

HRESULT CRecorder::CreateMediaSample(LONGLONG llTimeStamp, BITMAP *pImage, IMFSample **ppSample)
{
	*ppSample = NULL;
	HRESULT hr = S_OK;
	IMFSample *pSample = NULL;
	IMFMediaBuffer *pBuffer = NULL;
	BYTE *pData = NULL;

	const LONG cbWidth = 3 * VIDEO_WIDTH;
	const DWORD cbBuffer = cbWidth * VIDEO_HEIGHT;

	hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

	if (m_bFirstSample)
	{
		m_llBaseTime = llTimeStamp;
		m_bFirstSample = FALSE;
	}
	llTimeStamp -= m_llBaseTime;

	if (SUCCEEDED(hr))
	{
		hr = pBuffer->Lock(&pData, NULL, NULL);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFCopyImage(
			pData,                      // Destination buffer.
			cbWidth,                    // Destination stride.
			(BYTE*)pImage,				// First row in source image.
			cbWidth,                    // Source stride.
			cbWidth,                    // Image width in bytes.
			VIDEO_HEIGHT                // Image height in pixels.
		);
	}
	if (pBuffer)
	{
		hr = pBuffer->Unlock();
	}

	if (SUCCEEDED(hr))
	{
		hr = pBuffer->SetCurrentLength(cbBuffer);
	}

	if (SUCCEEDED(hr))
	{
		hr = MFCreateSample(&pSample);
	}

	if (SUCCEEDED(hr))
	{
		hr = pSample->AddBuffer(pBuffer);
	}

	if (SUCCEEDED(hr))
	{
		hr = pSample->SetSampleTime(llTimeStamp);
	}

	*ppSample = pSample;
	(*ppSample)->AddRef();

	SafeRelease(&pSample);
	SafeRelease(&pBuffer);
	return hr;
}

HRESULT CRecorder::StartCaptureToFile(const WCHAR *filePath, UINT32 width, UINT32 height, UINT32 fps) 
{
	if (!IsConnected) 
	{
		return E_POINTER;
	}

	VIDEO_WIDTH = width;
	VIDEO_HEIGHT = height;
	VIDEO_FPS = fps;

	IMFSinkWriter *pWriter;
	HRESULT hr = MFCreateSinkWriterFromURL(filePath, NULL, NULL, &pWriter);

	if (SUCCEEDED(hr))
	{
		hr = ConfigureCaptureToFile(pWriter, DEFAULT_BIT_RATE);
	}

	if (SUCCEEDED(hr)) {
		m_bFirstSample = TRUE;
		m_llBaseTime = 0;
		m_pWriter = pWriter;
	}
	SafeRelease(&pWriter);
	return hr;
}

HRESULT CRecorder::StopCaptureToFile()
{
	HRESULT hr = S_OK;
	EnterCriticalSection(&m_critsec);

	if (m_pWriter)
	{
		hr = m_pWriter->Finalize();
	}
	SafeRelease(&m_pWriter);
	LeaveCriticalSection(&m_critsec);
	hr = MFShutdown();
	return hr;
}

HRESULT CRecorder::Connect()
{
	IMFMediaSource  *pSource = NULL;
	IMFAttributes   *pAttributes = NULL;
	IMFMediaType    *pType = NULL;

	EnterCriticalSection(&m_critsec);

	if (IsConnected) {
		return S_OK;
	}

	HRESULT hr = MFStartup(MF_VERSION);
	if (SUCCEEDED(hr))
	{
		hr = Disconnect();
	}

	if (SUCCEEDED(hr))
	{
		IsConnected = TRUE;
	}

	if (FAILED(hr))
	{
		if (pSource)
		{
			pSource->Shutdown();
		}
		Disconnect();
	}

	SafeRelease(&pSource);
	SafeRelease(&pAttributes);
	SafeRelease(&pType);
	LeaveCriticalSection(&m_critsec);
	return hr;
}

HRESULT CRecorder::ConfigureCaptureToFile(IMFSinkWriter *pWriter, UINT32 bitrate)
{

	DWORD streamIndex;
	IMFMediaType *pType = NULL;
	IMFMediaType *pMediaTypeIn = NULL;
	IMFMediaType *pMediaTypeOut = NULL;
	HRESULT hr;

	if (SUCCEEDED(hr))
	{
		hr = MFTRegisterLocalByCLSID(
			__uuidof(CColorConvertDMO),
			MFT_CATEGORY_VIDEO_PROCESSOR,
			L"",
			MFT_ENUM_FLAG_SYNCMFT,
			0,
			NULL,
			0,
			NULL
		);
	}
	hr = MFCreateMediaType(&pMediaTypeOut);

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	}

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, DEFAULT_VIDEO_FORMAT);
	}

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, bitrate);
	}

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_MAX_KEYFRAME_SPACING, MAX_KEYFRAME_SPACING);
	}

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	}

	if (SUCCEEDED(hr))
	{
		
		hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	}

	if (SUCCEEDED(hr))
	{
		hr = pWriter->AddStream(pMediaTypeOut, &streamIndex);
	}

	if (SUCCEEDED(hr))
	{
		hr = MFCreateMediaType(&pMediaTypeIn);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, VIDEO_INPUT_FORMAT);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	}
	//Trial to improve KEY FRAME SPACING
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetUINT32(MF_MT_MAX_KEYFRAME_SPACING, MAX_KEYFRAME_SPACING);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	}

	if (SUCCEEDED(hr))
	{
		hr = pWriter->SetInputMediaType(streamIndex, pMediaTypeIn, NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = pWriter->BeginWriting();
	}

	pWriter->AddRef();
	SafeRelease(&pType);
	SafeRelease(&pMediaTypeOut);
	SafeRelease(&pMediaTypeIn);
	return hr;
}
