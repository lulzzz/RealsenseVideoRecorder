namespace Video_SDK.Playback
{
    public static class VideoPlayerFactory
    {
        public static IVideoPlayer Create()
        {
            return new VideoPlayer();
        }
    }
}