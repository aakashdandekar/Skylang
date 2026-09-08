import * as vscode from 'vscode';

export class SkylangCodeActionProvider implements vscode.CodeActionProvider {
    public static readonly providedCodeActionKinds = [
        vscode.CodeActionKind.QuickFix
    ];

    public provideCodeActions(
        document: vscode.TextDocument,
        range: vscode.Range | vscode.Selection,
        context: vscode.CodeActionContext,
        _token: vscode.CancellationToken
    ): vscode.CodeAction[] {
        const actions: vscode.CodeAction[] = [];
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
                    if (action) actions.push(action);
                }
            } else if (diagnostic.code === 'skylang-invalid-import-syntax') {
                const targetText = document.getText(diagnostic.range);
                if (targetText.includes('.')) {
                    const fixed = targetText
                        .split('.')
                        .map(s => s.trim())
                        .filter(Boolean)
                        .join(', ');
                    const action = new vscode.CodeAction(
                        `Convert '${targetText}' to comma-separated 'import ${fixed}'`,
                        vscode.CodeActionKind.QuickFix
                    );
                    action.edit = new vscode.WorkspaceEdit();
                    action.edit.replace(document.uri, diagnostic.range, fixed);
                    action.diagnostics = [diagnostic];
                    action.isPreferred = true;
                    actions.push(action);
                }
            } else if (diagnostic.code === 'skylang-invalid-import-module') {
                const targetText = document.getText(diagnostic.range);
                if (targetText === 'fmt') {
                    const lineIdx = diagnostic.range.start.line;
                    const action = new vscode.CodeAction(
                        `Replace with 'import go' and load 'fmt' via 'fmt := go.load("fmt")'`,
                        vscode.CodeActionKind.QuickFix
                    );
                    action.edit = new vscode.WorkspaceEdit();
                    const lineRange = document.lineAt(lineIdx).range;
                    action.edit.replace(document.uri, lineRange, `import go\nfmt := go.load("fmt")`);
                    action.diagnostics = [diagnostic];
                    action.isPreferred = true;
                    actions.push(action);
                }
            } else if (diagnostic.code === 'skylang-unnecessary-import') {
                const lineIdx = diagnostic.range.start.line;
                const lineText = document.lineAt(lineIdx).text.trim();
                const action = new vscode.CodeAction(
                    `Remove unnecessary import statement`,
                    vscode.CodeActionKind.QuickFix
                );
                action.edit = new vscode.WorkspaceEdit();
                if (lineText.startsWith('import ') && !lineText.includes(',')) {
                    action.edit.delete(document.uri, document.lineAt(lineIdx).rangeIncludingLineBreak);
                } else {
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
                        if (action) actions.push(action);
                    }
                }
            }
        }

        // 3. Known Python package reference QuickFix: numpy.array(...) -> import python and load
        const pythonPkgs = ['numpy', 'pandas', 'torch', 'scipy', 'requests', 'matplotlib', 'sklearn', 'fastapi', 'flask', 'django', 'cv2', 'PIL', 'polars'];
        for (const pkg of pythonPkgs) {
            const pkgRegex = new RegExp(`\\b${pkg}\\.`);
            if (pkgRegex.test(line)) {
                const action = new vscode.CodeAction(
                    `Import 'python' and load package '${pkg}'`,
                    vscode.CodeActionKind.QuickFix
                );
                action.edit = new vscode.WorkspaceEdit();
                const lines = text.split('\n');
                let insertLine = 0;
                let foundImport = false;
                for (let i = 0; i < lines.length; i++) {
                    const l = lines[i].trim();
                    if (l.startsWith('import ') || l.startsWith('cimport ')) {
                        insertLine = i + 1;
                        foundImport = true;
                    } else if (!foundImport && (l.startsWith('//') || l.startsWith('#') || l === '')) {
                        insertLine = i + 1;
                    } else {
                        if (foundImport) break;
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

    private createImportAction(
        document: vscode.TextDocument,
        modName: string,
        diagnostic?: vscode.Diagnostic,
        customTitle?: string
    ): vscode.CodeAction | null {
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
            } else if (!foundExistingImport && (l.startsWith('//') || l.startsWith('#') || l === '')) {
                insertLine = i + 1;
            } else {
                if (foundExistingImport) break;
            }
        }

        if (insertLine > lines.length) insertLine = lines.length;

        action.edit = new vscode.WorkspaceEdit();
        action.edit.insert(document.uri, new vscode.Position(insertLine, 0), `import ${modName}\n`);
        return action;
    }
}
