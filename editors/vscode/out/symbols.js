"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.SkylangWorkspaceSymbolProvider = exports.SkylangDocumentSymbolProvider = void 0;
const vscode = require("vscode");
class SkylangDocumentSymbolProvider {
    provideDocumentSymbols(document, _token) {
        const symbols = [];
        const text = document.getText();
        const lines = text.split('\n');
        let currentClassSymbol = null;
        let classEndLine = -1;
        for (let i = 0; i < lines.length; i++) {
            const rawLine = lines[i];
            const line = rawLine.replace(/\/\/.*$/, '').replace(/#.*$/, '').trim();
            if (!line)
                continue;
            // Class declaration: class Dog {
            const classMatch = line.match(/^class\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
            if (classMatch) {
                const className = classMatch[1];
                const classRange = this.findBlockRange(document, i);
                const col = rawLine.indexOf(className);
                const selRange = new vscode.Range(new vscode.Position(i, col >= 0 ? col : 0), new vscode.Position(i, (col >= 0 ? col : 0) + className.length));
                currentClassSymbol = new vscode.DocumentSymbol(className, 'class', vscode.SymbolKind.Class, classRange, selRange);
                symbols.push(currentClassSymbol);
                classEndLine = classRange.end.line;
                continue;
            }
            // Standalone function declaration: f add {
            const fnMatch = line.match(/^f\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
            if (fnMatch) {
                const fnName = fnMatch[1];
                const fnRange = this.findBlockRange(document, i);
                const col = rawLine.indexOf(fnName);
                const selRange = new vscode.Range(new vscode.Position(i, col >= 0 ? col : 0), new vscode.Position(i, (col >= 0 ? col : 0) + fnName.length));
                const params = this.extractTakesParams(lines, i + 1);
                const detail = params.length > 0 ? `(${params.join(', ')})` : '()';
                const fnSym = new vscode.DocumentSymbol(fnName, detail, vscode.SymbolKind.Function, fnRange, selRange);
                symbols.push(fnSym);
                continue;
            }
            // Extern C function declaration: extern f cos(x)
            const externMatch = line.match(/^extern\s+f\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)/);
            if (externMatch) {
                const extName = externMatch[1];
                const extParams = externMatch[2];
                const col = rawLine.indexOf(extName);
                const range = new vscode.Range(new vscode.Position(i, 0), new vscode.Position(i, rawLine.length));
                const selRange = new vscode.Range(new vscode.Position(i, col >= 0 ? col : 0), new vscode.Position(i, (col >= 0 ? col : 0) + extName.length));
                const extSym = new vscode.DocumentSymbol(extName, `extern (${extParams})`, vscode.SymbolKind.Function, range, selRange);
                symbols.push(extSym);
                continue;
            }
            // Inside class: constructor, methods, and fields
            if (currentClassSymbol && i <= classEndLine) {
                // Class constructor: init {
                if (line.startsWith('init') && line.includes('{')) {
                    const initRange = this.findBlockRange(document, i);
                    const col = rawLine.indexOf('init');
                    const selRange = new vscode.Range(new vscode.Position(i, col >= 0 ? col : 0), new vscode.Position(i, (col >= 0 ? col : 0) + 4));
                    const params = this.extractTakesParams(lines, i + 1);
                    const initSym = new vscode.DocumentSymbol('init', `(${params.join(', ')})`, vscode.SymbolKind.Constructor, initRange, selRange);
                    currentClassSymbol.children.push(initSym);
                    continue;
                }
                // Class method: bark {
                const methodMatch = line.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s*\{/);
                if (methodMatch &&
                    methodMatch[1] !== 'if' &&
                    methodMatch[1] !== 'for' &&
                    methodMatch[1] !== 'else' &&
                    methodMatch[1] !== 'elif' &&
                    methodMatch[1] !== 'init') {
                    const methodName = methodMatch[1];
                    const methodRange = this.findBlockRange(document, i);
                    const col = rawLine.indexOf(methodName);
                    const selRange = new vscode.Range(new vscode.Position(i, col >= 0 ? col : 0), new vscode.Position(i, (col >= 0 ? col : 0) + methodName.length));
                    const params = this.extractTakesParams(lines, i + 1);
                    const methodSym = new vscode.DocumentSymbol(methodName, `(${params.join(', ')})`, vscode.SymbolKind.Method, methodRange, selRange);
                    currentClassSymbol.children.push(methodSym);
                    continue;
                }
                // Class field: this.fieldName
                const fieldMatch = line.match(/^this\.([a-zA-Z_][a-zA-Z0-9_]*)/);
                if (fieldMatch) {
                    const fieldName = fieldMatch[1];
                    const col = rawLine.indexOf(fieldName);
                    const range = new vscode.Range(new vscode.Position(i, 0), new vscode.Position(i, rawLine.length));
                    const selRange = new vscode.Range(new vscode.Position(i, col >= 0 ? col : 0), new vscode.Position(i, (col >= 0 ? col : 0) + fieldName.length));
                    const fieldSym = new vscode.DocumentSymbol(fieldName, 'field', vscode.SymbolKind.Field, range, selRange);
                    currentClassSymbol.children.push(fieldSym);
                    continue;
                }
            }
            // Top-level variable declaration: x := 10 or I count = 10
            if (!currentClassSymbol || i > classEndLine) {
                const walrusMatch = line.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s*:=/);
                if (walrusMatch && walrusMatch[1] !== '_') {
                    const varName = walrusMatch[1];
                    const col = rawLine.indexOf(varName);
                    const range = new vscode.Range(new vscode.Position(i, 0), new vscode.Position(i, rawLine.length));
                    const selRange = new vscode.Range(new vscode.Position(i, col >= 0 ? col : 0), new vscode.Position(i, (col >= 0 ? col : 0) + varName.length));
                    symbols.push(new vscode.DocumentSymbol(varName, 'variable', vscode.SymbolKind.Variable, range, selRange));
                }
                const typedMatch = line.match(/^([IDBCS])\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
                if (typedMatch) {
                    const typeTok = typedMatch[1];
                    const varName = typedMatch[2];
                    const col = rawLine.indexOf(varName);
                    const range = new vscode.Range(new vscode.Position(i, 0), new vscode.Position(i, rawLine.length));
                    const selRange = new vscode.Range(new vscode.Position(i, col >= 0 ? col : 0), new vscode.Position(i, (col >= 0 ? col : 0) + varName.length));
                    symbols.push(new vscode.DocumentSymbol(varName, typeTok, vscode.SymbolKind.Variable, range, selRange));
                }
                const collMatch = line.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s+(L|T|DICT|SET|SL)\b/);
                if (collMatch) {
                    const varName = collMatch[1];
                    const typeTok = collMatch[2];
                    const col = rawLine.indexOf(varName);
                    const range = new vscode.Range(new vscode.Position(i, 0), new vscode.Position(i, rawLine.length));
                    const selRange = new vscode.Range(new vscode.Position(i, col >= 0 ? col : 0), new vscode.Position(i, (col >= 0 ? col : 0) + varName.length));
                    symbols.push(new vscode.DocumentSymbol(varName, typeTok, vscode.SymbolKind.Variable, range, selRange));
                }
            }
        }
        return symbols;
    }
    findBlockRange(document, startLine) {
        const lineCount = document.lineCount;
        let depth = 0;
        let started = false;
        for (let i = startLine; i < lineCount; i++) {
            const line = document.lineAt(i).text;
            for (const ch of line) {
                if (ch === '{') {
                    depth++;
                    started = true;
                }
                else if (ch === '}') {
                    depth--;
                    if (started && depth <= 0) {
                        return new vscode.Range(new vscode.Position(startLine, 0), new vscode.Position(i, line.length));
                    }
                }
            }
        }
        return new vscode.Range(new vscode.Position(startLine, 0), new vscode.Position(startLine, document.lineAt(startLine).text.length));
    }
    extractTakesParams(lines, startLine) {
        for (let i = startLine; i < Math.min(startLine + 6, lines.length); i++) {
            const line = lines[i].trim();
            const takesMatch = line.match(/takes\s*\(([^)]*)\)/);
            if (takesMatch) {
                return takesMatch[1].split(',').map(p => p.trim()).filter(p => p.length > 0);
            }
            if (line.includes('}'))
                break;
        }
        return [];
    }
}
exports.SkylangDocumentSymbolProvider = SkylangDocumentSymbolProvider;
class SkylangWorkspaceSymbolProvider {
    async provideWorkspaceSymbols(query, token) {
        const symbols = [];
        const files = await vscode.workspace.findFiles('**/*.{sky,skylang}', '**/node_modules/**', 100, token);
        for (const file of files) {
            try {
                const doc = await vscode.workspace.openTextDocument(file);
                const text = doc.getText();
                const lines = text.split('\n');
                for (let i = 0; i < lines.length; i++) {
                    const line = lines[i];
                    const fnMatch = line.match(/^\s*f\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
                    if (fnMatch && (!query || fnMatch[1].toLowerCase().includes(query.toLowerCase()))) {
                        symbols.push(new vscode.SymbolInformation(fnMatch[1], vscode.SymbolKind.Function, '', new vscode.Location(file, new vscode.Position(i, line.indexOf(fnMatch[1])))));
                    }
                    const classMatch = line.match(/^\s*class\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
                    if (classMatch && (!query || classMatch[1].toLowerCase().includes(query.toLowerCase()))) {
                        symbols.push(new vscode.SymbolInformation(classMatch[1], vscode.SymbolKind.Class, '', new vscode.Location(file, new vscode.Position(i, line.indexOf(classMatch[1])))));
                    }
                    const externMatch = line.match(/^\s*extern\s+f\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
                    if (externMatch && (!query || externMatch[1].toLowerCase().includes(query.toLowerCase()))) {
                        symbols.push(new vscode.SymbolInformation(externMatch[1], vscode.SymbolKind.Function, 'extern', new vscode.Location(file, new vscode.Position(i, line.indexOf(externMatch[1])))));
                    }
                }
            }
            catch {
                // Ignore unreadable files
            }
        }
        return symbols;
    }
}
exports.SkylangWorkspaceSymbolProvider = SkylangWorkspaceSymbolProvider;
//# sourceMappingURL=symbols.js.map