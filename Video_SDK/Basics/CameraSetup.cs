using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Video_SDK.Basics
{
	public class CameraSetup
	{
		public int RGBFPS { get; private set; }
		public int RGBWidth { get; private set; }
		public int RGBHeight { get; private set; }
		public int DepthFPS { get; private set; }
		public int DepthWidth { get; private set; }
		public int DepthHeight { get; private set; }
		public float Ratio => (float)RGBWidth / RGBHeight;

		public CameraSetup(int fps, int width)
		{
			Set(fps, width);
		}

		public CameraSetup(int fps, int width, int height)
		{
			RGBWidth = width;
			RGBHeight = height;
			DepthWidth = width;
			DepthHeight = height;
			RGBFPS = fps;
			DepthFPS = fps;
		}

		// Available Resolutions
		//
		// 424 X 240 | 640 X 480 | 848 X 480 | 960 X 540 | 1280 X 720 | 1920 X 1080

		private void Set(int fps, int width)
		{
			RGBWidth = width;
			DepthWidth = width;
			RGBFPS = fps;
			DepthFPS = fps;

			switch (width)
			{
				case 424:
					RGBHeight = 240;
					DepthHeight = 240;
					break;
				case 640:
					RGBHeight = 480;
					DepthHeight = 480;
					break;
				case 848:
					RGBHeight = 480;
					DepthHeight = 480;
					break;
				case 960:
					RGBHeight = 540;
					DepthWidth = 848;
					DepthHeight = 480;
					break;
				case 1280:
					RGBHeight = 720;
					RGBFPS = 30;
					DepthHeight = 720;
					DepthFPS = 30;
					break;
				case 1920:
					RGBHeight = 1080;
					RGBFPS = 30;
					DepthHeight = 1080;
					DepthFPS = 30;
					break;
				default:
					RGBWidth = 640;
					RGBHeight = 480;
					RGBFPS = 30;
					DepthWidth = 640;
					DepthHeight = 480;
					DepthFPS = 30;
					Console.Write("Using Defauls Resolution 640 X 480 @30FPS");
					break;
			}
		}
	}
}
