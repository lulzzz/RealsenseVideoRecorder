using Intel.RealSense;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace Video_SDK
{
	public partial class CameraView : Window
	{
		public CameraView()
		{
			InitializeComponent();
		}

		public CameraView(CameraViewModel viewModel)
			: this()
		{
			DataContext = viewModel;
		}

		public CameraViewModel ViewModel => DataContext as CameraViewModel;

		private void control_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{			
			ViewModel.TokenSource.Cancel();
		}

		private void mainWindow_Loaded(object sender, RoutedEventArgs e)
		{
			ViewModel.StartCapture();
		}
	}
}
