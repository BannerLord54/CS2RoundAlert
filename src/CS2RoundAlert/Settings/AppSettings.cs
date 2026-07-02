namespace CS2RoundAlert.Settings;

internal sealed class AppSettings
{
    public int Port { get; set; } = 3000;
    public bool Enabled { get; set; } = true;
    public bool AlertOnlyWhenNotFocused { get; set; } = true;
    public bool UseSystemSound { get; set; } = true;
    public string? CustomWavPath { get; set; }
    public string? Cs2CfgFolderPath { get; set; }
    public bool FirstRunNotificationShown { get; set; }
}
