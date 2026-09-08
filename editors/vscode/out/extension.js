"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.activate = activate;
exports.deactivate = deactivate;
const vscode = require("vscode");
const completions_1 = require("./completions");
const hover_1 = require("./hover");
const signatureHelp_1 = require("./signatureHelp");
const definitions_1 = require("./definitions");
const symbols_1 = require("./symbols");
const formatter_1 = require("./formatter");
const diagnostics_1 = require("./diagnostics");
const codeActions_1 = require("./codeActions");
const codelens_1 = require("./codelens");
const runner_1 = require("./runner");
function activate(context) {
    const SKYLANG_MODE = { language: 'skylang' };
    // 0. Code Actions Provider (Auto-Import QuickFix)
    context.subscriptions.push(vscode.languages.registerCodeActionsProvider(SKYLANG_MODE, new codeActions_1.SkylangCodeActionProvider(), {
        providedCodeActionKinds: codeActions_1.SkylangCodeActionProvider.providedCodeActionKinds
    }));
    // 1. Completion Provider
    context.subscriptions.push(vscode.languages.registerCompletionItemProvider(SKYLANG_MODE, new completions_1.SkylangCompletionItemProvider(), '.', '(', ' ', ',', '='));
    // 2. Hover Provider
    context.subscriptions.push(vscode.languages.registerHoverProvider(SKYLANG_MODE, new hover_1.SkylangHoverProvider()));
    // 3. Signature Help Provider
    context.subscriptions.push(vscode.languages.registerSignatureHelpProvider(SKYLANG_MODE, new signatureHelp_1.SkylangSignatureHelpProvider(), '(', ','));
    // 4. Definition Provider (Go to Definition)
    context.subscriptions.push(vscode.languages.registerDefinitionProvider(SKYLANG_MODE, new definitions_1.SkylangDefinitionProvider()));
    // 5. Document and Workspace Symbol Providers
    context.subscriptions.push(vscode.languages.registerDocumentSymbolProvider(SKYLANG_MODE, new symbols_1.SkylangDocumentSymbolProvider()));
    context.subscriptions.push(vscode.languages.registerWorkspaceSymbolProvider(new symbols_1.SkylangWorkspaceSymbolProvider()));
    // 6. Formatting Provider
    context.subscriptions.push(vscode.languages.registerDocumentFormattingEditProvider(SKYLANG_MODE, new formatter_1.SkylangDocumentFormattingProvider()));
    // 7. CodeLens Provider
    context.subscriptions.push(vscode.languages.registerCodeLensProvider(SKYLANG_MODE, new codelens_1.SkylangCodeLensProvider()));
    // 8. Diagnostics Provider (Linter)
    const diagnosticsProvider = new diagnostics_1.SkylangDiagnosticsProvider();
    context.subscriptions.push(diagnosticsProvider.getCollection());
    if (vscode.window.activeTextEditor) {
        diagnosticsProvider.refreshDiagnostics(vscode.window.activeTextEditor.document);
    }
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(editor => {
        if (editor) {
            diagnosticsProvider.refreshDiagnostics(editor.document);
        }
    }));
    context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(event => {
        diagnosticsProvider.refreshDiagnostics(event.document);
    }));
    context.subscriptions.push(vscode.workspace.onDidCloseTextDocument(doc => {
        diagnosticsProvider.clear(doc);
    }));
    // 9. Commands Registration
    context.subscriptions.push(vscode.commands.registerCommand('skylang.run', (uri) => {
        runner_1.SkylangRunner.runFile(uri);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('skylang.runVM', (uri) => {
        runner_1.SkylangRunner.runVM(uri);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('skylang.profile', (uri) => {
        runner_1.SkylangRunner.profileVM(uri);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('skylang.build', (uri) => {
        runner_1.SkylangRunner.buildFile(uri);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('skylang.repl', () => {
        runner_1.SkylangRunner.startRepl();
    }));
    context.subscriptions.push(vscode.commands.registerCommand('skylang.formatDocument', () => {
        vscode.commands.executeCommand('editor.action.formatDocument');
    }));
}
function deactivate() { }
//# sourceMappingURL=extension.js.map