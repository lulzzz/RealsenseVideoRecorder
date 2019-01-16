#define MODULE_API __declspec(dllexport)

#include "Common.h"
#include "Player.h"
#include "Recorder.h"

extern "C"
{
	MODULE_API int GetImplementationVersion()
	{
		return 28;
	}

//------ Player
	MODULE_API void CreatePlayerEx(CPlayer*** pppPlayer, HWND hVideo, HWND hEvent, INT64 *hr)
	{
		CPlayer** ppPlayer = *pppPlayer;
		*hr = CPlayer::CreateInstance(hVideo, hEvent, ppPlayer);
	}
	MODULE_API void PlayerHandleEventEx(CPlayer** ppPlayer, UINT_PTR pUnkPtr)
	{
		(*ppPlayer)->HandleEvent(pUnkPtr);
	}
	MODULE_API void PlayerOpenEx(CPlayer** ppPlayer, WCHAR *filePath, MFTIME initialSeekTime, INT64 *hr)
	{
		*hr = (*ppPlayer)->OpenURL(filePath, initialSeekTime);
	}
	MODULE_API void PlayerPlayEx(CPlayer** ppPlayer, INT64 *hr)
	{
		*hr = (*ppPlayer)->Play();
	}
	MODULE_API void PlayerPauseEx(CPlayer** ppPlayer)
	{
		(*ppPlayer)->Pause();
	}
	MODULE_API void PlayerShutDownEx(CPlayer** ppPlayer)
	{
		(*ppPlayer)->Shutdown();
	}
	MODULE_API void PlayerSkipToPositionEx(CPlayer** ppPlayer, MFTIME SeekTime)
	{
		CPlayer* pPlayer = *ppPlayer;
		pPlayer->SkipToPosition(SeekTime, false);
	}
	MODULE_API void PlayerSetRateEx(CPlayer** ppPlayer, float rate)
	{
		(*ppPlayer)->SetRate(rate);
	}
	MODULE_API void PlayerResizeEx(CPlayer** ppPlayer, WORD width, WORD height)
	{
		(*ppPlayer)->ResizeVideo(width, height);
	}
//------ Recorder
	MODULE_API void WriteImageToFileEx(CRecorder** ppRecorder, LONGLONG llTimeStamp, BITMAP *pImage)
	{
		(*ppRecorder)->WriteImageSample(llTimeStamp, pImage);
	}
	MODULE_API void StartCaptureImageToFileEx(CRecorder** ppRecorder, WCHAR *filePath, UINT32 width, UINT32 height, UINT32 FPS, INT64 *hr)
	{
		CRecorder *pImageReader = *ppRecorder;

		*hr = pImageReader->StartCaptureToFile(filePath, width, height, FPS);
	}
	MODULE_API void StopCaptureImageToFileEx(CRecorder** ppRecorder)
	{
		(*ppRecorder)->StopCaptureToFile();
	}
	MODULE_API void ConnectToCameraEx(CRecorder*** pppRecorder, INT64 *hr)
	{
		CRecorder** ppRecorder = *pppRecorder;
		*hr = CRecorder::CreateInstance(ppRecorder);
		if (SUCCEEDED(*hr))
		{
			*hr = (**ppRecorder).Connect();
		}
	}
	MODULE_API void DisconnectFromCameraEx(CRecorder** ppRecorder)
	{
		(*ppRecorder)->Disconnect();
	}
}