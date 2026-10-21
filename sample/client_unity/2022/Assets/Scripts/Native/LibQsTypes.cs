using System;
using System.Runtime.InteropServices;

namespace QS.Native
{
    public enum QsWebSocketState : int
    {
        Connecting = 0,
        Handshaking = 1,
        Open = 2,
        Closing = 3,
        Closed = 4,
        Error = 5,
    }

    public enum QsWebSocketError : int
    {
        None = 0,
        HandshakeSend = 1,
        HandshakeResponse = 2,
        HandshakeAccept = 3,
        ReceiveBuffer = 4,
        Frame = 5,
        Send = 6,
    }

    public static class LibQsMarshal
    {
        public static string PtrToString(IntPtr ptr)
        {
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(ptr);
        }

        public static byte[] CopyBytes(IntPtr src, int length)
        {
            if (src == IntPtr.Zero || length <= 0)
            {
                return Array.Empty<byte>();
            }

            var bytes = new byte[length];
            Marshal.Copy(src, bytes, 0, length);
            return bytes;
        }
    }
}
