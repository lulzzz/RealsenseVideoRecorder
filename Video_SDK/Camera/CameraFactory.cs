using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Video_SDK.Camera
{
	public static class CameraFactory
	{
		public static ICamera Create()
		{
			return new Camera();
		}
	}
}
