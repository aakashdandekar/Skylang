import * as vscode from 'vscode';

export class SkylangDocumentFormattingProvider implements vscode.DocumentFormattingEditProvider {
    public provideDocumentFormattingEdits(
        document: vscode.TextDocument,
        options: vscode.FormattingOptions,
        _token: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.TextEdit[]> {
        const edits: vscode.TextEdit[] = [];
        const lineCount = document.lineCount;
        const tabSize = options.tabSize || 4;
        const indentUnit = options.insertSpaces ? ' '.repeat(tabSize) : '\t';

        let indentLevel = 0;
        const formattedLines: string[] = [];

        for (let i = 0; i < lineCount; i++) {
            const rawLine = document.lineAt(i).text;
            const trimmed = rawLine.trim();

            if (trimmed.length === 0) {
                formattedLines.push('');
                continue;
            }

            // If line starts with closing brace, decrement indent before applying
            let leadingCloses = 0;
            for (let j = 0; j < trimmed.length; j++) {
                if (trimmed[j] === '}') leadingCloses++;
                else break;
            }

            const currentIndent = Math.max(0, indentLevel - leadingCloses);
            const formattedLine = indentUnit.repeat(currentIndent) + this.formatLineContent(trimmed);

            formattedLines.push(formattedLine);

            // Update indent level for subsequent lines based on braces
            const braceDelta = this.calculateBraceDelta(trimmed);
            indentLevel = Math.max(0, indentLevel + braceDelta);
        }

        const fullRange = new vscode.Range(
            new vscode.Position(0, 0),
            new vscode.Position(lineCount - 1, document.lineAt(lineCount - 1).text.length)
        );

        edits.push(vscode.TextEdit.replace(fullRange, formattedLines.join('\n')));
        return edits;
    }

    private formatLineContent(line: string): string {
        // Leave pure comment lines untouched
        if (line.startsWith('//') || line.startsWith('#') || line.startsWith('/*')) {
            return line;
        }

        // Normalize spaces around commas
        let res = line.replace(/\s*,\s*/g, ', ');

        // Normalize space after keywords: if, elif, for, class, f, import, from, as, cimport, extern, return
        res = res.replace(/\b(if|elif|for|class|f|import|from|as|cimport|extern|return)\s+/g, '$1 ');

        // Normalize space before opening brace `{`
        res = res.replace(/\s*\{/g, ' {');

        // Normalize compound assignment operators
        res = res.replace(/\s*(\:\=|\+\=|\-\=|\*\=|\/\=|\=\=|\!\=|\<\=|\>\=)\s*/g, ' $1 ');

        return res;
    }

    private calculateBraceDelta(line: string): number {
        let delta = 0;
        let inString = false;
        let quoteChar = '';

        for (let i = 0; i < line.length; i++) {
            const ch = line[i];
            const prev = i > 0 ? line[i - 1] : '';

            // Ignore line comments
            if (!inString && ch === '/' && i + 1 < line.length && line[i + 1] === '/') {
                break;
            }
            if (!inString && ch === '#') {
                break;
            }

            if (inString) {
                if (ch === quoteChar && prev !== '\\') {
                    inString = false;
                }
                continue;
            }

            if (ch === '"' || ch === '\'') {
                inString = true;
                quoteChar = ch;
                continue;
            }

            if (ch === '{') delta++;
            else if (ch === '}') delta--;
        }
        return delta;
    }
}
