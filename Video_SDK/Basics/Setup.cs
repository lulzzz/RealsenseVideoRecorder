using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Video_SDK.Basics
{
	public class Setup
	{
		public WindowParams WindowParameters { get; set; }
		public CameraSetup CameraParameters { get; set; }

		public Setup(int FPS, int xCoordinate, int yCoordinate, int windowHeight, int horizontalResolution)
		{
			CameraParameters = new CameraSetup(FPS, horizontalResolution);
			WindowParameters = new WindowParams(xCoordinate, yCoordinate, windowHeight, CameraParameters.Ratio);
		}
	}
}
