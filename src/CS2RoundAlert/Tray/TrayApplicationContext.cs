using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;
using CS2RoundAlert.Alerting;
using CS2RoundAlert.GameState;
using CS2RoundAlert.Integration;
using CS2RoundAlert.Settings;
using CS2RoundAlert.Windows;

namespace CS2RoundAlert.Tray;

internal sealed class TrayApplicationContext : ApplicationContext
{
    private const string RepositoryUrl = "https://github.com/BannerLord54/CS2RoundAlert";

    private readonly AlertService _alertService = new();
    private readonly ForegroundWindowService _foregroundWindowService = new();
    private readonly GsiConfigInstaller _configInstaller = new();
    private readonly SettingsService _settingsService = new();
    private readonly AppSettings _settings;
    private readonly NotifyIcon _notifyIcon;
    private readonly GsiListener _listener;
    private readonly ConfigInstallResult _configInstallResult;

    public TrayApplicationContext()
    {
        _settings = _settingsService.Load();
        _configInstallResult = _configInstaller.EnsureInstalled(_settings);
        _settingsService.Save(_settings);

        _notifyIcon = CreateNotifyIcon();
        _listener = new GsiListener(_settings.Port);
        _listener.FreezetimeEntered += HandleFreezetimeEntered;

        StartListener();
        ShowFirstRunNotification();
    }

    private NotifyIcon CreateNotifyIcon()
    {
        var enabledMenuItem = new ToolStripMenuItem("Enable alerts")
        {
            Checked = _settings.Enabled,
            CheckOnClick = true
        };

        enabledMenuItem.CheckedChanged += (_, _) =>
        {
            _settings.Enabled = enabledMenuItem.Checked;
            _settingsService.Save(_settings);
        };

        var menu = new ContextMenuStrip();
        menu.Items.Add(enabledMenuItem);
        menu.Items.Add(new ToolStripSeparator());
        menu.Items.Add("Open settings folder", null, (_, _) => OpenSettingsFolder());
        menu.Items.Add("Open GitHub repo", null, (_, _) => OpenUrl(RepositoryUrl));
        menu.Items.Add(new ToolStripSeparator());
        menu.Items.Add("Quit", null, (_, _) => Quit());

        return new NotifyIcon
        {
            Text = "CS2RoundAlert",
            Icon = SystemIcons.Information,
            Visible = true,
            ContextMenuStrip = menu
        };
    }

    private void StartListener()
    {
        try
        {
            _listener.Start();
        }
        catch (Exception ex)
        {
            _notifyIcon.ShowBalloonTip(
                7000,
                "CS2RoundAlert",
                $"Could not listen on 127.0.0.1:{_settings.Port}. {ex.Message}",
                ToolTipIcon.Error);
        }
    }

    private void HandleFreezetimeEntered(object? sender, EventArgs args)
    {
        if (!_settings.Enabled)
        {
            return;
        }

        if (_settings.AlertOnlyWhenNotFocused && _foregroundWindowService.IsCs2Foreground())
        {
            return;
        }

        _alertService.Play(_settings);
    }

    private void ShowFirstRunNotification()
    {
        if (_settings.FirstRunNotificationShown)
        {
            return;
        }

        var timer = new System.Windows.Forms.Timer
        {
            Interval = 1000
        };

        timer.Tick += (_, _) =>
        {
            timer.Stop();
            timer.Dispose();

            _notifyIcon.ShowBalloonTip(
                9000,
                "CS2RoundAlert",
                _configInstallResult.Message,
                _configInstallResult.Installed ? ToolTipIcon.Info : ToolTipIcon.Warning);

            _settings.FirstRunNotificationShown = true;
            _settingsService.Save(_settings);
        };

        timer.Start();
    }

    private static void OpenSettingsFolder()
    {
        Directory.CreateDirectory(SettingsService.AppDataDirectory);
        OpenUrl(SettingsService.AppDataDirectory);
    }

    private static void OpenUrl(string target)
    {
        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = target,
                UseShellExecute = true
            });
        }
        catch
        {
        }
    }

    private void Quit()
    {
        _notifyIcon.Visible = false;
        _listener.Dispose();
        _notifyIcon.Dispose();
        ExitThread();
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            _listener.FreezetimeEntered -= HandleFreezetimeEntered;
            _listener.Dispose();
            _notifyIcon.Dispose();
        }

        base.Dispose(disposing);
    }
}
