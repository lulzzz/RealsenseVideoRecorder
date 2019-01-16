#define NDEBUG 
#include <evr.h>
#include "atlcomcli.h"
#include "Common.h"
#include "Player.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "intsafe.h"
#include <mferror.h>
#include "assert.h"
#include <initguid.h>

EXTERN_GUID(CLSID_CResamplerMediaObject, 0xf447b69e, 0x1884, 0x4a7e, 0x80, 0x55, 0x34, 0x6f, 0x74, 0xd6, 0xed, 0xb3);

HRESULT CPlayer::CreateInstance(HWND hVideo, HWND hEvent, CPlayer **ppPlayer)
{
	if (ppPlayer == NULL)
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	CPlayer *pPlayer = new CPlayer(hVideo, hEvent);

	if (pPlayer == NULL)
	{
		return E_OUTOFMEMORY;
	}

	hr = pPlayer->Initialize();

	if (SUCCEEDED(hr))
	{
		*ppPlayer = pPlayer;
		(*ppPlayer)->AddRef();
	}

	SAFE_RELEASE(pPlayer);

	return hr;
}

CPlayer::CPlayer(HWND hVideo, HWND hEvent) :
	m_pSession(NULL),
	m_pSource(NULL),
	m_pVideoDisplay(NULL),
	m_hwndVideo(hVideo),
	m_hwndEvent(hEvent),
	m_state(Ready),
	m_hCloseEvent(NULL),
	m_pRate(NULL),
	m_pRateSupport(NULL),
	m_nRefCount(1),
	m_userRate(1),
	m_deviceRate(1),
	m_initialSeekTime(0),
	m_hnsMediastartOffset(0)
{
}

CPlayer::~CPlayer()
{
	Shutdown();
}

HRESULT CPlayer::Initialize()
{
	HRESULT hr = S_OK;

	if (m_hCloseEvent)
	{
		return MF_E_ALREADY_INITIALIZED;
	}

	CHECK_HR(hr = MFStartup(MF_VERSION));

	m_hCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hCloseEvent == NULL)
	{
		CHECK_HR(hr = HRESULT_FROM_WIN32(GetLastError()));
	}

done:
	return hr;
}

ULONG CPlayer::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG CPlayer::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	return uCount;
}

HRESULT CPlayer::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv)
	{
		return E_POINTER;
	}
	if (iid == IID_IUnknown)
	{
		*ppv = static_cast<IUnknown*>(this);
	}
	else if (iid == IID_IMFAsyncCallback)
	{
		*ppv = static_cast<IMFAsyncCallback*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

HRESULT CPlayer::HandleEvent(UINT_PTR pUnkPtr)
{
	HRESULT hr = S_OK;
	HRESULT hrStatus = S_OK;

	MediaEventType meType = MEUnknown;
	MF_TOPOSTATUS TopoStatus = MF_TOPOSTATUS_INVALID;

	IUnknown *pUnk = NULL;
	IMFMediaEvent *pEvent = NULL;

	pUnk = (IUnknown*)pUnkPtr;

	if (pUnk == NULL)
	{
		return E_POINTER;
	}

	CHECK_HR(hr = pUnk->QueryInterface(__uuidof(IMFMediaEvent), (void**)&pEvent));

	CHECK_HR(hr = pEvent->GetType(&meType));

	CHECK_HR(hr = pEvent->GetStatus(&hrStatus));

	if (SUCCEEDED(hrStatus))
	{
		switch (meType)
		{
		case MESessionTopologyStatus:
			CHECK_HR(hr = pEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, (UINT32*)&TopoStatus));
			switch (TopoStatus)
			{
			case MF_TOPOSTATUS_READY:
				hr = OnTopologyReady(pEvent);
				break;
			default:
				break;
			}
			break;
		case MEEndOfPresentation:
			CHECK_HR(hr = OnPresentationEnded(pEvent));
			break;
		}
	}
	else
	{
		hr = hrStatus;
	}

done:
	SAFE_RELEASE(pUnk);
	SAFE_RELEASE(pEvent);
	return hr;
}

HRESULT CPlayer::Invoke(IMFAsyncResult *pResult)
{
	HRESULT hr = S_OK;

	MediaEventType meType = MEUnknown;
	IMFMediaEvent *pEvent = NULL;

	CHECK_HR(hr = m_pSession->EndGetEvent(pResult, &pEvent));

	CHECK_HR(hr = pEvent->GetType(&meType));

	if (meType == MESessionClosed)
	{
		SetEvent(m_hCloseEvent);
	}
	else
	{
		if (meType == MESessionStarted)
		{
			MFTIME phnsTime;
			GetTime(&phnsTime);
			m_currentTime = phnsTime;
			CHECK_HR(HandleSessionStarted(pEvent));
		}
		else if (meType == MESessionNotifyPresentationTime)
		{
			CHECK_HR(HandleNotifyPresentationTime(pEvent));
		}
		else if (meType == MESessionScrubSampleComplete)
		{
			m_state = Paused;
		}

		CHECK_HR(hr = m_pSession->BeginGetEvent(this, NULL));
	}

	if (m_state != Closing)
	{
		pEvent->AddRef();
		PostMessage(m_hwndEvent, WM_APP_PLAYER_EVENT, (WPARAM)pEvent, (LPARAM)0);
	}

done:
	SAFE_RELEASE(pEvent);
	return S_OK;
}

HRESULT CPlayer::Shutdown()
{
	HRESULT hr = S_OK;

	hr = CloseSession();

	MFShutdown();

	if (m_hCloseEvent)
	{
		CloseHandle(m_hCloseEvent);
		m_hCloseEvent = NULL;
	}

	return hr;
}

HRESULT CPlayer::Pause()
{
	HRESULT hr = S_OK;

	if (m_state == Paused)
	{
		return hr;
	}

	if (m_state != Started)
	{
		return MF_E_INVALIDREQUEST;
	}

	if (m_pSession == NULL || m_pSource == NULL)
	{
		return E_UNEXPECTED;
	}

	hr = m_pSession->Pause();
	if (SUCCEEDED(hr))
	{
		m_state = Paused;
	}

	return hr;
}

HRESULT CPlayer::CloseSession()
{
	HRESULT hr = S_OK;
	SAFE_RELEASE(m_pVideoDisplay);

	if (m_pSession)
	{
		DWORD dwWaitResult = 0;
		m_state = Closing;
		CHECK_HR(hr = m_pSession->Close());
		dwWaitResult = WaitForSingleObject(m_hCloseEvent, 5000);
	}

	if (m_pSource)
	{
		m_pSource->Shutdown();
	}

	if (m_pSession)
	{
		m_pSession->Shutdown();
	}

	SAFE_RELEASE(m_pSource);
	SAFE_RELEASE(m_pSession);
	SafeRelease(&m_pRate);
	SAFE_RELEASE(m_pRateSupport);

	m_state = Closed;

done:
	return hr;
}

HRESULT CPlayer::OpenURL(const WCHAR *sURL, MFTIME initialSeekTime)
{
	HRESULT hr = S_OK;

	IMFTopology *pTopology = NULL;
	m_initialSeekTime = initialSeekTime;
	m_currentTime = initialSeekTime;
	m_hnsMediastartOffset = INT64_MAX;

	CHECK_HR(hr = CreateSession());

	CHECK_HR(hr = CreateMediaSource(sURL));

	CHECK_HR(hr = CreateTopologyFromSource(&pTopology));

	CHECK_HR(hr = RemoveResamplerNode(pTopology));

	CHECK_HR(hr = m_pSession->SetTopology(MFSESSION_SETTOPOLOGY_IMMEDIATE, pTopology));

	m_deviceRate = 1;
	m_state = OpenPending;

	InitFromSession();

done:
	if (FAILED(hr))
	{
		m_state = Closed;
	}
	m_fReceivedTime = false;

	SAFE_RELEASE(pTopology);
	return hr;
}

HRESULT CPlayer::CreateSession()
{
	HRESULT hr = S_OK;

	IMFAttributes *pAttributes = NULL;
	IMFActivate   *pEnablerActivate = NULL;

	CHECK_HR(hr = CloseSession());

	assert(m_state == Closed);

	CHECK_HR(hr = MFCreateAttributes(&pAttributes, 1));

	CHECK_HR(hr = MFCreateMediaSession(pAttributes, &m_pSession));

	CHECK_HR(hr = m_pSession->BeginGetEvent((IMFAsyncCallback*)this, NULL));

done:
	SAFE_RELEASE(pAttributes);
	SAFE_RELEASE(pEnablerActivate);
	return hr;
}

HRESULT CPlayer::CreateMediaSource(const WCHAR *sURL)
{
	HRESULT hr = S_OK;

	MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
	IMFSourceResolver* pSourceResolver = NULL;
	IUnknown* pSource = NULL;

	SAFE_RELEASE(m_pSource);

	CHECK_HR(hr = MFCreateSourceResolver(&pSourceResolver));

	CHECK_HR(hr = pSourceResolver->CreateObjectFromURL(
		sURL,                       // URL of the source.
		MF_RESOLUTION_MEDIASOURCE,  // Create a source object.
		NULL,                  // Optional property store.
		&ObjectType,                // Receives the created object type. 
		&pSource                    // Receives a pointer to the media source.
	));

	CHECK_HR(hr = pSource->QueryInterface(__uuidof(IMFMediaSource), (void**)&m_pSource));

done:
	SAFE_RELEASE(pSourceResolver);
	SAFE_RELEASE(pSource);
	return hr;
}

HRESULT CPlayer::CreateTopologyFromSource(IMFTopology **ppTopology)
{
	assert(m_pSession != NULL);
	assert(m_pSource != NULL);

	HRESULT hr = S_OK;
	IMFTopology *pTopology = NULL;
	IMFPresentationDescriptor* pSourcePD = NULL;
	DWORD cSourceStreams = 0;

	CHECK_HR(hr = MFCreateTopology(&pTopology));

	CHECK_HR(hr = m_pSource->CreatePresentationDescriptor(&pSourcePD));

	CHECK_HR(hr = pSourcePD->GetStreamDescriptorCount(&cSourceStreams));

	for (DWORD i = 0; i < cSourceStreams; i++)
	{
		CHECK_HR(hr = AddBranchToPartialTopology(pTopology, pSourcePD, i));
	}

	if (SUCCEEDED(hr))
	{
		*ppTopology = pTopology;
		(*ppTopology)->AddRef();
	}

done:
	SAFE_RELEASE(pTopology);
	SAFE_RELEASE(pSourcePD);
	return hr;
}

HRESULT CreateSourceStreamNode(IMFMediaSource *pSource, IMFPresentationDescriptor *pSourcePD, IMFStreamDescriptor *pSourceSD, IMFTopologyNode **ppNode)
{
	if (!pSource || !pSourcePD || !pSourceSD || !ppNode)
	{
		return E_POINTER;
	}

	IMFTopologyNode *pNode = NULL;
	HRESULT hr = S_OK;

	CHECK_HR(hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode));

	CHECK_HR(hr = pNode->SetUnknown(MF_TOPONODE_SOURCE, pSource));

	CHECK_HR(hr = pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pSourcePD));

	CHECK_HR(hr = pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSourceSD));

	*ppNode = pNode;
	(*ppNode)->AddRef();

done:
	SAFE_RELEASE(pNode);
	return hr;
}

HRESULT CreateOutputNode(IMFStreamDescriptor *pSourceSD, HWND hwndVideo, IMFTopologyNode **ppNode)
{
	IMFTopologyNode *pNode = NULL;
	IMFMediaTypeHandler *pHandler = NULL;
	IMFActivate *pRendererActivate = NULL;
	GUID guidMajorType = GUID_NULL;

	HRESULT hr = S_OK;
	DWORD streamID = 0;

	pSourceSD->GetStreamIdentifier(&streamID);

	CHECK_HR(hr = pSourceSD->GetMediaTypeHandler(&pHandler));

	CHECK_HR(hr = pHandler->GetMajorType(&guidMajorType));

	CHECK_HR(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode));

	if (MFMediaType_Audio == guidMajorType)
	{
		CHECK_HR(hr = MFCreateAudioRendererActivate(&pRendererActivate));
	}
	else if (MFMediaType_Video == guidMajorType)
	{
		CHECK_HR(hr = MFCreateVideoRendererActivate(hwndVideo, &pRendererActivate));
	}
	else
	{
		CHECK_HR(hr = E_FAIL);
	}

	CHECK_HR(hr = pNode->SetObject(pRendererActivate));

	*ppNode = pNode;
	(*ppNode)->AddRef();

done:
	SAFE_RELEASE(pNode);
	SAFE_RELEASE(pHandler);
	SAFE_RELEASE(pRendererActivate);
	return hr;
}

HRESULT CPlayer::AddBranchToPartialTopology(IMFTopology *pTopology, IMFPresentationDescriptor *pSourcePD, DWORD iStream)
{
	assert(pTopology != NULL);

	IMFStreamDescriptor* pSourceSD = NULL;
	IMFTopologyNode* pSourceNode = NULL;
	IMFTopologyNode* pOutputNode = NULL;
	BOOL fSelected = FALSE;

	HRESULT hr = S_OK;

	CHECK_HR(hr = pSourcePD->GetStreamDescriptorByIndex(iStream, &fSelected, &pSourceSD));

	if (fSelected)
	{
		CHECK_HR(hr = CreateSourceStreamNode(m_pSource, pSourcePD, pSourceSD, &pSourceNode));

		CHECK_HR(hr = CreateOutputNode(pSourceSD, m_hwndVideo, &pOutputNode));

		CHECK_HR(hr = pTopology->AddNode(pSourceNode));

		CHECK_HR(hr = pTopology->AddNode(pOutputNode));

		CHECK_HR(hr = pSourceNode->ConnectOutput(0, pOutputNode, 0));
	}

done:
	SAFE_RELEASE(pSourceSD);
	SAFE_RELEASE(pSourceNode);
	SAFE_RELEASE(pOutputNode);
	return hr;
}

HRESULT CPlayer::OnTopologyReady(IMFMediaEvent *pEvent)
{
	MFGetService(m_pSession, MR_VIDEO_RENDER_SERVICE, __uuidof(IMFVideoDisplayControl), (void**)&m_pVideoDisplay);

	MFGetService(m_pSession, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&m_pRateSupport));

	MFGetService(m_pSession, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&m_pRate));

	SkipToPosition(m_initialSeekTime, true);

	return S_OK;
}

HRESULT CPlayer::OnPresentationEnded(IMFMediaEvent *pEvent)
{
	m_state = Stopped;
	return S_OK;
}

HRESULT CPlayer::StartPlayback()
{
	assert(m_pSession != NULL);
	HRESULT hr = SetRate(m_userRate);

	PROPVARIANT varStart;
	PropVariantInit(&varStart);

	varStart.vt = VT_EMPTY;

	if (SUCCEEDED(hr))
	{
		hr = m_pSession->Start(&GUID_NULL, &varStart);
	}
	if (SUCCEEDED(hr))
	{
		m_state = Started;
	}
	PropVariantClear(&varStart);

	return hr;
}

HRESULT CPlayer::SkipToPosition(MFTIME SeekTime, BOOL anyState)
{
	if (!anyState) {
		if (m_state != Paused && m_state != Stopped) {
			return S_OK;
		}
	}

	MFTIME phnsTime;
	PROPVARIANT var;
	PropVariantInit(&var);
	HRESULT hr = S_OK;

	if (m_deviceRate != 0) {
		m_deviceRate = 0.0F;
		hr = m_pRate->SetRate(FALSE, m_deviceRate);
	}

	if (SeekTime == PRESENTATION_CURRENT_POSITION)
	{
		var.vt = VT_EMPTY;
	}
	else
	{
		var.vt = VT_I8;
		var.hVal.QuadPart = SeekTime;
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pSession->Start(&GUID_NULL, &var);
	}

	if (SUCCEEDED(hr))
	{
		hr = GetTime(&phnsTime);
		m_currentTime = phnsTime;
	}
	m_state = Started;
	PropVariantClear(&var);

	return hr;
}

HRESULT CPlayer::SetRate(float fRate)
{
	HRESULT hr = S_OK;
	BOOL bThin = FALSE;

	if (fRate == m_deviceRate)
	{
		return hr;
	}

	if (m_pRateSupport == NULL)
	{
		return MF_E_INVALIDREQUEST;
	}

	hr = m_pRateSupport->IsRateSupported(FALSE, fRate, NULL);
	if (FAILED(hr))
	{
		bThin = TRUE;
		hr = m_pRateSupport->IsRateSupported(TRUE, fRate, NULL);
	}
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_pRate->SetRate(bThin, fRate);
	if (SUCCEEDED(hr))
	{
		m_deviceRate = fRate;
	}

	return hr;
}

HRESULT CPlayer::Repaint()
{
	HRESULT hr = S_OK;

	if (m_pVideoDisplay)
	{
		hr = m_pVideoDisplay->RepaintVideo();
	}
	return hr;
}

HRESULT CPlayer::ResizeVideo(WORD width, WORD height)
{
	HRESULT hr = S_OK;

	if (m_pVideoDisplay)
	{
		RECT rcDest = { 0, 0, width, height };
		hr = m_pVideoDisplay->SetVideoPosition(NULL, &rcDest);
	}
	return hr;
}

HRESULT CPlayer::Play()
{
	if (m_state == Started)
	{
		return S_OK;
	}

	if (m_state != Paused && m_state != Stopped)
	{
		return MF_E_INVALIDREQUEST;
	}
	if (m_pSession == NULL || m_pSource == NULL)
	{
		return E_UNEXPECTED;
	}
	HRESULT hr = StartPlayback();

	return hr;
}

HRESULT CPlayer::GetTime(MFTIME *phnsTime)
{
	HRESULT hr = S_OK;

	CHECK_HR(m_spSessionClock->GetTime(phnsTime));

	if (m_fReceivedTime)
	{
		*phnsTime -= m_hnsOffsetTime;
	}

done:
	return(hr);
}

HRESULT CPlayer::HandleNotifyPresentationTime(IMFMediaEvent* pEvent)
{
	HRESULT hr = S_OK;

	CHECK_HR(hr = (pEvent->GetUINT64(MF_EVENT_START_PRESENTATION_TIME, (UINT64*)&m_hnsStartTime)));

	CHECK_HR(hr = (pEvent->GetUINT64(MF_EVENT_PRESENTATION_TIME_OFFSET, (UINT64*)&m_hnsOffsetTime)));

	CHECK_HR(hr = (pEvent->GetUINT64(MF_EVENT_START_PRESENTATION_TIME_AT_OUTPUT, (UINT64*)&m_hnsStartTimeAtOutput)));

	m_fReceivedTime = true;

done:
	return(hr);
}

HRESULT CPlayer::HandleSessionStarted(IMFMediaEvent* pEvent)
{
	MFTIME hnsTopologyPresentationOffset;

	if (SUCCEEDED(pEvent->GetUINT64(MF_EVENT_PRESENTATION_TIME_OFFSET, (UINT64*)&hnsTopologyPresentationOffset)))
	{
		m_hnsOffsetTime = hnsTopologyPresentationOffset;
	}

	return S_OK;
}

HRESULT CPlayer::InitFromSession()
{
	HRESULT hr = S_OK;
	CComPtr<IMFClock> spClock;

	if (m_spSessionClock.p)
	{
		m_spSessionClock.Release();
	}

	CHECK_HR(hr = Reset());

	CHECK_HR(m_pSession->GetClock(&spClock));

	CHECK_HR(spClock->QueryInterface(IID_IMFPresentationClock, (void**)&m_spSessionClock));

done:
	return hr;
}

HRESULT CPlayer::Reset()
{
	HRESULT hr;
	CHECK_HR(m_pSession->ClearTopologies());

done:
	return hr;
}

HRESULT CPlayer::RemoveResamplerNode(IMFTopology* pTopology)
{
	HRESULT hr = S_OK;

	WORD cNodes;
	CHECK_HR(pTopology->GetNodeCount(&cNodes));

	for (WORD i = 0; i < cNodes; i++)
	{
		CComPtr<IMFTopologyNode> spNode;
		CHECK_HR(pTopology->GetNode(i, &spNode));

		GUID gidTransformID;
		hr = spNode->GetGUID(MF_TOPONODE_TRANSFORM_OBJECTID, &gidTransformID);

		if (SUCCEEDED(hr) && CLSID_CResamplerMediaObject == gidTransformID)
		{
			CComPtr<IMFTopologyNode> spUpstreamNode;
			DWORD dwUpstreamIndex;
			CComPtr<IMFTopologyNode> spDownstreamNode;
			DWORD dwDownstreamIndex;

			CHECK_HR(spNode->GetInput(0, &spUpstreamNode, &dwUpstreamIndex));
			CHECK_HR(spNode->GetOutput(0, &spDownstreamNode, &dwDownstreamIndex));
			CHECK_HR(spUpstreamNode->ConnectOutput(dwUpstreamIndex, spDownstreamNode, dwDownstreamIndex));

			CHECK_HR(pTopology->RemoveNode(spNode));
			CHECK_HR(pTopology->GetNodeCount(&cNodes));

			i--;
		}

		hr = S_OK;
	}

done:
	return hr;
}
