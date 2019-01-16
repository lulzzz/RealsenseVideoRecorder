using Intel.RealSense;
using System;
using System.ComponentModel;
using System.Threading;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using Video_SDK.Basics;
using Video_SDK.Camera;

namespace Video_SDK
{
	public class CameraViewModel:IDisposable
	{
		public event PropertyChangedEventHandler PropertyChanged;
		public CancellationTokenSource TokenSource = new CancellationTokenSource();

		public ImageSource ColorImageSource
		{
			get { return _colorImageSource; }
			set
			{
				_colorImageSource = value;
				RaisePropertyChanged("ColorImageSource");
			}
		}
		private ImageSource _colorImageSource;

		private int _mainWindowLeft;
		public int MainWindowLeft
		{
			get => _mainWindowLeft;
			set
			{
				_mainWindowLeft = value;
				RaisePropertyChanged("MainWindowLeft");
			}
		}
		private int _mainWindowTop;

		public int MainWindowTop
		{
			get => _mainWindowTop;
			set
			{
				_mainWindowTop = value;
				RaisePropertyChanged("MainWindowTop");
			}
		}
		private int _displayWidth;
		public int DisplayWidth
		{
			get => _displayWidth;
			set
			{
				_displayWidth = value;
				RaisePropertyChanged("DisplayWidth");
			}
		}
		private int _displayHeigth;
		public int DisplayHeight
		{
			get => _displayHeigth;
			set
			{
				_displayHeigth = value;
				RaisePropertyChanged("DisplayHeight");
			}
		}
		private string _title;
		public string MainWindowTitle
		{
			get => _title;
			set
			{
				_title = value;
				RaisePropertyChanged("MainWindowTitle");
			}
		}

		private readonly Dispatcher _dispatcher;
		private readonly ICamera _camera;
		private readonly Setup _setup;

		private static Action<VideoFrame> UpdateImage(ImageSource img)
		{
			var wbmp = img as WriteableBitmap;			
			return new Action<VideoFrame>(frame =>
			{
				using (frame)
				{
					var rect = new Int32Rect(0, 0, frame.Width, frame.Height);

					wbmp.WritePixels(rect, frame.Data, frame.Stride * frame.Height, frame.Stride);
				}
			});
		}

		public CameraViewModel(Setup setup)
		{
			_dispatcher = Dispatcher.CurrentDispatcher;
			_camera = CameraFactory.Create();
			_setup = setup;
			MainWindowTitle = $"Gaitrite {setup.CameraParameters.RGBWidth} X " +
				$"{setup.CameraParameters.RGBHeight} @ {setup.CameraParameters.RGBFPS}FPS";

			RearrangeWindow(setup.WindowParameters);
		}

		public void RearrangeWindow(WindowParams dimensions)
		{
			MainWindowLeft = dimensions.Position.X;
			MainWindowTop = dimensions.Position.Y;
			DisplayWidth = dimensions.Size.Width;
			DisplayHeight = dimensions.Size.Height;			
		}

		public void StartCapture()
		{
			Action<VideoFrame> updateColor;
			var size = new System.Drawing.Size(_setup.CameraParameters.RGBWidth, _setup.CameraParameters.RGBHeight);
			SetupWindow(size, out updateColor);
			_camera.StartStreaming(_dispatcher, TokenSource, _setup.CameraParameters, updateColor);
		}

		public void StopCapture()
		{
			_camera.StopStreaming();
		}

		public void StartRecording()
		{
			_camera.StartRecording();
		}

		public void StopRecording()
		{
			_camera.StopRecording();
		}

		public void Save(string name)
		{
			_camera.Save(name);
		}

		public void Close()
		{
			Dispose();
			TokenSource.Cancel();
			_dispatcher.InvokeShutdown();
			_dispatcher.InvokeAsync(() => Environment.Exit(0));
		}

		private void onWindowClosed(object sender, EventArgs e)
		{
			TokenSource.Cancel();
		}

		private void SetupWindow(System.Drawing.Size size, out Action<VideoFrame> color)
		{
			ColorImageSource = new WriteableBitmap(size.Width, size.Height, 96d, 96d, PixelFormats.Bgr24, null);
			color = UpdateImage(ColorImageSource);
		}

		private void RaisePropertyChanged(string propertyName)
		{
			if (Thread.CurrentThread != _dispatcher.Thread)
			{
				_dispatcher.BeginInvoke(DispatcherPriority.DataBind, new Action(() => RaisePropertyChanged(propertyName)));
				return;
			}
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
		}

		#region IDisposable Support
		private bool disposedValue = false;

		protected virtual void Dispose(bool disposing)
		{
			if (!disposedValue)
			{
				if (disposing)
				{
					_camera.Dispose();
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
