using System;
using System.Diagnostics;
using System.Threading;
using Video_SDK;

namespace TestConsole
{
	class Program
	{
		[STAThread]
		static void Main(string[] args)
		{
			var testApp = new RunTest();			
		}
	}

	class RunTest
	{
		//private Sensor3D _sensor;
		private const int _horzResolution = 848;
		private const string name = "0022_2386";

		public RunTest()
		{
			using (var sensor = new Sensor3D())
			{
#if DEBUG
				Console.WriteLine("MFCpp version: " + sensor.GetMFCppVersion());
#endif
				record(sensor, name, 9000, 1);
				record(sensor, name, 9000, 1);
			}			
		}

		private void record(Sensor3D sensor, string name, int durationMilSec, int cycles)
		{
			sensor.Open(100, 100, 450, _horzResolution);
			Debug.WriteLine("----- camera opened State = Idle ");

			Thread.Sleep(2000);

			for (int i = 0; i < cycles; i++)
			{
				sensor.StartRecording();
				Debug.WriteLine("----- start recording");

				Thread.Sleep(durationMilSec);

				sensor.StopRecording();
				Debug.WriteLine("----- stop recording");

				sensor.SaveLast(name);
				Debug.WriteLine("----- save last");

				Thread.Sleep(1500);
			}
			sensor.Close();
			Debug.WriteLine("----- recording closed ");

			Thread.Sleep(1500);

		}
	}
}
