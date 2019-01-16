using System;
using System.Runtime.InteropServices;
using Video_SDK.Basics;

namespace Video_SDK
{
	internal class Recorder : IRecorder
	{
		private readonly object _lock = new object();
		private bool _isCapturing;
		private IntPtr _cameraReaderPointer;
		private DateTime _startTime;

		public void ConfigureCaptureToFile(string path, CameraSetup setup)
		{
			Connect();

			lock (_lock)
			{
				if (_isCapturing)
				{
					CppAssembly.StopCaptureImageToFile(_cameraReaderPointer);
				}

				CppAssembly.StartCaptureImageToFile(_cameraReaderPointer, path, (uint)setup.RGBWidth, (uint)setup.RGBHeight, (uint)setup.RGBFPS, out var invokeResult);
				_isCapturing = true;
				_startTime = DateTime.Now;
				if (invokeResult != 0)
				{
					throw new InvalidOperationException($"StartRecording Failed #{invokeResult}.");
				}
			}
		}

		public void RecordBitmap(IntPtr pFrame)
		{
			lock (_lock)
			{
				if (_isCapturing)
				{
					var timestamp = (long)(DateTime.Now - _startTime).TotalMilliseconds * 10000;
					CppAssembly.WriteImageToFile(_cameraReaderPointer, timestamp, pFrame);
				}
			}
		}		

		public void StopCaptureToFile()
		{
			if (!IsConnected())
			{
				return;
			}
			lock (_lock)
			{
				if (!_isCapturing)
				{
					return;
				}
				CppAssembly.StopCaptureImageToFile(_cameraReaderPointer);
				_isCapturing = false;
			}
		}
		
		public void Dispose()
		{
			Disconnect();
			DisposePointerToCamera();
		}

		public bool IsConnected()
		{
			return _cameraReaderPointer != IntPtr.Zero;
		}

		private void Disconnect()
		{
			if (_cameraReaderPointer == IntPtr.Zero)
			{
				return;
			}

			lock (_lock)
			{
				CppAssembly.DisconnectFromCamera(_cameraReaderPointer);
				_isCapturing = false;
			}
		}

		private void Connect()
		{
			Dispose();
			try
			{
				lock (_lock)
				{
					_cameraReaderPointer = Marshal.AllocHGlobal(IntPtr.Size);
					CppAssembly.ConnectToCamera(out _cameraReaderPointer, out var res);
					if (res != 0)
					{
						throw new InvalidOperationException($"Failed to connect to camera #{res}.");
					}
				}
			}
			catch (Exception)
			{
				DisposePointerToCamera();
				throw;
			}
		}

		private void DisposePointerToCamera()
		{
			if (_cameraReaderPointer == IntPtr.Zero)
			{
				return;
			}
			Marshal.FreeHGlobal(_cameraReaderPointer);
			_cameraReaderPointer = IntPtr.Zero;
		}
	}
}