using System;
using Video_SDK.Basics;

namespace Video_SDK
{
	public interface IRecorder : IDisposable
    {     
        void ConfigureCaptureToFile(string filePath, CameraSetup setup);
		void RecordBitmap(IntPtr pFrame);
		void StopCaptureToFile();
	}
}