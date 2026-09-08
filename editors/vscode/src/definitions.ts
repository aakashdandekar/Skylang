import * as vscode from 'vscode';

export class SkylangDefinitionProvider implements vscode.DefinitionProvider {
    public provideDefinition(
        document: vscode.TextDocument,
        position: vscode.Position,
        _token: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.Definition | vscode.LocationLink[]> {
        const wordRange = document.getWordRangeAtPosition(position, /[a-zA-Z0-9_]+/);
        if (!wordRange) return null;

        const word = document.getText(wordRange);
        const lines = document.getText().split('\n');

        // Check each line for the definition of `word`
        for (let i = 0; i < lines.length; i++) {
            const rawLine = lines[i];
            const line = rawLine.replace(/\/\/.*$/, '').replace(/#.*$/, '').trim();

            // 1. Function definition: `f word`
            const fnMatch = line.match(new RegExp(`^f\\s+${word}\\b`));
            if (fnMatch) {
                const col = rawLine.indexOf(word);
                return new vscode.Location(document.uri, new vscode.Position(i, col));
            }

            // 2. Class definition: `class word`
            const classMatch = line.match(new RegExp(`^class\\s+${word}\\b`));
            if (classMatch) {
                const col = rawLine.indexOf(word);
                return new vscode.Location(document.uri, new vscode.Position(i, col));
            }

            // 3. Class Method definition: `word {`
            const methodMatch = line.match(new RegExp(`^${word}\\s*\\{`));
            if (
                methodMatch &&
                word !== 'if' &&
                word !== 'for' &&
                word !== 'elif' &&
                word !== 'else' &&
                word !== 'init'
            ) {
                const col = rawLine.indexOf(word);
                return new vscode.Location(document.uri, new vscode.Position(i, col));
            }

            // 4. Class field: `this.word`
            const fieldMatch = line.match(new RegExp(`^this\\.${word}\\b`));
            if (fieldMatch) {
                const col = rawLine.indexOf(word);
                return new vscode.Location(document.uri, new vscode.Position(i, col));
            }

            // 5. Extern C function: `extern f word(...)`
            const externMatch = line.match(new RegExp(`^extern\\s+f\\s+${word}\\b`));
            if (externMatch) {
                const col = rawLine.indexOf(word);
                return new vscode.Location(document.uri, new vscode.Position(i, col));
            }

            // 6. Walrus variable declaration: `word :=` or `a, word :=`
            const walrusMatch = line.match(new RegExp(`\\b${word}\\b.*:=`));
            if (walrusMatch) {
                const col = rawLine.indexOf(word);
                return new vscode.Location(document.uri, new vscode.Position(i, col));
            }

            // 7. Typed scalar or collection declaration: `I word` or `word L` or `numbers I 5`
            const typedMatch = line.match(
                new RegExp(
                    `(?:(?:I|D|B|C|S)\\s+\\b${word}\\b|\\b${word}\\b\\s+(?:L|T|DICT|SET|SL|I|D|B|C|S))`
                )
            );
            if (typedMatch) {
                const col = rawLine.indexOf(word);
                return new vscode.Location(document.uri, new vscode.Position(i, col));
            }

            // 8. Parameter in takes(...): `takes(..., word, ...)`
            const takesMatch = line.match(/takes\s*\(([^)]*)\)/);
            if (takesMatch) {
                const params = takesMatch[1].split(',').map(p => p.trim());
                if (params.includes(word)) {
                    const col = rawLine.indexOf(word);
                    return new vscode.Location(document.uri, new vscode.Position(i, col));
                }
            }

            // 9. Loop iterator: `for word in ...`
            const forMatch = line.match(new RegExp(`for\\s+\\b${word}\\b\\s+in\\b`));
            if (forMatch) {
                const col = rawLine.indexOf(word);
                return new vscode.Location(document.uri, new vscode.Position(i, col));
            }

            // 10. Module import: `import word`
            const importMatch = line.match(new RegExp(`^import\\s+\\b${word}\\b`));
            if (importMatch) {
                const col = rawLine.indexOf(word);
                return new vscode.Location(document.uri, new vscode.Position(i, col));
            }
        }

        return null;
    }
}
