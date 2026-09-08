import * as vscode from 'vscode';
import { STDLIB_MODULES } from './data/stdlib';

export class SkylangDiagnosticsProvider {
    private diagnosticCollection: vscode.DiagnosticCollection;

    constructor() {
        this.diagnosticCollection = vscode.languages.createDiagnosticCollection('skylang');
    }

    public getCollection(): vscode.DiagnosticCollection {
        return this.diagnosticCollection;
    }

    public refreshDiagnostics(document: vscode.TextDocument): void {
        if (document.languageId !== 'skylang') return;

        const config = vscode.workspace.getConfiguration('skylang');
        const enabled = config.get<boolean>('diagnostics.enabled', true);
        if (!enabled) {
            this.diagnosticCollection.delete(document.uri);
            return;
        }

        const diagnostics: vscode.Diagnostic[] = [];
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
                const range = new vscode.Range(
                    new vscode.Position(i, startCol),
                    new vscode.Position(i, endCol > startCol ? endCol : startCol + 1)
                );
                const diag = new vscode.Diagnostic(
                    range,
                    'Skylang function declarations are strictly bracketless. Use `takes(...)` inside the function body instead of parentheses after the function name.',
                    vscode.DiagnosticSeverity.Warning
                );
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
                } else {
                    if ((ch === '/' && c + 1 < rawLine.length && rawLine[c + 1] === '/') || ch === '#') {
                        break; // comment starts
                    }
                    if (ch === '"' || ch === '\'') {
                        inString = true;
                        stringChar = ch;
                        stringStartCol = c;
                    } else if (ch === '{') braceBalance++;
                    else if (ch === '}') braceBalance--;
                    else if (ch === '(') parenBalance++;
                    else if (ch === ')') parenBalance--;
                    else if (ch === '[') bracketBalance++;
                    else if (ch === ']') bracketBalance--;
                }
            }

            if (inString) {
                const range = new vscode.Range(new vscode.Position(i, stringStartCol), new vscode.Position(i, rawLine.length));
                const diag = new vscode.Diagnostic(
                    range,
                    `Unterminated string literal starting with ${stringChar}`,
                    vscode.DiagnosticSeverity.Error
                );
                diag.code = 'skylang-unterminated-string';
                diagnostics.push(diag);
            }

            // 3. Import syntax & validity check
            const importMatch = line.match(/^import(?:\s+(.*)|$)/);
            if (importMatch && !line.startsWith('cimport')) {
                const importKeywordCol = rawLine.indexOf('import');
                const rawAfter = importMatch[1] !== undefined ? importMatch[1].trim() : '';

                if (!rawAfter) {
                    const range = new vscode.Range(
                        new vscode.Position(i, importKeywordCol),
                        new vscode.Position(i, importKeywordCol + 6)
                    );
                    const diag = new vscode.Diagnostic(
                        range,
                        "Expected module name, path, or language bridge after 'import'",
                        vscode.DiagnosticSeverity.Error
                    );
                    diag.code = 'skylang-invalid-import-syntax';
                    diagnostics.push(diag);
                } else {
                    const items = rawAfter.split(',');
                    let searchOffset = rawLine.indexOf(rawAfter, importKeywordCol + 6);

                    for (let idx = 0; idx < items.length; idx++) {
                        const rawItem = items[idx];
                        const trimmedItem = rawItem.trim();

                        if (trimmedItem.length === 0) {
                            const commaPos = rawLine.indexOf(',', searchOffset);
                            const pos = commaPos !== -1 ? commaPos : searchOffset;
                            const range = new vscode.Range(
                                new vscode.Position(i, pos),
                                new vscode.Position(i, pos + 1)
                            );
                            const diag = new vscode.Diagnostic(
                                range,
                                "Expected module name after ',' in import statement",
                                vscode.DiagnosticSeverity.Error
                            );
                            diag.code = 'skylang-invalid-import-syntax';
                            diagnostics.push(diag);
                            searchOffset = pos + 1;
                            continue;
                        }

                        const itemCol = rawLine.indexOf(trimmedItem, searchOffset);
                        const startCol = itemCol !== -1 ? itemCol : searchOffset;
                        const endCol = startCol + trimmedItem.length;
                        const range = new vscode.Range(
                            new vscode.Position(i, startCol),
                            new vscode.Position(i, endCol)
                        );
                        if (itemCol !== -1) {
                            searchOffset = endCol;
                        }

                        // Check alias: <module> as <alias>
                        const asParts = trimmedItem.split(/\s+as\s+/);
                        const modPart = asParts[0].trim();
                        const aliasPart = asParts.length > 1 ? asParts[1].trim() : null;

                        if (asParts.length > 1 && (!aliasPart || !/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(aliasPart))) {
                            const diag = new vscode.Diagnostic(
                                range,
                                `Invalid alias '${aliasPart || ''}' in import statement. Alias must be a valid identifier.`,
                                vscode.DiagnosticSeverity.Error
                            );
                            diag.code = 'skylang-invalid-import-alias';
                            diagnostics.push(diag);
                            continue;
                        }

                        // A. Quoted string file import: import "foo.sky"
                        if (
                            (modPart.startsWith('"') && modPart.endsWith('"')) ||
                            (modPart.startsWith("'") && modPart.endsWith("'"))
                        ) {
                            continue;
                        }

                        // B. Go library package / Python syntax error: import fmt
                        if (modPart === 'fmt') {
                            const diag = new vscode.Diagnostic(
                                range,
                                `'fmt' is not a Skylang library (for Go package, load with 'go.load("fmt")'). Use Python-style string formatting (e.g. str.format(), hex(), bin(), oct()).`,
                                vscode.DiagnosticSeverity.Error
                            );
                            diag.code = 'skylang-invalid-import-module';
                            diagnostics.push(diag);
                        }
                        // C. io is not a lib: import io
                        else if (modPart === 'io') {
                            const diag = new vscode.Diagnostic(
                                range,
                                `'io' is not a library in Skylang. Use Python-style file I/O built-ins (open(), input(), print()) without import.`,
                                vscode.DiagnosticSeverity.Error
                            );
                            diag.code = 'skylang-invalid-import-module';
                            diagnostics.push(diag);
                        }
                        // D. Built-in module: import math
                        else if (modPart === 'math') {
                            const diag = new vscode.Diagnostic(
                                range,
                                `'math' is built into Skylang and available globally without an 'import' statement.`,
                                vscode.DiagnosticSeverity.Warning
                            );
                            diag.code = 'skylang-unnecessary-import';
                            diagnostics.push(diag);
                        }
                        // E. Identifier or dotted path validation: e.g. math_utils, sub.helper
                        else if (!/^[a-zA-Z_][a-zA-Z0-9_]*(\.[a-zA-Z_][a-zA-Z0-9_]*)*$/.test(modPart)) {
                            const diag = new vscode.Diagnostic(
                                range,
                                `Invalid module name '${modPart}'. Must be an identifier, dotted submodule path, or string filename.`,
                                vscode.DiagnosticSeverity.Error
                            );
                            diag.code = 'skylang-invalid-import-syntax';
                            diagnostics.push(diag);
                        }
                    }
                }
            }

            // 3b. From ... import syntax check
            const fromMatch = line.match(/^from(?:\s+(.*)|$)/);
            if (fromMatch) {
                const fromKeywordCol = rawLine.indexOf('from');
                const rawAfterFrom = fromMatch[1] !== undefined ? fromMatch[1].trim() : '';

                if (!rawAfterFrom || !rawAfterFrom.includes('import')) {
                    const range = new vscode.Range(
                        new vscode.Position(i, fromKeywordCol),
                        new vscode.Position(i, rawLine.length)
                    );
                    const diag = new vscode.Diagnostic(
                        range,
                        "Expected 'from <module> import <symbols>' or 'from <module> import *'",
                        vscode.DiagnosticSeverity.Error
                    );
                    diag.code = 'skylang-invalid-from-import-syntax';
                    diagnostics.push(diag);
                } else {
                    const fromImportParts = rawAfterFrom.split(/\s+import\s+/);
                    const modPath = fromImportParts[0]?.trim();
                    const symbolsPart = fromImportParts[1]?.trim();

                    if (!modPath) {
                        const range = new vscode.Range(
                            new vscode.Position(i, fromKeywordCol),
                            new vscode.Position(i, fromKeywordCol + 4)
                        );
                        const diag = new vscode.Diagnostic(
                            range,
                            "Expected module name after 'from'",
                            vscode.DiagnosticSeverity.Error
                        );
                        diag.code = 'skylang-invalid-from-import-syntax';
                        diagnostics.push(diag);
                    } else if (modPath === 'fmt') {
                        const col = rawLine.indexOf('fmt', fromKeywordCol);
                        const range = new vscode.Range(new vscode.Position(i, col), new vscode.Position(i, col + 3));
                        const diag = new vscode.Diagnostic(
                            range,
                            `'fmt' is not a Skylang library (for Go package, load with 'go.load("fmt")'). Use Python-style string formatting.`,
                            vscode.DiagnosticSeverity.Error
                        );
                        diag.code = 'skylang-invalid-import-module';
                        diagnostics.push(diag);
                    } else if (modPath === 'io') {
                        const col = rawLine.indexOf('io', fromKeywordCol);
                        const range = new vscode.Range(new vscode.Position(i, col), new vscode.Position(i, col + 2));
                        const diag = new vscode.Diagnostic(
                            range,
                            `'io' is not a library in Skylang. Use Python-style built-ins (open(), input(), print()).`,
                            vscode.DiagnosticSeverity.Error
                        );
                        diag.code = 'skylang-invalid-import-module';
                        diagnostics.push(diag);
                    }

                    if (!symbolsPart) {
                        const importCol = rawLine.indexOf('import', fromKeywordCol);
                        const range = new vscode.Range(
                            new vscode.Position(i, importCol !== -1 ? importCol : fromKeywordCol),
                            new vscode.Position(i, rawLine.length)
                        );
                        const diag = new vscode.Diagnostic(
                            range,
                            "Expected symbols or '*' after 'import'",
                            vscode.DiagnosticSeverity.Error
                        );
                        diag.code = 'skylang-invalid-from-import-syntax';
                        diagnostics.push(diag);
                    } else if (symbolsPart !== '*') {
                        // Validate symbol list
                        const cleanSymbols = symbolsPart.replace(/^\(|\)$/g, '').trim();
                        const symItems = cleanSymbols.split(',');
                        for (const s of symItems) {
                            const trimmedS = s.trim();
                            if (trimmedS.length === 0) continue;
                            const symAs = trimmedS.split(/\s+as\s+/);
                            const symName = symAs[0].trim();
                            const symAlias = symAs.length > 1 ? symAs[1].trim() : null;

                            if (!/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(symName)) {
                                const sCol = rawLine.indexOf(symName);
                                const range = new vscode.Range(
                                    new vscode.Position(i, sCol !== -1 ? sCol : fromKeywordCol),
                                    new vscode.Position(i, sCol !== -1 ? sCol + symName.length : rawLine.length)
                                );
                                const diag = new vscode.Diagnostic(
                                    range,
                                    `Invalid symbol name '${symName}' in import statement.`,
                                    vscode.DiagnosticSeverity.Error
                                );
                                diag.code = 'skylang-invalid-import-syntax';
                                diagnostics.push(diag);
                            }
                            if (symAlias && !/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(symAlias)) {
                                const aCol = rawLine.indexOf(symAlias);
                                const range = new vscode.Range(
                                    new vscode.Position(i, aCol !== -1 ? aCol : fromKeywordCol),
                                    new vscode.Position(i, aCol !== -1 ? aCol + symAlias.length : rawLine.length)
                                );
                                const diag = new vscode.Diagnostic(
                                    range,
                                    `Invalid alias '${symAlias}' in import statement.`,
                                    vscode.DiagnosticSeverity.Error
                                );
                                diag.code = 'skylang-invalid-import-alias';
                                diagnostics.push(diag);
                            }
                        }
                    }
                }
            }

            // 4. Invalid io. or fmt. module call check (Python syntax reminder)
            const invalidLibMatch = line.match(/\b(io|fmt)\.([a-zA-Z_][a-zA-Z0-9_]*)/g);
            if (invalidLibMatch) {
                for (const match of invalidLibMatch) {
                    const [mod, member] = match.split('.');
                    const col = rawLine.indexOf(match);
                    if (col >= 0) {
                        const range = new vscode.Range(
                            new vscode.Position(i, col),
                            new vscode.Position(i, col + match.length)
                        );
                        let hint = '';
                        if (mod === 'io') {
                            if (member === 'readfile' || member === 'readlines') {
                                hint = `Use Python-style 'open(filepath).${member === 'readfile' ? 'read()' : 'readlines()'}' instead.`;
                            } else if (member === 'input') {
                                hint = `Use Python-style built-in 'input([prompt])' directly.`;
                            } else {
                                hint = `Use Python-style built-in file operations (open(), input(), print()) instead.`;
                            }
                        } else if (mod === 'fmt') {
                            if (member === 'format') {
                                hint = `Use Python-style string method 'template.format(...)' instead.`;
                            } else if (member === 'hex' || member === 'bin' || member === 'oct') {
                                hint = `Use Python-style built-in '${member}(...)' directly.`;
                            } else if (member === 'pad' || member === 'padleft') {
                                hint = `Use Python-style string method 'str.ljust()' or 'str.rjust()' instead.`;
                            } else {
                                hint = `Use Python-style formatting ('str.format()', hex(), bin(), oct()) or load Go fmt via 'go.load("fmt")'.`;
                            }
                        }
                        const diag = new vscode.Diagnostic(
                            range,
                            `'${mod}' is not a library in Skylang. ${hint}`,
                            vscode.DiagnosticSeverity.Error
                        );
                        diag.code = 'skylang-invalid-module-usage';
                        diagnostics.push(diag);
                    }
                }
            }

            // 5. Unknown method on standard library & interop modules
            const moduleCallMatch = line.match(/\b(math|python|js|cpp|java|go|golang|rust)\.([a-zA-Z_][a-zA-Z0-9_]*)/g);
            if (moduleCallMatch) {
                for (const match of moduleCallMatch) {
                    const [mod, member] = match.split('.');
                    if (STDLIB_MODULES[mod]) {
                        const modDef = STDLIB_MODULES[mod];
                        const isFn = modDef.functions && modDef.functions[member] !== undefined;
                        const isConst = modDef.constants && modDef.constants[member] !== undefined;

                        // For math (static stdlib), check invalid member names
                        if (mod === 'math' && !isFn && !isConst) {
                            const col = rawLine.indexOf(match);
                            if (col >= 0) {
                                const range = new vscode.Range(
                                    new vscode.Position(i, col),
                                    new vscode.Position(i, col + match.length)
                                );
                                const diag = new vscode.Diagnostic(
                                    range,
                                    `Module '${mod}' has no member '${member}'. Available functions: ${Object.keys(modDef.functions).join(', ')}`,
                                    vscode.DiagnosticSeverity.Error
                                );
                                diag.code = 'skylang-invalid-module-member';
                                diagnostics.push(diag);
                            }
                        }
                    }
                }
            }

            // 5. Unimported module check for interop modules (python, js, cpp, java, go, golang, rust)
            const interopMods = ['python', 'js', 'cpp', 'java', 'go', 'golang', 'rust'];
            for (const mod of interopMods) {
                const modUsageRegex = new RegExp(`\\b${mod}\\.([a-zA-Z_][a-zA-Z0-9_]*)`, 'g');
                let m: RegExpExecArray | null;
                while ((m = modUsageRegex.exec(rawLine)) !== null) {
                    const importRegex = new RegExp(`^\\s*import\\s+[^;\\n]*\\b${mod}\\b`, 'm');
                    if (!importRegex.test(text)) {
                        const startCol = m.index;
                        const endCol = startCol + mod.length;
                        const range = new vscode.Range(
                            new vscode.Position(i, startCol),
                            new vscode.Position(i, endCol)
                        );
                        const diag = new vscode.Diagnostic(
                            range,
                            `Module '${mod}' is used without being imported. Add 'import ${mod}' at top of file.`,
                            vscode.DiagnosticSeverity.Information
                        );
                        diag.code = 'skylang-unimported-module';
                        diagnostics.push(diag);
                    }
                }
            }
        }

        // 4. Global balance checks
        if (braceBalance !== 0) {
            const lastLine = lines.length - 1;
            const range = new vscode.Range(
                new vscode.Position(lastLine, 0),
                new vscode.Position(lastLine, lines[lastLine].length)
            );
            diagnostics.push(
                new vscode.Diagnostic(
                    range,
                    braceBalance > 0
                        ? `Unclosed curly brace '{' (${braceBalance} missing '}')`
                        : `Extra closing curly brace '}'`,
                    vscode.DiagnosticSeverity.Error
                )
            );
        }

        if (parenBalance !== 0) {
            const lastLine = lines.length - 1;
            const range = new vscode.Range(
                new vscode.Position(lastLine, 0),
                new vscode.Position(lastLine, lines[lastLine].length)
            );
            diagnostics.push(
                new vscode.Diagnostic(
                    range,
                    parenBalance > 0 ? `Unclosed parenthesis '('` : `Extra closing parenthesis ')'`,
                    vscode.DiagnosticSeverity.Error
                )
            );
        }

        if (bracketBalance !== 0) {
            const lastLine = lines.length - 1;
            const range = new vscode.Range(
                new vscode.Position(lastLine, 0),
                new vscode.Position(lastLine, lines[lastLine].length)
            );
            diagnostics.push(
                new vscode.Diagnostic(
                    range,
                    bracketBalance > 0 ? `Unclosed square bracket '['` : `Extra closing square bracket ']'`,
                    vscode.DiagnosticSeverity.Error
                )
            );
        }

        this.diagnosticCollection.set(document.uri, diagnostics);
    }

    public clear(document: vscode.TextDocument): void {
        this.diagnosticCollection.delete(document.uri);
    }

    public dispose(): void {
        this.diagnosticCollection.dispose();
    }
}
