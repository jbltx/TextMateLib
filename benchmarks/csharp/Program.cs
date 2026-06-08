// C# (.NET P/Invoke) scope-tokenization benchmark for TextMateLib.
//
// Measures the managed .NET binding (TextMateLib.Bindings, P/Invoke over the native
// shared library) tokenizing the same fixtures + tm-grammars as bench-js.mjs and
// bench-native.cpp. Like the JS binding, the cost includes marshalling per-token scope
// objects across the native boundary. Scope tokenization only (the binding exposes no
// themed tokenizeLine2 path).
//
// Usage: BenchCs <scope> <grammar.json> <fixture> [<scope> <grammar.json> <fixture> ...]
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

static long Median(List<long> xs)
{
    xs.Sort();
    return xs[xs.Count / 2];
}

if (args.Length < 3 || args.Length % 3 != 0)
{
    Console.Error.WriteLine("Usage: BenchCs <scope> <grammar.json> <fixture> [...]");
    return 1;
}

var rows = new List<(string name, double mb, double mbps, long tokens)>();

for (int i = 0; i < args.Length; i += 3)
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
        var grammar = registry.LoadGrammar(scope);

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
        rows.Add((Path.GetFileName(fixturePath), mb, mbps, tokens));
        Console.Error.WriteLine($"  {scope} ok ({mbps:F1} MB/s, {tokens} tokens)");
    }
    catch (Exception e)
    {
        Console.Error.WriteLine($"  ERROR {scope}: {e.Message}");
    }
}

Console.WriteLine("| File | Size | tml-cs (P/Invoke) |");
Console.WriteLine("|------|------|-------------------|");
foreach (var r in rows)
    Console.WriteLine($"| {r.name} | {r.mb:F2} MB | **{r.mbps:F1} MB/s** |");
return 0;
