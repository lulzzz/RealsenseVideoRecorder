using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Threading;
using Video_SDK.Basics;

namespace Video_SDK
{
	[Guid(Sensor3D.ClassId), ClassInterface(ClassInterfaceType.None)]
	public class Sensor3D : ISensor3D
	{
		public const string ClassId = "bf24b6fe-35c3-41ed-84c7-435dca09a11d";
		public const string InterfaceId = "9f6866b3-a43d-4381-9921-40a14e953d22";
		public const string EventsId = "6e85481a-e24e-4ad6-b9ff-aab33ed1bdac";

		const int FPS = 60;
		public bool IsActive => _view != null;

		private CameraView _view;

		public void Close()
		{
			_view?.Dispatcher.Invoke(() =>
			{
				_view?.ViewModel.Close();
				_view?.Close();
				_view = null;
			});
		}

		public void Open(int xCoordinate, int yCoordinate, int windowHeight, int horizontalResolution)
		{
			if (!IsActive)
			{
				Setup setup = SetParameters(xCoordinate, yCoordinate, windowHeight, horizontalResolution);

				var uiThread = new Thread(new ThreadStart(() => showWindow(setup)));
				uiThread.SetApartmentState(ApartmentState.STA);
				uiThread.Priority = ThreadPriority.Highest;
				uiThread.IsBackground = true;
				uiThread.Start();
			}
		}

		public void StartRecording()
		{
			_view?.Dispatcher.Invoke(() => _view.ViewModel.StartRecording());
		}

		public void StopRecording()
		{
			_view?.Dispatcher.Invoke(() => _view.ViewModel.StopRecording());
		}

		public void SaveLast(string name)
		{
			_view?.Dispatcher.Invoke(() => _view.ViewModel.Save(name));
		}

		public int GetMFCppVersion()
		{
			return CppAssembly.GetImplementationVersion();
		}

		private void showWindow(Setup setup)
		{
			using (var cvm = new CameraViewModel(setup))
			{
				_view = new CameraView(cvm);
				_view.ShowDialog();
			}
		}

		private Setup SetParameters(int xCoordinate, int yCoordinate, int windowHeight, int horizontalResolution)
		{
			return new Setup(FPS, xCoordinate, yCoordinate, windowHeight, horizontalResolution);
		}

		#region IDisposable Support
		private bool disposedValue = false;

		protected virtual void Dispose(bool disposing)
		{
			if (!disposedValue)
			{
				if (disposing)
				{
					Close();
					_view?.ViewModel.TokenSource.Dispose();
				}
				disposedValue = true;
			}
		}

		public void Dispose()
		{
			Dispose(true);
		}
		#endregion
	}
}
