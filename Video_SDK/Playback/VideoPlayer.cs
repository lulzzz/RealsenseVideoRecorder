using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace Video_SDK.Playback
{
    internal class VideoPlayer : Control, IVideoPlayer
    {
        private readonly Exception _notConnected = new InvalidOperationException("Player is not connected.");
        private IntPtr _playerPointer;
        private Control _surface;
        private readonly object _lock = new object();
        private DateTime _createFileTime;
        private const long SECONDS_TO_VIDEO_TIME_MULTIPLIER = 10000000;
		private const long MILSECONDS_TO_VIDEO_TIME_MULTIPLIER = 10000;
        private double _seekTime;
        private bool _surfaceNeedsRepaint;

        public void Open(IVideoSurface playbackSurface, string filePath, DateTime correlationStartTime)
        {
            filePath = filePath.Replace("\"", "");
            lock (_lock)
            {
                CreatePlayer(playbackSurface);
                OpenFile(filePath, correlationStartTime);
            }
        }

        public void Play()
        {
            lock (_lock)
            {
                if (!IsConnected())
                {
                    throw _notConnected;
                }

                CppAssembly.PlayerPlay(_playerPointer, out var invokeResult);
                if (invokeResult != 0)
                {
                    throw new InvalidOperationException($"Failed to play #{invokeResult}.");
                }
            }
        }

        public void Pause()
        {
            lock (_lock)
            {
                if (!IsConnected())
                {
                    throw _notConnected;
                }

                CppAssembly.PlayerPause(_playerPointer);
            }
        }

        public void Seek(DateTime absoluteTime)
        {
            var diff = absoluteTime - _createFileTime;
            var milSeconds = diff.TotalMilliseconds;
            Seek(milSeconds);
        }

        public void Seek(double seekTimeMilSec)
        {
			var prevSeekTime = MilSecondsToSeekNanoSeconds(_seekTime);
            _seekTime = seekTimeMilSec;
            var seek = MilSecondsToSeekNanoSeconds(_seekTime);

            lock (_lock)
            {
                if (!IsConnected())
                {
                    throw _notConnected;
                }
				if (prevSeekTime > seek)
				{
					Debug.WriteLine($"seek time problem: {seek - prevSeekTime}");
				}
                CppAssembly.PlayerSkipToPosition(_playerPointer, seek);
            }
        }

        public void Close()
        {
            lock (_lock)
            {
                if (!IsConnected())
                {
                    return;
                }

                _surface.Paint -= SurfacePaint;
                _surface.SizeChanged -= SurfaceSizeChanged;
                CppAssembly.PlayerShutDown(_playerPointer);
                DisposePointerToPlayer();
            }
        }

        private void OpenFile(string filePath, DateTime correlationStartTime)
        {
            filePath = filePath.Replace("\"", "");
            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException(filePath);
            }

            SetInitialTime(filePath, correlationStartTime);

            var seek = MilSecondsToSeekNanoSeconds(_seekTime);
            CppAssembly.PlayerOpen(_playerPointer, filePath, seek, out var invokeResult);
            if (invokeResult != 0)
            {
                throw new InvalidOperationException($"Failed to open video #{invokeResult}.");
            }
        }

        private void SetInitialTime(string filePath, DateTime correlationStartTime)
        {
            _createFileTime = new FileInfo(filePath).CreationTime;
            _seekTime = (correlationStartTime - _createFileTime).TotalMilliseconds;
        }

        private long SecondsToSeekNanoSeconds(double seconds)
        {
            if (seconds < 0)
            {
                seconds = 0;
            }
            
            return Convert.ToInt64(seconds * SECONDS_TO_VIDEO_TIME_MULTIPLIER);
        }

		private long MilSecondsToSeekNanoSeconds(double milSeconds)
		{
			if (milSeconds < 0)
			{
				milSeconds = 0;
			}
			return Convert.ToInt64(milSeconds * MILSECONDS_TO_VIDEO_TIME_MULTIPLIER);
		}

		private void CreatePlayer(IVideoSurface playbackSurface)
        {
            var surface = playbackSurface.GetSurfaceControl();
            if (IsConnected())
            {
                if (_surface == surface)
                {
                    return;
                }
                Close();
            }

            if (_surface != null)
            {
                _surface.Paint -= SurfacePaint;
                _surface.SizeChanged -= SurfaceSizeChanged;
            }

            try
            {
                _playerPointer = Marshal.AllocHGlobal(IntPtr.Size);
                CppAssembly.CreatePlayer(out _playerPointer, surface.Handle, Handle, out var invokeResult);
                if (invokeResult != 0)
                {
                    throw new InvalidOperationException($"Failed to connect to player #{invokeResult}.");
                }
                _surface = surface;
                _surface.Paint += SurfacePaint;
                _surface.SizeChanged += SurfaceSizeChanged;
            }
            catch (Exception)
            {
                DisposePointerToPlayer();
                throw;
            }
        }
        
        private void SurfaceSizeChanged(object sender, EventArgs e)
        {
            if (!IsConnected())
            {
                return;
            }

            lock (_lock)
            {
                _surfaceNeedsRepaint = true;
                CppAssembly.PlayerResize(_playerPointer, Convert.ToUInt16(_surface.Width), Convert.ToUInt16(_surface.Height));
            }
        }

        private void SurfacePaint(object sender, PaintEventArgs e)
        {
            if (!IsConnected() || !_surfaceNeedsRepaint)
            {
                return;
            }

            lock (_lock)
            {
                Seek(_seekTime);
                _surfaceNeedsRepaint = false;
            }
        }

        private void HandleEvent(IntPtr wParam)
        {
            lock (_lock)
            {
                if (!IsConnected())
                {
                    return;
                }

                CppAssembly.PlayerHandleEvent(_playerPointer, wParam);
            }
        }

        protected override void WndProc(ref Message m)
        {
            const int WM_APP_PLAYER_EVENT = 0x8000 + 1;
			switch (m.Msg)
            {
                case WM_APP_PLAYER_EVENT:
                    var wParam = m.WParam;
                    HandleEvent(wParam);
					break;
				default:
                    base.WndProc(ref m);
                    break;
            }
        }

        protected override void Dispose(bool disposing)
        {
            Close();
            base.Dispose(disposing);
        }

        private void DisposePointerToPlayer()
        {
            if (_playerPointer == IntPtr.Zero)
            {
                return;
            }
            Marshal.FreeHGlobal(_playerPointer);
            _playerPointer = IntPtr.Zero;
        }

        private bool IsConnected()
        {
            return _playerPointer != IntPtr.Zero;
        }
    }
}