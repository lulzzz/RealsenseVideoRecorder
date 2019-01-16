using System;

namespace Video_SDK.Playback
{
    public interface IVideoPlayer
    {
        void Open(IVideoSurface playbackSurface, string filePath, DateTime correlationStartTime);

        void Play();
        void Pause();
        void Seek(double seekTimeSec);
        void Seek(DateTime absoluteTime);
		void Close();
    }
}