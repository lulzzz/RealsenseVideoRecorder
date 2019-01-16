#pragma once

#include <evr.h>
#include "atlcomcli.h"

const UINT WM_APP_PLAYER_EVENT = WM_APP + 1;

enum PlayerState
{
	Closed = 0,     // No session.
	Ready,          // Session was created, ready to open a file. 
	OpenPending,    // Session is opening a file.
	Started,        // Session is playing a file.
	Paused,         // Session is paused.
	Stopped,        // Session is stopped (ready to play). 
	Closing         // Application has closed the session, but is waiting for MESessionClosed.
};

class CPlayer : public IMFAsyncCallback
{
public:
	static HRESULT CreateInstance(HWND hVideo, HWND hEvent, CPlayer **ppPlayer);

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP GetParameters(DWORD*, DWORD*)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult);

	HRESULT     OpenURL(const WCHAR *sURL, MFTIME initialSeekTime);
	HRESULT     Shutdown();
	HRESULT     Play();
	HRESULT		GetTime(MFTIME * phnsTime);
	HRESULT		HandleNotifyPresentationTime(IMFMediaEvent * pEvent);
	HRESULT		HandleSessionStarted(IMFMediaEvent * pEvent);
	HRESULT		InitFromSession();
	HRESULT		Reset();
	HRESULT		RemoveResamplerNode(IMFTopology * pTopology);
	bool		GetKeyFrameTime(double userTime);
	HRESULT     Pause();
	HRESULT     HandleEvent(UINT_PTR pUnkPtr);
	HRESULT		SkipToPosition(MFTIME SeekTime, BOOL anyState);
	HRESULT		SetRate(float fRate);	
	HRESULT     Repaint();
	HRESULT     ResizeVideo(WORD width, WORD height);

protected:
	CPlayer(HWND hVideo, HWND hEvent);
	virtual ~CPlayer();
	HRESULT Initialize();
	HRESULT CloseSession();
	HRESULT CreateSession();
	HRESULT CreateMediaSource(const WCHAR *sURL);
	HRESULT CreateTopologyFromSource(IMFTopology **ppTopology);
	HRESULT AddBranchToPartialTopology(IMFTopology *pTopology, IMFPresentationDescriptor *pSourcePD, DWORD iStream);
	HRESULT StartPlayback();

	HRESULT OnTopologyReady(IMFMediaEvent *pEvent);
	HRESULT OnSessionStarted(IMFMediaEvent *pEvent);
	HRESULT OnSessionClosed(IMFMediaEvent *pEvent);
	HRESULT OnPresentationEnded(IMFMediaEvent *pEvent);

	IMFMediaSession         *m_pSession;
	IMFVideoDisplayControl  *m_pVideoDisplay;
	IMFMediaSource          *m_pSource;
	IMFRateSupport          *m_pRateSupport;
	IMFRateControl          *m_pRate;

	long                    m_nRefCount;        
	HWND                    m_hwndVideo;        
	HWND                    m_hwndEvent;        
	PlayerState             m_state;            
	HANDLE                  m_hCloseEvent;      
	float					m_deviceRate;		
	float					m_userRate;		    
	MFTIME					m_initialSeekTime;
	bool					m_fReceivedTime;
	MFTIME					m_hnsOffsetTime;
	MFTIME					m_hnsMediastartOffset;
	MFTIME					m_hnsStartTime;
	MFTIME					m_hnsStartTimeAtOutput;
	MFTIME					m_currentTime;
	CComPtr<IMFPresentationClock> m_spSessionClock;
};