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
exports.activate = activate;
exports.deactivate = deactivate;
const vscode = __importStar(require("vscode"));
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
const foreignLspBridge_1 = require("./foreignLspBridge");
function activate(context) {
    const SKYLANG_MODE = { language: 'skylang' };
    // Initialize Universal Foreign Language Extension IntelliSense Bridge
    foreignLspBridge_1.ForeignLspBridge.initialize(context);
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
    context.subscriptions.push(vscode.commands.registerCommand('skylang.build', (uri) => {
        runner_1.SkylangRunner.buildFile(uri);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('skylang.formatDocument', () => {
        vscode.commands.executeCommand('editor.action.formatDocument');
    }));
}
function deactivate() { }
//# sourceMappingURL=extension.js.map