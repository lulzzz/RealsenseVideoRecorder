using Intel.RealSense;
using System;
using System.Threading;
using System.Windows.Threading;
using Video_SDK.Basics;

namespace Video_SDK.Camera
{
	public interface ICamera : IDisposable
	{
		void StartStreaming(Dispatcher dispatcher, CancellationTokenSource tokenSource, CameraSetup setup, Action<VideoFrame> updateColor);
		void StopStreaming();
		void StartRecording();
		void StopRecording();
		void Save(string name);
	}
}