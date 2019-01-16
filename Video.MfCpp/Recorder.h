#pragma once
class CRecorder : public IMFSourceReaderCallback
{
public:
	static HRESULT CreateInstance(CRecorder **ppPlayer);

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFSourceReaderCallback methods
	STDMETHODIMP OnReadSample(
		HRESULT hrStatus,
		DWORD dwStreamIndex,
		DWORD dwStreamFlags,
		LONGLONG llTimestamp,
		IMFSample *pSample
	);
	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *)
	{
		return S_OK;
	}

	STDMETHODIMP OnFlush(DWORD)
	{
		return S_OK;
	}

	HRESULT		Connect();
	HRESULT		Disconnect();
	HRESULT		StartCaptureToFile(const WCHAR *filePath, UINT32 width, UINT32 height, UINT32 fps);
	HRESULT		StopCaptureToFile();
	HRESULT		WriteImageSample(LONGLONG llTimeStamp, BITMAP *pImage);
	HRESULT		CreateMediaSample(LONGLONG llTimeStamp, BITMAP *pImage, IMFSample **ppSample);

	HRESULT		RuntimeError;
	BOOL		IsConnected = 0;

protected:

	CRecorder();
	virtual ~CRecorder();

	HRESULT	ConfigureCaptureToFile(IMFSinkWriter *pWriter, UINT32 bitrate);

	long                    m_nRefCount;
	CRITICAL_SECTION        m_critsec;
	BOOL                    m_bFirstSample;
	LONGLONG                m_llBaseTime;
	IMFSinkWriter           *m_pWriter;

	// Video Format Constants
	GUID					DEFAULT_VIDEO_FORMAT = MFVideoFormat_H264;
	GUID					VIDEO_INPUT_FORMAT = MFVideoFormat_RGB24;
	UINT32					DEFAULT_BIT_RATE = 240 * 1000 * 10;
	UINT32					MAX_KEYFRAME_SPACING = 10;
	UINT32					VIDEO_WIDTH;
	UINT32					VIDEO_HEIGHT;
	UINT32					VIDEO_FPS;
};