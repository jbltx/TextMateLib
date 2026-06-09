// C# (.NET P/Invoke) benchmark for TextMateLib.
//
// Measures the managed .NET binding (TextMateLib.Bindings, P/Invoke over the native
// shared library) tokenizing the same fixtures + tm-grammars as bench-js.mjs and
// bench-native.cpp. Like the JS binding, the scope path includes per-token scope-object
// marshalling across the native boundary; the themed path returns a binary token count.
//
// Usage: BenchCs [--theme <theme.json>] <scope> <grammar.json> <fixture> [...]
using System.Diagnostics;
using System.Text;
using TextMateLib.Bindings;

const int Warmup = 3;
const int Runs = 7;

static (long ns, long tokens) TokenizePass(Grammar grammar, string[] lines)
{
    var sw = Stopwatch.StartNew();
    IntPtr state = IntPtr.Zero;
    long tokenCount = 0;
    foreach (var line in lines)
    {
        var result = grammar.TokenizeLine(line, state);
        state = result.StateStack;
        tokenCount += result.Tokens.Count;
    }
    sw.Stop();
    double ns = sw.ElapsedTicks * (1_000_000_000.0 / Stopwatch.Frequency);
    return ((long)ns, tokenCount);
}

static (long ns, long tokens) TokenizePass2(Grammar grammar, string[] lines)
{
    var sw = Stopwatch.StartNew();
    IntPtr state = IntPtr.Zero;
    long tokenCount = 0;
    foreach (var line in lines)
    {
        var result = grammar.TokenizeLine2(line, state);
        state = result.StateStack;
        tokenCount += result.TokenCount;
    }
    sw.Stop();
    double ns = sw.ElapsedTicks * (1_000_000_000.0 / Stopwatch.Frequency);
    return ((long)ns, tokenCount);
}

static long Median(List<long> xs)
{
    xs.Sort();
    return xs[xs.Count / 2];
}

string? themeFile = null;
int argStart = 0;
if (args.Length >= 2 && args[0] == "--theme")
{
    themeFile = args[1];
    argStart = 2;
}

var remaining = args.Length - argStart;
if (remaining < 3 || remaining % 3 != 0)
{
    Console.Error.WriteLine("Usage: BenchCs [--theme <theme.json>] <scope> <grammar.json> <fixture> [...]");
    return 1;
}

var scopeRows = new List<(string name, double mb, double mbps, long tokens)>();
var themedRows = new List<(string name, double mb, double mbps, long tokens)>();

for (int i = argStart; i < args.Length; i += 3)
{
    string scope = args[i];
    string grammarPath = args[i + 1];
    string fixturePath = args[i + 2];
    try
    {
        string content = File.ReadAllText(fixturePath);
        string[] lines = content.Split('\n');
        double mb = new FileInfo(fixturePath).Length / 1024.0 / 1024.0;

        using var registry = new Registry();
        registry.AddGrammarFromFile(grammarPath);
        if (themeFile != null)
            registry.SetThemeFromJson(File.ReadAllText(themeFile));
        var grammar = registry.LoadGrammar(scope);

        // Scope pass
        {
            for (int w = 0; w < Warmup; w++) TokenizePass(grammar, lines);
            var samples = new List<long>();
            long tokens = 0;
            for (int r = 0; r < Runs; r++)
            {
                var (ns, tk) = TokenizePass(grammar, lines);
                samples.Add(ns);
                tokens = tk;
            }
            long medianNs = Median(samples);
            double mbps = mb / (medianNs / 1e9);
            scopeRows.Add((Path.GetFileName(fixturePath), mb, mbps, tokens));
            Console.Error.WriteLine($"  {scope} scope ok ({mbps:F1} MB/s, {tokens} tokens)");
        }

        // Themed pass
        if (themeFile != null)
        {
            for (int w = 0; w < Warmup; w++) TokenizePass2(grammar, lines);
            var samples = new List<long>();
            long tokens = 0;
            for (int r = 0; r < Runs; r++)
            {
                var (ns, tk) = TokenizePass2(grammar, lines);
                samples.Add(ns);
                tokens = tk;
            }
            long medianNs = Median(samples);
            double mbps = mb / (medianNs / 1e9);
            themedRows.Add((Path.GetFileName(fixturePath), mb, mbps, tokens));
            Console.Error.WriteLine($"  {scope} themed ok ({mbps:F1} MB/s, {tokens} tokens)");
        }
    }
    catch (Exception e)
    {
        Console.Error.WriteLine($"  ERROR {scope}: {e.Message}");
    }
}

Console.WriteLine("| File | Size | tml-cs scope (P/Invoke) |");
Console.WriteLine("|------|------|-------------------------|");
foreach (var r in scopeRows)
    Console.WriteLine($"| {r.name} | {r.mb:F2} MB | **{r.mbps:F1} MB/s** |");

if (themedRows.Count > 0)
{
    Console.WriteLine();
    Console.WriteLine("| File | Size | tml-cs themed (P/Invoke) |");
    Console.WriteLine("|------|------|--------------------------|");
    foreach (var r in themedRows)
        Console.WriteLine($"| {r.name} | {r.mb:F2} MB | **{r.mbps:F1} MB/s** |");
}
return 0;
