"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.SkylangDiagnosticsProvider = void 0;
const vscode = require("vscode");
const stdlib_1 = require("./data/stdlib");
class SkylangDiagnosticsProvider {
    diagnosticCollection;
    constructor() {
        this.diagnosticCollection = vscode.languages.createDiagnosticCollection('skylang');
    }
    getCollection() {
        return this.diagnosticCollection;
    }
    refreshDiagnostics(document) {
        if (document.languageId !== 'skylang')
            return;
        const config = vscode.workspace.getConfiguration('skylang');
        const enabled = config.get('diagnostics.enabled', true);
        if (!enabled) {
            this.diagnosticCollection.delete(document.uri);
            return;
        }
        const diagnostics = [];
        const text = document.getText();
        const lines = text.split('\n');
        let braceBalance = 0;
        let parenBalance = 0;
        let bracketBalance = 0;
        for (let i = 0; i < lines.length; i++) {
            const rawLine = lines[i];
            const line = rawLine.replace(/\/\/.*$/, '').replace(/#.*$/, '').trim();
            // 1. Bracketless function check: f foo(...)
            const bracketedFnMatch = line.match(/^f\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(/);
            if (bracketedFnMatch) {
                const startCol = rawLine.indexOf('(');
                const endCol = rawLine.indexOf(')', startCol) + 1;
                const range = new vscode.Range(new vscode.Position(i, startCol), new vscode.Position(i, endCol > startCol ? endCol : startCol + 1));
                const diag = new vscode.Diagnostic(range, 'Skylang function declarations are strictly bracketless. Use `takes(...)` inside the function body instead of parentheses after the function name.', vscode.DiagnosticSeverity.Warning);
                diag.code = 'skylang-bracketless-fn';
                diagnostics.push(diag);
            }
            // 2. Unterminated string check on single lines
            let inString = false;
            let stringChar = '';
            let stringStartCol = -1;
            for (let c = 0; c < rawLine.length; c++) {
                const ch = rawLine[c];
                const prev = c > 0 ? rawLine[c - 1] : '';
                if (inString) {
                    if (ch === stringChar && prev !== '\\') {
                        inString = false;
                    }
                }
                else {
                    if ((ch === '/' && c + 1 < rawLine.length && rawLine[c + 1] === '/') || ch === '#') {
                        break; // comment starts
                    }
                    if (ch === '"' || ch === '\'') {
                        inString = true;
                        stringChar = ch;
                        stringStartCol = c;
                    }
                    else if (ch === '{')
                        braceBalance++;
                    else if (ch === '}')
                        braceBalance--;
                    else if (ch === '(')
                        parenBalance++;
                    else if (ch === ')')
                        parenBalance--;
                    else if (ch === '[')
                        bracketBalance++;
                    else if (ch === ']')
                        bracketBalance--;
                }
            }
            if (inString) {
                const range = new vscode.Range(new vscode.Position(i, stringStartCol), new vscode.Position(i, rawLine.length));
                const diag = new vscode.Diagnostic(range, `Unterminated string literal starting with ${stringChar}`, vscode.DiagnosticSeverity.Error);
                diag.code = 'skylang-unterminated-string';
                diagnostics.push(diag);
            }
            // 3. Unknown method on standard library & interop modules
            const moduleCallMatch = line.match(/\b(math|io|fmt|str|python|js|npm|cpp|java)\.([a-zA-Z_][a-zA-Z0-9_]*)/g);
            if (moduleCallMatch) {
                for (const match of moduleCallMatch) {
                    const [mod, member] = match.split('.');
                    if (stdlib_1.STDLIB_MODULES[mod]) {
                        const modDef = stdlib_1.STDLIB_MODULES[mod];
                        const isFn = modDef.functions && modDef.functions[member] !== undefined;
                        const isConst = modDef.constants && modDef.constants[member] !== undefined;
                        // For math/io/fmt/str (static stdlib), check invalid member names
                        if (['math', 'io', 'fmt', 'str'].includes(mod) && !isFn && !isConst) {
                            const col = rawLine.indexOf(match);
                            if (col >= 0) {
                                const range = new vscode.Range(new vscode.Position(i, col), new vscode.Position(i, col + match.length));
                                const diag = new vscode.Diagnostic(range, `Module '${mod}' has no member '${member}'. Available functions: ${Object.keys(modDef.functions).join(', ')}`, vscode.DiagnosticSeverity.Error);
                                diag.code = 'skylang-invalid-module-member';
                                diagnostics.push(diag);
                            }
                        }
                    }
                }
            }
            // 4. Unimported module check for interop modules (python, js, cpp, java, npm)
            const interopMods = ['python', 'js', 'cpp', 'java', 'npm'];
            for (const mod of interopMods) {
                const targetMod = mod === 'npm' ? 'js' : mod;
                const modUsageRegex = new RegExp(`\\b${mod}\\.([a-zA-Z_][a-zA-Z0-9_]*)`, 'g');
                let m;
                while ((m = modUsageRegex.exec(rawLine)) !== null) {
                    const importRegex = new RegExp(`^\\s*import\\s+[^;\\n]*\\b${targetMod}\\b`, 'm');
                    if (!importRegex.test(text)) {
                        const startCol = m.index;
                        const endCol = startCol + mod.length;
                        const range = new vscode.Range(new vscode.Position(i, startCol), new vscode.Position(i, endCol));
                        const diag = new vscode.Diagnostic(range, `Module '${targetMod}' is used without being imported. Add 'import ${targetMod}' at top of file.`, vscode.DiagnosticSeverity.Information);
                        diag.code = 'skylang-unimported-module';
                        diagnostics.push(diag);
                    }
                }
            }
        }
        // 4. Global balance checks
        if (braceBalance !== 0) {
            const lastLine = lines.length - 1;
            const range = new vscode.Range(new vscode.Position(lastLine, 0), new vscode.Position(lastLine, lines[lastLine].length));
            diagnostics.push(new vscode.Diagnostic(range, braceBalance > 0
                ? `Unclosed curly brace '{' (${braceBalance} missing '}')`
                : `Extra closing curly brace '}'`, vscode.DiagnosticSeverity.Error));
        }
        if (parenBalance !== 0) {
            const lastLine = lines.length - 1;
            const range = new vscode.Range(new vscode.Position(lastLine, 0), new vscode.Position(lastLine, lines[lastLine].length));
            diagnostics.push(new vscode.Diagnostic(range, parenBalance > 0 ? `Unclosed parenthesis '('` : `Extra closing parenthesis ')'`, vscode.DiagnosticSeverity.Error));
        }
        if (bracketBalance !== 0) {
            const lastLine = lines.length - 1;
            const range = new vscode.Range(new vscode.Position(lastLine, 0), new vscode.Position(lastLine, lines[lastLine].length));
            diagnostics.push(new vscode.Diagnostic(range, bracketBalance > 0 ? `Unclosed square bracket '['` : `Extra closing square bracket ']'`, vscode.DiagnosticSeverity.Error));
        }
        this.diagnosticCollection.set(document.uri, diagnostics);
    }
    clear(document) {
        this.diagnosticCollection.delete(document.uri);
    }
    dispose() {
        this.diagnosticCollection.dispose();
    }
}
exports.SkylangDiagnosticsProvider = SkylangDiagnosticsProvider;
//# sourceMappingURL=diagnostics.js.map