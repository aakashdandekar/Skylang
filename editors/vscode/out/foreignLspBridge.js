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
exports.ForeignLspBridge = exports.ForeignVirtualDocumentProvider = void 0;
const vscode = __importStar(require("vscode"));
/**
 * Virtual Text Document Content Provider for Foreign Language Bridge Documents.
 * Serves synthetic shadow documents to VS Code's internal language servers (Pylance, TS Server, Clangd, gopls, rust-analyzer, JDT LS).
 */
class ForeignVirtualDocumentProvider {
    static scheme = 'skylang-foreign';
    _documents = new Map();
    _onDidChange = new vscode.EventEmitter();
    onDidChange = this._onDidChange.event;
    setDocument(uri, content) {
        this._documents.set(uri.toString(), content);
        this._onDidChange.fire(uri);
    }
    clearDocument(uri) {
        this._documents.delete(uri.toString());
    }
    provideTextDocumentContent(uri) {
        return this._documents.get(uri.toString()) || '';
    }
}
exports.ForeignVirtualDocumentProvider = ForeignVirtualDocumentProvider;
/**
 * Universal Foreign Language Extension IntelliSense Bridge.
 * Connects Skylang's editor with actual installed VS Code extensions for Python, JavaScript/Node.js, C/C++, Golang, Rust, and Java.
 */
class ForeignLspBridge {
    static instance;
    virtualProvider;
    docVersionCounter = 0;
    statusBarItem;
    // Known official extensions for each language ecosystem
    static LANGUAGE_EXTENSIONS = {
        python: [
            'ms-python.vscode-pylance',
            'ms-python.python',
            'charliermarsh.ruff',
            'ms-python.black-formatter'
        ],
        js: [
            'vscode.typescript-language-features',
            'dbaeumer.vscode-eslint',
            'denoland.vscode-deno'
        ],
        cpp: [
            'ms-vscode.cpptools',
            'llvm-vs-code-extensions.vscode-clangd',
            'mitaki28.vscode-clang'
        ],
        golang: [
            'golang.go'
        ],
        rust: [
            'rust-lang.rust-analyzer',
            'matklad.rust-analyzer'
        ],
        java: [
            'redhat.java',
            'vscjava.vscode-java-pack',
            'vscjava.vscode-java-core'
        ]
    };
    static LANGUAGE_FILE_EXTENSIONS = {
        python: 'py',
        js: 'js',
        cpp: 'cpp',
        golang: 'go',
        rust: 'rs',
        java: 'java'
    };
    static LANGUAGE_DISPLAY_NAMES = {
        python: 'Python (Pylance/Python)',
        js: 'JavaScript / Node.js (TypeScript Server)',
        cpp: 'C/C++ (Clangd/CppTools)',
        golang: 'Golang (gopls)',
        rust: 'Rust (rust-analyzer)',
        java: 'Java (Red Hat JDTLS)'
    };
    constructor(virtualProvider) {
        this.virtualProvider = virtualProvider;
    }
    static initialize(context) {
        const virtualProvider = new ForeignVirtualDocumentProvider();
        context.subscriptions.push(vscode.workspace.registerTextDocumentContentProvider(ForeignVirtualDocumentProvider.scheme, virtualProvider));
        ForeignLspBridge.instance = new ForeignLspBridge(virtualProvider);
        ForeignLspBridge.instance.setupStatusBar(context);
        return ForeignLspBridge.instance;
    }
    static getInstance() {
        return ForeignLspBridge.instance;
    }
    /**
     * Checks if an official language extension for a given bridge is installed and active in VS Code.
     */
    isLanguageExtensionAvailable(bridge) {
        const extIds = ForeignLspBridge.LANGUAGE_EXTENSIONS[bridge] || [];
        return extIds.some(id => {
            const ext = vscode.extensions.getExtension(id);
            return ext !== undefined;
        });
    }
    /**
     * Returns metadata on all supported language extensions and their installation status.
     */
    getLanguageExtensionStatuses() {
        const statuses = [];
        for (const [bridgeStr, extIds] of Object.entries(ForeignLspBridge.LANGUAGE_EXTENSIONS)) {
            const bridge = bridgeStr;
            let installed = false;
            let activeId = undefined;
            for (const id of extIds) {
                const ext = vscode.extensions.getExtension(id);
                if (ext) {
                    installed = true;
                    activeId = id;
                    break;
                }
            }
            statuses.push({
                id: activeId || extIds[0],
                name: ForeignLspBridge.LANGUAGE_DISPLAY_NAMES[bridge],
                publisher: extIds[0].split('.')[0],
                language: bridge,
                recommendedExtensionIds: extIds,
                isInstalled: installed,
                activeExtensionId: activeId
            });
        }
        return statuses;
    }
    /**
     * Synthesizes a shadow document in the foreign language corresponding to a bridge variable or expression.
     */
    createShadowDocumentForMember(bridge, packageName, memberPrefix = '') {
        const fileExt = ForeignLspBridge.LANGUAGE_FILE_EXTENSIONS[bridge];
        const docId = `shadow_${bridge}_${Date.now()}_${++this.docVersionCounter}.${fileExt}`;
        const uri = vscode.Uri.parse(`${ForeignVirtualDocumentProvider.scheme}:///${docId}`);
        let content = '';
        let targetLine = 0;
        let targetCol = 0;
        switch (bridge) {
            case 'python': {
                // e.g. import numpy as np\nnp.
                const varName = packageName.replace(/[^a-zA-Z0-9_]/g, '_');
                content = `import ${packageName} as ${varName}\n${varName}.${memberPrefix}`;
                targetLine = 1;
                targetCol = `${varName}.${memberPrefix}`.length;
                break;
            }
            case 'js': {
                // e.g. const express = require('express');\nexpress.
                const varName = packageName.replace(/[^a-zA-Z0-9_]/g, '_');
                content = `const ${varName} = require('${packageName}');\n${varName}.${memberPrefix}`;
                targetLine = 1;
                targetCol = `${varName}.${memberPrefix}`.length;
                break;
            }
            case 'cpp': {
                // e.g. #include <math.h>\n
                const header = packageName.endsWith('.h') || packageName.endsWith('.hpp') ? packageName : `${packageName}.h`;
                content = `#include <${header}>\nvoid _skylang_bridge_test() {\n    ${memberPrefix}\n}`;
                targetLine = 2;
                targetCol = `    ${memberPrefix}`.length;
                break;
            }
            case 'golang': {
                // e.g. package main\nimport "fmt"\nfunc _() { fmt. }
                content = `package main\nimport "${packageName}"\nfunc _() {\n    ${packageName}.${memberPrefix}\n}`;
                targetLine = 3;
                targetCol = `    ${packageName}.${memberPrefix}`.length;
                break;
            }
            case 'rust': {
                // e.g. use std::f64::consts::*;\nfn main() {  }
                content = `use ${packageName}::*;\nfn _skylang_shadow() {\n    ${memberPrefix}\n}`;
                targetLine = 2;
                targetCol = `    ${memberPrefix}`.length;
                break;
            }
            case 'java': {
                // e.g. class Main { void _() { java.lang.Math. } }
                content = `class SkylangShadowBridge {\n    void _() {\n        ${packageName}.${memberPrefix}\n    }\n}`;
                targetLine = 2;
                targetCol = `        ${packageName}.${memberPrefix}`.length;
                break;
            }
        }
        this.virtualProvider.setDocument(uri, content);
        return {
            uri,
            position: new vscode.Position(targetLine, targetCol),
            languageId: bridge === 'js' ? 'javascript' : (bridge === 'golang' ? 'go' : bridge),
            bridgeType: bridge
        };
    }
    /**
     * Synthesizes a shadow document for an embedded code block (e.g. inside python.exec("..."), cpp.compile("..."), etc.).
     */
    createShadowDocumentForEmbeddedCode(bridge, embeddedCode, cursorOffsetInCode) {
        const fileExt = ForeignLspBridge.LANGUAGE_FILE_EXTENSIONS[bridge];
        const docId = `embedded_${bridge}_${Date.now()}_${++this.docVersionCounter}.${fileExt}`;
        const uri = vscode.Uri.parse(`${ForeignVirtualDocumentProvider.scheme}:///${docId}`);
        this.virtualProvider.setDocument(uri, embeddedCode);
        // Compute line and column from offset
        const prefix = embeddedCode.substring(0, Math.min(cursorOffsetInCode, embeddedCode.length));
        const lines = prefix.split('\n');
        const line = lines.length - 1;
        const col = lines[lines.length - 1].length;
        return {
            uri,
            position: new vscode.Position(line, col),
            languageId: bridge === 'js' ? 'javascript' : bridge,
            bridgeType: bridge
        };
    }
    /**
     * Executes completion provider on the virtual document via VS Code's internal LSP delegation.
     */
    async queryCompletions(shadowDoc, triggerChar) {
        try {
            // Open the virtual document in VS Code's document manager
            await vscode.workspace.openTextDocument(shadowDoc.uri);
            const results = await vscode.commands.executeCommand('vscode.executeCompletionItemProvider', shadowDoc.uri, shadowDoc.position, triggerChar);
            if (!results)
                return [];
            const rawItems = Array.isArray(results) ? results : results.items;
            const badge = `[${ForeignLspBridge.LANGUAGE_DISPLAY_NAMES[shadowDoc.bridgeType].split(' ')[0]} LSP]`;
            return rawItems.map(item => {
                const cloned = new vscode.CompletionItem(item.label, item.kind);
                cloned.detail = item.detail ? `${badge} ${item.detail}` : badge;
                cloned.documentation = item.documentation;
                cloned.insertText = item.insertText;
                cloned.sortText = item.sortText || `0_${typeof item.label === 'string' ? item.label : item.label.label}`;
                return cloned;
            });
        }
        catch {
            return [];
        }
        finally {
            this.virtualProvider.clearDocument(shadowDoc.uri);
        }
    }
    /**
     * Executes hover provider on the virtual document via VS Code's internal LSP delegation.
     */
    async queryHover(shadowDoc) {
        try {
            await vscode.workspace.openTextDocument(shadowDoc.uri);
            const hovers = await vscode.commands.executeCommand('vscode.executeHoverProvider', shadowDoc.uri, shadowDoc.position);
            if (!hovers || hovers.length === 0)
                return null;
            const firstHover = hovers[0];
            const badge = `**[${ForeignLspBridge.LANGUAGE_DISPLAY_NAMES[shadowDoc.bridgeType]} IntelliSense]**\n\n`;
            const enhancedContents = firstHover.contents.map(c => {
                if (typeof c === 'string') {
                    return new vscode.MarkdownString(badge + c);
                }
                else if (c instanceof vscode.MarkdownString) {
                    const md = new vscode.MarkdownString(badge + c.value);
                    md.isTrusted = c.isTrusted;
                    return md;
                }
                return c;
            });
            return new vscode.Hover(enhancedContents);
        }
        catch {
            return null;
        }
        finally {
            this.virtualProvider.clearDocument(shadowDoc.uri);
        }
    }
    /**
     * Executes signature help provider on the virtual document via VS Code's internal LSP delegation.
     */
    async querySignatureHelp(shadowDoc, triggerChar = '(') {
        try {
            await vscode.workspace.openTextDocument(shadowDoc.uri);
            const help = await vscode.commands.executeCommand('vscode.executeSignatureHelpProvider', shadowDoc.uri, shadowDoc.position, triggerChar);
            return help || null;
        }
        catch {
            return null;
        }
        finally {
            this.virtualProvider.clearDocument(shadowDoc.uri);
        }
    }
    /**
     * Executes definition provider on the virtual document via VS Code's internal LSP delegation.
     */
    async queryDefinition(shadowDoc) {
        try {
            await vscode.workspace.openTextDocument(shadowDoc.uri);
            const defs = await vscode.commands.executeCommand('vscode.executeDefinitionProvider', shadowDoc.uri, shadowDoc.position);
            return defs || null;
        }
        catch {
            return null;
        }
        finally {
            this.virtualProvider.clearDocument(shadowDoc.uri);
        }
    }
    /**
     * Configures the status bar item showing active foreign IntelliSense bridges.
     */
    setupStatusBar(context) {
        this.statusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 90);
        this.statusBarItem.command = 'skylang.checkForeignExtensions';
        this.updateStatusBar();
        this.statusBarItem.show();
        context.subscriptions.push(this.statusBarItem);
        // Register QuickPick command to inspect and install language extensions
        context.subscriptions.push(vscode.commands.registerCommand('skylang.checkForeignExtensions', () => {
            this.showExtensionStatusQuickPick();
        }));
    }
    updateStatusBar() {
        if (!this.statusBarItem)
            return;
        const statuses = this.getLanguageExtensionStatuses();
        const activeCount = statuses.filter(s => s.isInstalled).length;
        const totalCount = statuses.length;
        this.statusBarItem.text = `$(globe) Skylang Foreign LSP: ${activeCount}/${totalCount}`;
        this.statusBarItem.tooltip = new vscode.MarkdownString(`### Skylang Universal Foreign Language IntelliSense\n\n` +
            `Directly connected to **${activeCount} of ${totalCount}** official VS Code language extensions:\n\n` +
            statuses.map(s => `- **${s.name}**: ${s.isInstalled ? '✅ Connected (`' + s.activeExtensionId + '`)' : '⚠️ Not installed (Click to install)'}`).join('\n') +
            `\n\n*Click to inspect or install language extensions.*`);
    }
    /**
     * Shows an interactive QuickPick modal allowing developers to see status and install language extensions.
     */
    async showExtensionStatusQuickPick() {
        const statuses = this.getLanguageExtensionStatuses();
        const items = statuses.map(s => {
            const label = `${s.isInstalled ? '$(check) ' : '$(cloud-download) '} ${s.name}`;
            const description = s.isInstalled ? `Active: ${s.activeExtensionId}` : `Recommended: ${s.recommendedExtensionIds[0]}`;
            const detail = s.isInstalled
                ? `Provides live completions, hovers, and signature help from official ${s.name} language server.`
                : `Install ${s.recommendedExtensionIds[0]} from VS Code Marketplace to enable full real-time language server IntelliSense.`;
            return {
                label,
                description,
                detail,
                buttons: s.isInstalled ? [] : [{
                        iconPath: new vscode.ThemeIcon('cloud-download'),
                        tooltip: `Install ${s.recommendedExtensionIds[0]}`
                    }]
            };
        });
        const selected = await vscode.window.showQuickPick(items, {
            title: 'Skylang Foreign Language Extension Bridges',
            placeHolder: 'Select a language to view or install its official VS Code extension'
        });
        if (selected) {
            const matchingStatus = statuses.find(s => selected.label.includes(s.name));
            if (matchingStatus && !matchingStatus.isInstalled) {
                const extToInstall = matchingStatus.recommendedExtensionIds[0];
                const action = await vscode.window.showInformationMessage(`Would you like to install the official extension for ${matchingStatus.name} (${extToInstall})?`, 'Install Extension', 'Cancel');
                if (action === 'Install Extension') {
                    await vscode.commands.executeCommand('workbench.extensions.installExtension', extToInstall);
                    vscode.window.showInformationMessage(`Installing ${extToInstall}... Foreign IntelliSense will activate automatically once ready.`);
                    this.updateStatusBar();
                }
            }
        }
    }
}
exports.ForeignLspBridge = ForeignLspBridge;
//# sourceMappingURL=foreignLspBridge.js.map