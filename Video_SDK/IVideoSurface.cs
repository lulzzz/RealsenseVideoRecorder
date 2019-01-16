using System;

namespace Video_SDK
{
    public interface IVideoSurface
    {
        System.Windows.Forms.Control GetSurfaceControl();

        IntPtr GetHandle();
    }
}