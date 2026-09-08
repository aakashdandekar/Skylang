import * as vscode from 'vscode';
import { SkylangCompletionItemProvider } from './completions';
import { SkylangHoverProvider } from './hover';
import { SkylangSignatureHelpProvider } from './signatureHelp';
import { SkylangDefinitionProvider } from './definitions';
import { SkylangDocumentSymbolProvider, SkylangWorkspaceSymbolProvider } from './symbols';
import { SkylangDocumentFormattingProvider } from './formatter';
import { SkylangDiagnosticsProvider } from './diagnostics';
import { SkylangCodeActionProvider } from './codeActions';
import { SkylangCodeLensProvider } from './codelens';
import { SkylangRunner } from './runner';

export function activate(context: vscode.ExtensionContext) {
    const SKYLANG_MODE: vscode.DocumentSelector = { language: 'skylang' };

    // 0. Code Actions Provider (Auto-Import QuickFix)
    context.subscriptions.push(
        vscode.languages.registerCodeActionsProvider(
            SKYLANG_MODE,
            new SkylangCodeActionProvider(),
            {
                providedCodeActionKinds: SkylangCodeActionProvider.providedCodeActionKinds
            }
        )
    );

    // 1. Completion Provider
    context.subscriptions.push(
        vscode.languages.registerCompletionItemProvider(
            SKYLANG_MODE,
            new SkylangCompletionItemProvider(),
            '.', '(', ' ', ',', '='
        )
    );

    // 2. Hover Provider
    context.subscriptions.push(
        vscode.languages.registerHoverProvider(
            SKYLANG_MODE,
            new SkylangHoverProvider()
        )
    );

    // 3. Signature Help Provider
    context.subscriptions.push(
        vscode.languages.registerSignatureHelpProvider(
            SKYLANG_MODE,
            new SkylangSignatureHelpProvider(),
            '(', ','
        )
    );

    // 4. Definition Provider (Go to Definition)
    context.subscriptions.push(
        vscode.languages.registerDefinitionProvider(
            SKYLANG_MODE,
            new SkylangDefinitionProvider()
        )
    );

    // 5. Document and Workspace Symbol Providers
    context.subscriptions.push(
        vscode.languages.registerDocumentSymbolProvider(
            SKYLANG_MODE,
            new SkylangDocumentSymbolProvider()
        )
    );
    context.subscriptions.push(
        vscode.languages.registerWorkspaceSymbolProvider(
            new SkylangWorkspaceSymbolProvider()
        )
    );

    // 6. Formatting Provider
    context.subscriptions.push(
        vscode.languages.registerDocumentFormattingEditProvider(
            SKYLANG_MODE,
            new SkylangDocumentFormattingProvider()
        )
    );

    // 7. CodeLens Provider
    context.subscriptions.push(
        vscode.languages.registerCodeLensProvider(
            SKYLANG_MODE,
            new SkylangCodeLensProvider()
        )
    );

    // 8. Diagnostics Provider (Linter)
    const diagnosticsProvider = new SkylangDiagnosticsProvider();
    context.subscriptions.push(diagnosticsProvider.getCollection());

    if (vscode.window.activeTextEditor) {
        diagnosticsProvider.refreshDiagnostics(vscode.window.activeTextEditor.document);
    }

    context.subscriptions.push(
        vscode.window.onDidChangeActiveTextEditor(editor => {
            if (editor) {
                diagnosticsProvider.refreshDiagnostics(editor.document);
            }
        })
    );

    context.subscriptions.push(
        vscode.workspace.onDidChangeTextDocument(event => {
            diagnosticsProvider.refreshDiagnostics(event.document);
        })
    );

    context.subscriptions.push(
        vscode.workspace.onDidCloseTextDocument(doc => {
            diagnosticsProvider.clear(doc);
        })
    );

    // 9. Commands Registration
    context.subscriptions.push(
        vscode.commands.registerCommand('skylang.run', (uri?: vscode.Uri) => {
            SkylangRunner.runFile(uri);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('skylang.runVM', (uri?: vscode.Uri) => {
            SkylangRunner.runVM(uri);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('skylang.profile', (uri?: vscode.Uri) => {
            SkylangRunner.profileVM(uri);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('skylang.build', (uri?: vscode.Uri) => {
            SkylangRunner.buildFile(uri);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('skylang.repl', () => {
            SkylangRunner.startRepl();
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('skylang.formatDocument', () => {
            vscode.commands.executeCommand('editor.action.formatDocument');
        })
    );
}

export function deactivate() {}
