using System.Media;
using CS2RoundAlert.Settings;

namespace CS2RoundAlert.Alerting;

internal sealed class AlertService
{
    public void Play(AppSettings settings)
    {
        if (!settings.UseSystemSound && IsUsableWave(settings.CustomWavPath))
        {
            _ = Task.Run(() => PlayWave(settings.CustomWavPath!));
            return;
        }

        SystemSounds.Exclamation.Play();
    }

    private static bool IsUsableWave(string? path)
    {
        return !string.IsNullOrWhiteSpace(path)
            && Path.GetExtension(path).Equals(".wav", StringComparison.OrdinalIgnoreCase)
            && File.Exists(path);
    }

    private static void PlayWave(string path)
    {
        try
        {
            using var player = new SoundPlayer(path);
            player.PlaySync();
        }
        catch
        {
            SystemSounds.Exclamation.Play();
        }
    }
}
