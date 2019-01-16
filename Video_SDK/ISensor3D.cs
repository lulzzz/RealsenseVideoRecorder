using System;
using System.Runtime.InteropServices;

namespace Video_SDK
{
	[Guid(Sensor3D.InterfaceId)]
	public interface ISensor3D : IDisposable
	{
		void Open(int xCoordinate, int yCoordinate, int windowHeight, int horizontalResolution);
		void Close();
		void StartRecording();
		void StopRecording();
		void SaveLast(string path);
		int GetMFCppVersion();
	}
}
