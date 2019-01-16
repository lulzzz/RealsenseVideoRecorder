namespace Video_SDK
{
    public static class RecorderFactory
    {
        public static IRecorder Create()
        {
            return new Recorder();
        }
    }
}