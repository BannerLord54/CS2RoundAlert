namespace CS2RoundAlert.Integration;

internal sealed record ConfigInstallResult(bool Installed, string? ConfigPath, string Message);
