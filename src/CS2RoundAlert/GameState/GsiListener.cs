using System.Net;
using System.Text;
using System.Text.Json;

namespace CS2RoundAlert.GameState;

internal sealed class GsiListener : IDisposable
{
    private readonly HttpListener _listener = new();
    private CancellationTokenSource? _cancellation;
    private Task? _listenTask;
    private string? _lastPhase;

    public GsiListener(int port)
    {
        Port = port;
    }

    public event EventHandler? FreezetimeEntered;

    public int Port { get; }

    public void Start()
    {
        if (_listener.IsListening)
        {
            return;
        }

        _listener.Prefixes.Clear();
        _listener.Prefixes.Add($"http://127.0.0.1:{Port}/");
        _listener.Start();

        _cancellation = new CancellationTokenSource();
        _listenTask = Task.Run(() => ListenAsync(_cancellation.Token));
    }

    public void Stop()
    {
        try
        {
            _cancellation?.Cancel();
            _listener.Stop();
            _listener.Close();
        }
        catch
        {
        }
    }

    private async Task ListenAsync(CancellationToken cancellationToken)
    {
        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                var context = await _listener.GetContextAsync().WaitAsync(cancellationToken);
                _ = Task.Run(() => HandleRequestAsync(context, cancellationToken), CancellationToken.None);
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (HttpListenerException) when (!_listener.IsListening)
            {
                break;
            }
            catch
            {
            }
        }
    }

    private async Task HandleRequestAsync(HttpListenerContext context, CancellationToken cancellationToken)
    {
        try
        {
            if (context.Request.HasEntityBody)
            {
                using var reader = new StreamReader(context.Request.InputStream, context.Request.ContentEncoding);
                var body = await reader.ReadToEndAsync(cancellationToken);
                ProcessPayload(body);
            }
        }
        catch
        {
        }
        finally
        {
            await RespondOkAsync(context, cancellationToken);
        }
    }

    private static async Task RespondOkAsync(HttpListenerContext context, CancellationToken cancellationToken)
    {
        try
        {
            var bytes = Encoding.UTF8.GetBytes("OK");
            context.Response.StatusCode = (int)HttpStatusCode.OK;
            context.Response.ContentType = "text/plain";
            context.Response.ContentLength64 = bytes.Length;
            await context.Response.OutputStream.WriteAsync(bytes, cancellationToken);
        }
        catch
        {
        }
        finally
        {
            try
            {
                context.Response.Close();
            }
            catch
            {
            }
        }
    }

    private void ProcessPayload(string body)
    {
        if (string.IsNullOrWhiteSpace(body))
        {
            return;
        }

        try
        {
            using var document = JsonDocument.Parse(body);
            if (!document.RootElement.TryGetProperty("round", out var round))
            {
                return;
            }

            if (!round.TryGetProperty("phase", out var phaseProperty))
            {
                return;
            }

            var phase = phaseProperty.GetString();
            if (string.IsNullOrWhiteSpace(phase))
            {
                return;
            }

            var enteredFreezetime =
                phase.Equals("freezetime", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(_lastPhase, "freezetime", StringComparison.OrdinalIgnoreCase);

            _lastPhase = phase;

            if (enteredFreezetime)
            {
                FreezetimeEntered?.Invoke(this, EventArgs.Empty);
            }
        }
        catch (JsonException)
        {
        }
    }

    public void Dispose()
    {
        Stop();
        _cancellation?.Dispose();
    }
}
