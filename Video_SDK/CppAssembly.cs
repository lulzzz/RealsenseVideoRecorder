using System;
using System.Runtime.InteropServices;

namespace Video_SDK
{
	internal static class CppAssembly
	{
		private const string ASSEMBLY_NAME = "Video.MfCpp.dll";

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "GetImplementationVersion")]
		public static extern int GetImplementationVersion();

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "WriteImageToFileEx")]
		public static extern void WriteImageToFile(IntPtr imageReader, long timeStamp, IntPtr pImage);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "StartCaptureImageToFileEx")]
		public static extern bool StartCaptureImageToFile(IntPtr imageReader, [MarshalAs(UnmanagedType.LPWStr)] string filePath,
			UInt32 width, UInt32 height,UInt32 fps, out long hr);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "StopCaptureImageToFileEx")]
		public static extern void StopCaptureImageToFile(IntPtr imageReader);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "ConnectToCameraEx")]
		public static extern void ConnectToCamera(out IntPtr cameraReader, out long hr);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "DisconnectFromCameraEx")]
		public static extern void DisconnectFromCamera(IntPtr cameraReader);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "CreatePlayerEx")]
		public static extern void CreatePlayer(out IntPtr player, IntPtr videoView, IntPtr eventView, out long hr);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "PlayerHandleEventEx")]
		public static extern void PlayerHandleEvent(IntPtr player, IntPtr wParam);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "PlayerOpenEx")]
		public static extern void PlayerOpen(IntPtr player, [MarshalAs(UnmanagedType.LPWStr)] string filePath, long initialSeekTime, out long hr);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "PlayerPlayEx")]
		public static extern void PlayerPlay(IntPtr player, out long hr);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "PlayerPauseEx")]
		public static extern void PlayerPause(IntPtr player);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "PlayerShutDownEx")]
		public static extern void PlayerShutDown(IntPtr player);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "PlayerSkipToPositionEx")]
		public static extern void PlayerSkipToPosition(IntPtr player, long seekTime);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "PlayerSetRateEx")]
		public static extern void PlayerSetRate(IntPtr player, float rate);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "PlayerResizeEx")]
		public static extern void PlayerResize(IntPtr player, ushort width, ushort height);

		[DllImport(ASSEMBLY_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "ConnectToSensorEx")]
		public static extern void ConnectToSensor(out IntPtr sensor, out long hr);
	}
}