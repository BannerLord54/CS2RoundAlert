using System.Diagnostics;
using System.Runtime.InteropServices;

namespace CS2RoundAlert.Windows;

internal sealed class ForegroundWindowService
{
    public bool IsCs2Foreground()
    {
        var handle = GetForegroundWindow();
        if (handle == IntPtr.Zero)
        {
            return false;
        }

        _ = GetWindowThreadProcessId(handle, out var processId);
        if (processId == 0)
        {
            return false;
        }

        try
        {
            using var process = Process.GetProcessById((int)processId);
            return process.ProcessName.Equals("cs2", StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
    }

    [DllImport("user32.dll")]
    private static extern IntPtr GetForegroundWindow();

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
}
