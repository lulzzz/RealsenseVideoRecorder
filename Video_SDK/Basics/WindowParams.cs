using System.Drawing;

namespace Video_SDK.Basics
{
	public class WindowParams
	{
		public Point Position { get; set; }
		public Size Size { get; set; }

		public WindowParams(int x, int y, int height, float ratio)
		{
			Position = new Point(x, y);
			var width = (int)(height * ratio);
			Size = new Size(width, height);
		}
	}
}
