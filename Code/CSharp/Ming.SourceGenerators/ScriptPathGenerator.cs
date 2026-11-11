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

    private static readonly DiagnosticDescriptor UnsupportedMethod = new DiagnosticDescriptor(
        "MING002", "Method unavailable to native calls", "Method '{0}' is unavailable to native calls: {1}",
        "Ming", DiagnosticSeverity.Warning, true);

    private static readonly DiagnosticDescriptor AmbiguousMethod = new DiagnosticDescriptor(
        "MING003", "Ambiguous native method dispatch", "Method '{0}' has multiple instance overloads with {1} parameters; this group is unavailable to native calls",
        "Ming", DiagnosticSeverity.Warning, true);

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

                if (!TryGetScriptPath(context, symbol, declaration, root, out var path))
                {
                    continue;
                }

                GenerateScriptPath(context, symbol, path);
                GenerateMethodDispatch(context, symbol);
            }
        }
    }

    #region Script paths

    private static bool TryGetScriptPath(GeneratorExecutionContext context, INamedTypeSymbol symbol,
        ClassDeclarationSyntax declaration, string root, out string path)
    {
        path = string.Empty;
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
            return false;
        }

        path = "res://" + fullPath.Substring(root.Length).Replace('\\', '/');
        return true;
    }

    private static void GenerateScriptPath(GeneratorExecutionContext context, INamedTypeSymbol symbol, string path)
    {
        var source = new StringBuilder();
        if (!symbol.ContainingNamespace.IsGlobalNamespace)
        {
            source.Append("namespace ").AppendLine(symbol.ContainingNamespace.ToDisplayString());
            source.AppendLine("{");
        }

        source.Append("[global::Ming.ScriptPathAttribute(").Append(SymbolDisplay.FormatLiteral(path, true)).AppendLine(")]");
        source.Append("partial class @").AppendLine(symbol.Name);
        source.AppendLine("{");
        source.AppendLine("}");
        if (!symbol.ContainingNamespace.IsGlobalNamespace)
        {
            source.AppendLine("}");
        }

        AddSource(context, symbol, ".ScriptPath.g.cs", source);
    }

    #endregion

    #region Method dispatch

    private static void GenerateMethodDispatch(GeneratorExecutionContext context, INamedTypeSymbol symbol)
    {
        var candidates = symbol.GetMembers().OfType<IMethodSymbol>()
            .Where(m => m.MethodKind == MethodKind.Ordinary && !m.IsStatic
                && !m.IsImplicitlyDeclared
                && (!m.IsPartialDefinition || m.PartialImplementationPart != null))
            .ToArray();
        var methods = new List<IMethodSymbol>();
        foreach (var group in candidates.GroupBy(m => (m.Name, m.Parameters.Length)))
        {
            if (group.Count() > 1)
            {
                foreach (var method in group)
                {
                    context.ReportDiagnostic(Diagnostic.Create(AmbiguousMethod, method.Locations.FirstOrDefault(),
                        method.Name, method.Parameters.Length));
                }

                continue;
            }

            var candidate = group.First();
            string? reason = GetUnsupportedMethodReason(candidate);
            if (reason != null)
            {
                context.ReportDiagnostic(Diagnostic.Create(UnsupportedMethod, candidate.Locations.FirstOrDefault(),
                    candidate.Name, reason));
                continue;
            }

            methods.Add(candidate);
        }

        if (methods.Count == 0)
        {
            return;
        }

        var source = new StringBuilder();
        if (!symbol.ContainingNamespace.IsGlobalNamespace)
        {
            source.Append("namespace ").AppendLine(symbol.ContainingNamespace.ToDisplayString());
            source.AppendLine("{");
        }

        source.Append("partial class @").AppendLine(symbol.Name);
        source.AppendLine("{");
        source.Append("    ").Append(GetOverrideAccessibility(symbol, "InvokeMingClassMethod"))
            .AppendLine(" override void InvokeMingClassMethod(in global::Ming.String methodName, global::Ming.NativeVariantPtrArgs args,");
        source.AppendLine("        out global::Ming.ming_variant ret, out global::Ming.ming_call_error error)");
        source.AppendLine("    {");
        source.AppendLine("        string methodNameStr = methodName.ToString();");
        foreach (var group in methods.GroupBy(m => m.Name))
        {
            source.AppendLine();
            source.Append("        if (methodNameStr == ").Append(SymbolDisplay.FormatLiteral(group.Key, true)).AppendLine(")");
            source.AppendLine("        {");
            source.AppendLine("            ret = default;");
            source.AppendLine("            error = new global::Ming.ming_call_error();");
            var overloads = group.OrderBy(m => m.Parameters.Length).ToArray();
            foreach (var method in overloads)
            {
                source.AppendLine();
                source.Append("            if (args.Count == ").Append(method.Parameters.Length).AppendLine(")");
                source.AppendLine("            {");
                for (int i = 0; i < method.Parameters.Length; i++)
                {
                    var parameter = method.Parameters[i];
                    TryGetVariantType(parameter.Type, true, out var variantType);
                    string type = parameter.Type.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat);
                    source.Append("                if (!global::Ming.VariantUtils.TryConvertTo<").Append(type)
                        .Append(">(args[").Append(i).Append("], ").Append(i).Append(", global::Ming.Variant.Type.")
                        .Append(variantType).Append(", out ").Append(type).Append(" arg").Append(i).AppendLine(", out error))");
                    source.AppendLine("                {");
                    source.AppendLine("                    return;");
                    source.AppendLine("                }");
                    source.AppendLine();
                }

                string call = "this.@" + method.Name + "(" + string.Join(", ",
                    Enumerable.Range(0, method.Parameters.Length).Select(i => "arg" + i)) + ")";
                source.Append("                ");
                if (!method.ReturnsVoid)
                {
                    source.Append("ret = global::Ming.VariantUtils.CreateFrom(");
                }

                source.Append(call).AppendLine(method.ReturnsVoid ? ";" : ");");
                source.AppendLine("                return;");
                source.AppendLine("            }");
            }

            source.AppendLine();
            source.Append("            int expectedCount = ").Append(overloads[0].Parameters.Length).AppendLine(";");
            foreach (var method in overloads.Skip(1))
            {
                int count = method.Parameters.Length;
                source.Append("            if (global::System.Math.Abs(args.Count - ").Append(count)
                    .AppendLine(") < global::System.Math.Abs(args.Count - expectedCount))");
                source.AppendLine("            {");
                source.Append("                expectedCount = ").Append(count).AppendLine(";");
                source.AppendLine("            }");
                source.AppendLine();
            }

            source.AppendLine("            error.ExpectedArgumentCount = expectedCount;");
            source.AppendLine("            error.Error = args.Count < expectedCount");
            source.AppendLine("                ? global::Ming.CallErrorCode.TooFewArguments : global::Ming.CallErrorCode.TooManyArguments;");
            source.AppendLine("            return;");
            source.AppendLine("        }");
        }

        source.AppendLine();
        source.AppendLine("        base.InvokeMingClassMethod(in methodName, args, out ret, out error);");
        source.AppendLine("    }");
        source.AppendLine();
        source.Append("    ").Append(GetOverrideAccessibility(symbol, "HasMingClassMethod"))
            .AppendLine(" override bool HasMingClassMethod(in global::Ming.String methodName)");
        source.AppendLine("    {");
        source.AppendLine("        string methodNameStr = methodName.ToString();");
        source.AppendLine();
        var names = methods.Select(m => m.Name).Distinct().ToArray();
        for (int i = 0; i < names.Length; i++)
        {
            source.Append(i == 0 ? "        if (" : "            ");
            source.Append("methodNameStr == ").Append(SymbolDisplay.FormatLiteral(names[i], true));
            source.AppendLine(i == names.Length - 1 ? ")" : " ||");
        }

        source.AppendLine("        {");
        source.AppendLine("            return true;");
        source.AppendLine("        }");
        source.AppendLine();
        source.AppendLine("        return base.HasMingClassMethod(in methodName);");
        source.AppendLine("    }");
        source.AppendLine("}");
        if (!symbol.ContainingNamespace.IsGlobalNamespace)
        {
            source.AppendLine("}");
        }

        AddSource(context, symbol, ".ScriptMethods.g.cs", source);
    }

    private static string? GetUnsupportedMethodReason(IMethodSymbol method)
    {
        if (method.IsGenericMethod)
        {
            return "generic methods are not supported";
        }

        if (method.ReturnsByRef || method.ReturnsByRefReadonly)
        {
            return "reference returns are not supported";
        }

        foreach (var parameter in method.Parameters)
        {
            if (parameter.RefKind != RefKind.None)
            {
                return "ref, in and out parameters are not supported";
            }

            if (!TryGetVariantType(parameter.Type, true, out _))
            {
                return "unsupported parameter type '" + parameter.Type.ToDisplayString() + "'";
            }
        }

        if (!method.ReturnsVoid && !TryGetVariantType(method.ReturnType, false, out _))
        {
            return "unsupported return type '" + method.ReturnType.ToDisplayString() + "'";
        }

        return null;
    }

    private static bool TryGetVariantType(ITypeSymbol type, bool reading, out string name)
    {
        name = string.Empty;
        switch (type.SpecialType)
        {
            case SpecialType.System_Boolean: name = "Bool"; break;
            case SpecialType.System_Int32: name = "Int"; break;
            case SpecialType.System_UInt32: name = "UInt32"; break;
            case SpecialType.System_Single: name = "Float"; break;
            case SpecialType.System_String: name = "String"; break;
        }

        if (name.Length != 0)
        {
            return true;
        }

        if (type.TypeKind == TypeKind.Enum)
        {
            name = "Int";
            return ((INamedTypeSymbol)type).EnumUnderlyingType?.SpecialType == SpecialType.System_Int32;
        }

        string fullName = type.ToDisplayString();
        switch (fullName)
        {
            case "Ming.Vector2":
            case "Ming.Vector3":
            case "Ming.Vector4":
            case "Ming.Color":
            case "Ming.AABB2":
            case "Ming.EulerAngles":
                name = type.Name;
                return true;
            case "Ming.OBB2":
            case "Ming.Capsule3":
            case "Ming.Matrix4x4":
                name = type.Name;
                return reading;
        }

        for (var current = type as INamedTypeSymbol; current != null; current = current.BaseType)
        {
            if (current.Name == "MingObject" && current.ContainingNamespace.ToDisplayString() == "Ming")
            {
                name = "ObjectPtr";
                return true;
            }
        }

        return false;
    }

    private static string GetOverrideAccessibility(INamedTypeSymbol symbol, string methodName)
    {
        for (var current = symbol.BaseType; current != null; current = current.BaseType)
        {
            var method = current.GetMembers(methodName).OfType<IMethodSymbol>().FirstOrDefault();
            if (method != null)
            {
                return method.DeclaredAccessibility == Accessibility.ProtectedOrInternal
                    && SymbolEqualityComparer.Default.Equals(method.ContainingAssembly, symbol.ContainingAssembly)
                    ? "protected internal" : "protected";
            }
        }

        return "protected";
    }

    #endregion

    #region Shared helpers

    private static void AddSource(GeneratorExecutionContext context, INamedTypeSymbol symbol, string suffix, StringBuilder source)
    {
        var hintName = BitConverter.ToString(Encoding.UTF8.GetBytes(symbol.ToDisplayString())).Replace("-", "") + suffix;
        context.AddSource(hintName, SourceText.From(source.ToString(), Encoding.UTF8));
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

    #endregion
}
