import * as vscode from 'vscode';

export class SkylangCodeLensProvider implements vscode.CodeLensProvider {
    public provideCodeLenses(
        document: vscode.TextDocument,
        _token: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.CodeLens[]> {
        const lenses: vscode.CodeLens[] = [];
        const topRange = new vscode.Range(0, 0, 0, 0);

        lenses.push(new vscode.CodeLens(topRange, {
            title: '$(play) Run',
            command: 'skylang.run',
            arguments: [document.uri],
            tooltip: 'Run Skylang program'
        }));

        return lenses;
    }
}
