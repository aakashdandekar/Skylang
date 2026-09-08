"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.SkylangCodeActionProvider = void 0;
const vscode = __importStar(require("vscode"));
class SkylangCodeActionProvider {
    static providedCodeActionKinds = [
        vscode.CodeActionKind.QuickFix
    ];
    provideCodeActions(document, range, context, _token) {
        const actions = [];
        const text = document.getText();
        const line = document.lineAt(range.start.line).text;
        const checkModules = ['python', 'js', 'cpp', 'java', 'go', 'golang', 'rust'];
        // 1. Diagnostics-driven QuickFix
        for (const diagnostic of context.diagnostics) {
            if (diagnostic.code === 'skylang-unimported-module') {
                const match = diagnostic.message.match(/Module '([a-zA-Z0-9_]+)'/);
                if (match) {
                    const mod = match[1];
                    const action = this.createImportAction(document, mod, diagnostic);
                    if (action)
                        actions.push(action);
                }
            }
            else if (diagnostic.code === 'skylang-invalid-import-syntax') {
                const targetText = document.getText(diagnostic.range);
                if (targetText.includes('.')) {
                    const fixed = targetText
                        .split('.')
                        .map(s => s.trim())
                        .filter(Boolean)
                        .join(', ');
                    const action = new vscode.CodeAction(`Convert '${targetText}' to comma-separated 'import ${fixed}'`, vscode.CodeActionKind.QuickFix);
                    action.edit = new vscode.WorkspaceEdit();
                    action.edit.replace(document.uri, diagnostic.range, fixed);
                    action.diagnostics = [diagnostic];
                    action.isPreferred = true;
                    actions.push(action);
                }
            }
            else if (diagnostic.code === 'skylang-invalid-import-module') {
                const targetText = document.getText(diagnostic.range);
                if (targetText === 'fmt') {
                    const lineIdx = diagnostic.range.start.line;
                    const action = new vscode.CodeAction(`Replace with 'import go' and load 'fmt' via 'fmt := go.load("fmt")'`, vscode.CodeActionKind.QuickFix);
                    action.edit = new vscode.WorkspaceEdit();
                    const lineRange = document.lineAt(lineIdx).range;
                    action.edit.replace(document.uri, lineRange, `import go\nfmt := go.load("fmt")`);
                    action.diagnostics = [diagnostic];
                    action.isPreferred = true;
                    actions.push(action);
                }
            }
            else if (diagnostic.code === 'skylang-unnecessary-import') {
                const lineIdx = diagnostic.range.start.line;
                const lineText = document.lineAt(lineIdx).text.trim();
                const action = new vscode.CodeAction(`Remove unnecessary import statement`, vscode.CodeActionKind.QuickFix);
                action.edit = new vscode.WorkspaceEdit();
                if (lineText.startsWith('import ') && !lineText.includes(',')) {
                    action.edit.delete(document.uri, document.lineAt(lineIdx).rangeIncludingLineBreak);
                }
                else {
                    action.edit.delete(document.uri, diagnostic.range);
                }
                action.diagnostics = [diagnostic];
                action.isPreferred = true;
                actions.push(action);
            }
        }
        // 2. Cursor/selection context QuickFix
        for (const mod of checkModules) {
            const modRegex = new RegExp(`\\b${mod}\\b`);
            if (modRegex.test(line)) {
                const importRegex = new RegExp(`^\\s*import\\s+[^;\\n]*\\b${mod}\\b`, 'm');
                if (!importRegex.test(text)) {
                    if (!actions.some(a => a.title.includes(`'${mod}'`))) {
                        const action = this.createImportAction(document, mod);
                        if (action)
                            actions.push(action);
                    }
                }
            }
        }
        // 3. Known Python package reference QuickFix: numpy.array(...) -> import python and load
        const pythonPkgs = ['numpy', 'pandas', 'torch', 'scipy', 'requests', 'matplotlib', 'sklearn', 'fastapi', 'flask', 'django', 'cv2', 'PIL', 'polars'];
        for (const pkg of pythonPkgs) {
            const pkgRegex = new RegExp(`\\b${pkg}\\.`);
            if (pkgRegex.test(line)) {
                const action = new vscode.CodeAction(`Import 'python' and load package '${pkg}'`, vscode.CodeActionKind.QuickFix);
                action.edit = new vscode.WorkspaceEdit();
                const lines = text.split('\n');
                let insertLine = 0;
                let foundImport = false;
                for (let i = 0; i < lines.length; i++) {
                    const l = lines[i].trim();
                    if (l.startsWith('import ') || l.startsWith('cimport ')) {
                        insertLine = i + 1;
                        foundImport = true;
                    }
                    else if (!foundImport && (l.startsWith('//') || l.startsWith('#') || l === '')) {
                        insertLine = i + 1;
                    }
                    else {
                        if (foundImport)
                            break;
                    }
                }
                const importPyRegex = /^\s*import\s+[^;\n]*\bpython\b/m;
                let insertText = '';
                if (!importPyRegex.test(text)) {
                    insertText += `import python\n`;
                }
                const loadRegex = new RegExp(`\\b${pkg}\\s*:=\\s*python\\.(?:load|import)\\("${pkg}"\\)`);
                if (!loadRegex.test(text)) {
                    insertText += `${pkg} := python.load("${pkg}")\n`;
                }
                if (insertText.length > 0) {
                    action.edit.insert(document.uri, new vscode.Position(insertLine, 0), insertText);
                    action.isPreferred = true;
                    actions.push(action);
                }
            }
        }
        return actions;
    }
    createImportAction(document, modName, diagnostic, customTitle) {
        const text = document.getText();
        const importRegex = new RegExp(`^\\s*import\\s+[^;\\n]*\\b${modName}\\b`, 'm');
        if (importRegex.test(text)) {
            return null;
        }
        const title = customTitle || `Import '${modName}' module at top of file`;
        const action = new vscode.CodeAction(title, vscode.CodeActionKind.QuickFix);
        if (diagnostic) {
            action.diagnostics = [diagnostic];
        }
        action.isPreferred = true;
        const lines = text.split('\n');
        let insertLine = 0;
        let foundExistingImport = false;
        for (let i = 0; i < lines.length; i++) {
            const l = lines[i].trim();
            if (l.startsWith('import ') || l.startsWith('cimport ')) {
                insertLine = i + 1;
                foundExistingImport = true;
            }
            else if (!foundExistingImport && (l.startsWith('//') || l.startsWith('#') || l === '')) {
                insertLine = i + 1;
            }
            else {
                if (foundExistingImport)
                    break;
            }
        }
        if (insertLine > lines.length)
            insertLine = lines.length;
        action.edit = new vscode.WorkspaceEdit();
        action.edit.insert(document.uri, new vscode.Position(insertLine, 0), `import ${modName}\n`);
        return action;
    }
}
exports.SkylangCodeActionProvider = SkylangCodeActionProvider;
//# sourceMappingURL=codeActions.js.map