using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace Ming.SourceGenerators;

[Generator]
public sealed class ScriptPathGenerator : ISourceGenerator
{
    private static readonly DiagnosticDescriptor InvalidScript = new DiagnosticDescriptor(
        "MING001", "Invalid script declaration", "Script '{0}': {1}", "Ming", DiagnosticSeverity.Error, true);

    public void Initialize(GeneratorInitializationContext context)
    {
    }

    public void Execute(GeneratorExecutionContext context)
    {
        if (!context.AnalyzerConfigOptions.GlobalOptions.TryGetValue("build_property.MingProjectDirectory", out var root))
        {
            context.ReportDiagnostic(Diagnostic.Create(InvalidScript, Location.None, "project", "MingProjectDirectory is missing"));
            return;
        }

        root = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) + Path.DirectorySeparatorChar;
        var visited = new HashSet<ISymbol>(SymbolEqualityComparer.Default);
        foreach (var tree in context.Compilation.SyntaxTrees)
        {
            var model = context.Compilation.GetSemanticModel(tree);
            foreach (var declaration in tree.GetRoot().DescendantNodes().OfType<ClassDeclarationSyntax>())
            {
                var symbol = model.GetDeclaredSymbol(declaration);
                if (symbol == null || !IsNode(symbol) || !visited.Add(symbol))
                {
                    continue;
                }

                var parts = symbol.DeclaringSyntaxReferences.Select(r => (ClassDeclarationSyntax)r.GetSyntax()).ToArray();
                var matching = parts.FirstOrDefault(p => Path.GetFileNameWithoutExtension(p.SyntaxTree.FilePath) == symbol.Name);
                string? error = null;
                if (symbol.ContainingType != null || symbol.IsGenericType)
                {
                    error = "use a non-generic top-level class";
                }
                else if (parts.Any(p => !p.Modifiers.Any(SyntaxKind.PartialKeyword)))
                {
                    error = "all declarations must be partial";
                }
                else if (matching == null)
                {
                    error = "the class name must match its .cs filename";
                }

                var fullPath = Path.GetFullPath((matching ?? declaration).SyntaxTree.FilePath);
                if (!fullPath.StartsWith(root, StringComparison.OrdinalIgnoreCase))
                {
                    error = "the script must be inside the project directory";
                }

                if (symbol.GetAttributes().Any(a => a.AttributeClass?.ToDisplayString() == "Ming.ScriptPathAttribute"))
                {
                    error = "remove the manual ScriptPath attribute; Ming generates it during compilation";
                }

                if (error != null)
                {
                    context.ReportDiagnostic(Diagnostic.Create(InvalidScript, declaration.Identifier.GetLocation(), symbol.Name, error));
                    continue;
                }

                var path = "res://" + fullPath.Substring(root.Length).Replace('\\', '/');
                var ns = symbol.ContainingNamespace;
                var source = new StringBuilder();
                if (!ns.IsGlobalNamespace)
                {
                    source.Append("namespace ").Append(ns.ToDisplayString()).AppendLine(" {");
                }

                source.Append("[global::Ming.ScriptPathAttribute(").Append(SymbolDisplay.FormatLiteral(path, true)).AppendLine(")]");
                source.Append("partial class @").Append(symbol.Name).AppendLine(" {}");
                if (!ns.IsGlobalNamespace)
                {
                    source.AppendLine("}");
                }

                context.AddSource(BitConverter.ToString(Encoding.UTF8.GetBytes(symbol.ToDisplayString())).Replace("-", "") + ".ScriptPath.g.cs",
                    SourceText.From(source.ToString(), Encoding.UTF8));
            }
        }
    }

    private static bool IsNode(INamedTypeSymbol symbol)
    {
        for (var current = symbol.BaseType; current != null; current = current.BaseType)
        {
            if (current.ToDisplayString() == "Ming.Node")
            {
                return true;
            }
        }

        return false;
    }
}
