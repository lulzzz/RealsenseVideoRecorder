using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Threading;
using Intel.RealSense;
using Video_SDK.Basics;
using Video_SDK;
using System.IO;
using System.Diagnostics;

namespace Video_SDK.Camera
{
	public class Camera : ICamera
	{
			
		const string VIDEO_FILE_FORMAT = @".mp4";
		const string TEMP_FILE_PATH = @"tempVid_";
		const string VIDEO_FILE_NAME_ROOT = @"Vid_";
		const string SAVED_FILE_PATH = @"Video";

		private CameraSetup _setup;
		private Dispatcher _dispatcher;
		private IRecorder _recorder;
		private Pipeline _pipeline;
		private State _state;
		CancellationTokenSource _tokenSource;

		public void StartStreaming(Dispatcher dispatcher, CancellationTokenSource tokenSource, CameraSetup setup, Action<VideoFrame> updateColor)
		{
			if (dispatcher.Thread != Thread.CurrentThread)
			{
				throw new InvalidOperationException(
			"Call this constructor from UI thread please, because it creates ImageSource object for UI");
			}

			_dispatcher = dispatcher;
			_tokenSource = tokenSource;
			_setup = setup;
			_state = State.Idle;
			SetPipeline(setup);
			Task.Factory.StartNew(() => BackgroundLoop(updateColor), tokenSource.Token);
		}

		public void StopStreaming()
		{
			_tokenSource.Cancel();
		}

		public void StartRecording()
		{
			var path = $"{TEMP_FILE_PATH}{VIDEO_FILE_FORMAT}";
			_recorder = RecorderFactory.Create();
			_recorder.ConfigureCaptureToFile(path, _setup);
			_state = State.Recording;
		}

		public void StopRecording()
		{
			_state = State.Idle;
			_recorder.StopCaptureToFile();
			_recorder.Dispose();
		}

		public void Save(string name)
		{
			CreateDirectory(SAVED_FILE_PATH);
			CleanOldFile($"{SAVED_FILE_PATH}\\{VIDEO_FILE_NAME_ROOT}{name}{VIDEO_FILE_FORMAT}");
			CopyVideo(name);
		}

		public void Dispose()
		{
			_recorder.Dispose();
			_pipeline.Dispose();
		}

		private void CleanOldFile(string path)
		{
			if (File.Exists(path))
			{
				File.Delete(path);
			}
		}

		private void CopyVideo(string name)
		{
			var videoFileSource = $"{TEMP_FILE_PATH}{VIDEO_FILE_FORMAT}";
			var videoFileDest = $"{SAVED_FILE_PATH}\\{VIDEO_FILE_NAME_ROOT}{name}{VIDEO_FILE_FORMAT}";
			File.Copy(videoFileSource, videoFileDest);
		}

		private void CreateDirectory(string path)
		{
			try
			{
				if (!Directory.Exists(path))
				{
					Directory.CreateDirectory(path);
				}
			}
			catch (IOException ex)
			{
				Debug.Write("Save session IOException, can not delete the session because it is open " + ex.Message);
			}
			catch (UnauthorizedAccessException exc)
			{
				Debug.Write("Save session UnauthorizedAccessException can not delete the session because it is open " + exc.Message);
			}
		}

		private void SetPipeline(CameraSetup setup)
		{
			var cfg = CreateConfig(setup);
			_pipeline = new Pipeline();
			_pipeline.Start(cfg);
		}

		private Config CreateConfig(CameraSetup setup)
		{
			var width = setup.RGBWidth;
			var height = setup.RGBHeight;
			var fps = setup.RGBFPS;
			var cfg = new Config();
			cfg.EnableStream(Intel.RealSense.Stream.Color, width, height, Format.Bgr8, fps);
			return cfg;
		}

		private void BackgroundLoop(Action<VideoFrame> updateColor)
		{
			try
			{
				while (!_tokenSource.Token.IsCancellationRequested)
				{
					using (var frames = _pipeline.WaitForFrames())
					{
						var colorFrame = frames.ColorFrame.DisposeWith(frames);
						if (_state.Equals(State.Recording))
						{
							_dispatcher.InvokeAsync(() => _recorder.RecordBitmap(colorFrame.Data));
						}
						ExecuteInUi(updateColor, colorFrame);
					}
				}
			}
			catch (Exception ex)
			{
				Debug.WriteLine(ex.Message);
			}
		}

		private void ExecuteInUi(Action<VideoFrame> action, VideoFrame frames)
		{
			try
			{
				if (_dispatcher == null)
				{
					return;
				}
				if (Thread.CurrentThread != _dispatcher.Thread)
				{
					_dispatcher.Invoke(DispatcherPriority.Render, action, frames);
					return;
				}
				action.Invoke(frames);
			}
			catch (System.Exception ex)
			{
				Debug.Write("Camera Action Error " + ex.Message);
			}
		}
	}
}
