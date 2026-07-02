using CS2RoundAlert.Tray;
using System.Windows.Forms;

namespace CS2RoundAlert;

internal static class Program
{
    [STAThread]
    private static void Main()
    {
        ApplicationConfiguration.Initialize();
        Application.Run(new TrayApplicationContext());
    }
}
