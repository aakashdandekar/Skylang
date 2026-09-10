import * as vscode from 'vscode';

export type ForeignBridgeType = 'python' | 'js' | 'cpp' | 'golang' | 'rust' | 'java';

export interface LanguageExtensionInfo {
    id: string;
    name: string;
    publisher: string;
    language: ForeignBridgeType;
    recommendedExtensionIds: string[];
    isInstalled: boolean;
    activeExtensionId?: string;
}

export interface ShadowDocumentResult {
    uri: vscode.Uri;
    position: vscode.Position;
    languageId: string;
    bridgeType: ForeignBridgeType;
}

/**
 * Virtual Text Document Content Provider for Foreign Language Bridge Documents.
 * Serves synthetic shadow documents to VS Code's internal language servers (Pylance, TS Server, Clangd, gopls, rust-analyzer, JDT LS).
 */
export class ForeignVirtualDocumentProvider implements vscode.TextDocumentContentProvider {
    public static readonly scheme = 'skylang-foreign';
    private _documents = new Map<string, string>();
    private _onDidChange = new vscode.EventEmitter<vscode.Uri>();

    public readonly onDidChange = this._onDidChange.event;

    public setDocument(uri: vscode.Uri, content: string): void {
        this._documents.set(uri.toString(), content);
        this._onDidChange.fire(uri);
    }

    public clearDocument(uri: vscode.Uri): void {
        this._documents.delete(uri.toString());
    }

    public provideTextDocumentContent(uri: vscode.Uri): string {
        return this._documents.get(uri.toString()) || '';
    }
}

/**
 * Universal Foreign Language Extension IntelliSense Bridge.
 * Connects Skylang's editor with actual installed VS Code extensions for Python, JavaScript/Node.js, C/C++, Golang, Rust, and Java.
 */
export class ForeignLspBridge {
    private static instance: ForeignLspBridge;
    private virtualProvider: ForeignVirtualDocumentProvider;
    private docVersionCounter = 0;
    private statusBarItem?: vscode.StatusBarItem;

    // Known official extensions for each language ecosystem
    public static readonly LANGUAGE_EXTENSIONS: Record<ForeignBridgeType, string[]> = {
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

    public static readonly LANGUAGE_FILE_EXTENSIONS: Record<ForeignBridgeType, string> = {
        python: 'py',
        js: 'js',
        cpp: 'cpp',
        golang: 'go',
        rust: 'rs',
        java: 'java'
    };

    public static readonly LANGUAGE_DISPLAY_NAMES: Record<ForeignBridgeType, string> = {
        python: 'Python (Pylance/Python)',
        js: 'JavaScript / Node.js (TypeScript Server)',
        cpp: 'C/C++ (Clangd/CppTools)',
        golang: 'Golang (gopls)',
        rust: 'Rust (rust-analyzer)',
        java: 'Java (Red Hat JDTLS)'
    };

    constructor(virtualProvider: ForeignVirtualDocumentProvider) {
        this.virtualProvider = virtualProvider;
    }

    public static initialize(context: vscode.ExtensionContext): ForeignLspBridge {
        const virtualProvider = new ForeignVirtualDocumentProvider();
        context.subscriptions.push(
            vscode.workspace.registerTextDocumentContentProvider(
                ForeignVirtualDocumentProvider.scheme,
                virtualProvider
            )
        );

        ForeignLspBridge.instance = new ForeignLspBridge(virtualProvider);
        ForeignLspBridge.instance.setupStatusBar(context);
        return ForeignLspBridge.instance;
    }

    public static getInstance(): ForeignLspBridge {
        return ForeignLspBridge.instance;
    }

    /**
     * Checks if an official language extension for a given bridge is installed and active in VS Code.
     */
    public isLanguageExtensionAvailable(bridge: ForeignBridgeType): boolean {
        const extIds = ForeignLspBridge.LANGUAGE_EXTENSIONS[bridge] || [];
        return extIds.some(id => {
            const ext = vscode.extensions.getExtension(id);
            return ext !== undefined;
        });
    }

    /**
     * Returns metadata on all supported language extensions and their installation status.
     */
    public getLanguageExtensionStatuses(): LanguageExtensionInfo[] {
        const statuses: LanguageExtensionInfo[] = [];

        for (const [bridgeStr, extIds] of Object.entries(ForeignLspBridge.LANGUAGE_EXTENSIONS)) {
            const bridge = bridgeStr as ForeignBridgeType;
            let installed = false;
            let activeId: string | undefined = undefined;

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
    public createShadowDocumentForMember(
        bridge: ForeignBridgeType,
        packageName: string,
        memberPrefix: string = ''
    ): ShadowDocumentResult {
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
    public createShadowDocumentForEmbeddedCode(
        bridge: ForeignBridgeType,
        embeddedCode: string,
        cursorOffsetInCode: number
    ): ShadowDocumentResult {
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
    public async queryCompletions(
        shadowDoc: ShadowDocumentResult,
        triggerChar?: string
    ): Promise<vscode.CompletionItem[]> {
        try {
            // Open the virtual document in VS Code's document manager
            await vscode.workspace.openTextDocument(shadowDoc.uri);

            const results = await vscode.commands.executeCommand<vscode.CompletionList | vscode.CompletionItem[]>(
                'vscode.executeCompletionItemProvider',
                shadowDoc.uri,
                shadowDoc.position,
                triggerChar
            );

            if (!results) return [];

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
        } catch {
            return [];
        } finally {
            this.virtualProvider.clearDocument(shadowDoc.uri);
        }
    }

    /**
     * Executes hover provider on the virtual document via VS Code's internal LSP delegation.
     */
    public async queryHover(
        shadowDoc: ShadowDocumentResult
    ): Promise<vscode.Hover | null> {
        try {
            await vscode.workspace.openTextDocument(shadowDoc.uri);

            const hovers = await vscode.commands.executeCommand<vscode.Hover[]>(
                'vscode.executeHoverProvider',
                shadowDoc.uri,
                shadowDoc.position
            );

            if (!hovers || hovers.length === 0) return null;

            const firstHover = hovers[0];
            const badge = `**[${ForeignLspBridge.LANGUAGE_DISPLAY_NAMES[shadowDoc.bridgeType]} IntelliSense]**\n\n`;

            const enhancedContents = firstHover.contents.map(c => {
                if (typeof c === 'string') {
                    return new vscode.MarkdownString(badge + c);
                } else if (c instanceof vscode.MarkdownString) {
                    const md = new vscode.MarkdownString(badge + c.value);
                    md.isTrusted = c.isTrusted;
                    return md;
                }
                return c;
            });

            return new vscode.Hover(enhancedContents);
        } catch {
            return null;
        } finally {
            this.virtualProvider.clearDocument(shadowDoc.uri);
        }
    }

    /**
     * Executes signature help provider on the virtual document via VS Code's internal LSP delegation.
     */
    public async querySignatureHelp(
        shadowDoc: ShadowDocumentResult,
        triggerChar: string = '('
    ): Promise<vscode.SignatureHelp | null> {
        try {
            await vscode.workspace.openTextDocument(shadowDoc.uri);

            const help = await vscode.commands.executeCommand<vscode.SignatureHelp>(
                'vscode.executeSignatureHelpProvider',
                shadowDoc.uri,
                shadowDoc.position,
                triggerChar
            );

            return help || null;
        } catch {
            return null;
        } finally {
            this.virtualProvider.clearDocument(shadowDoc.uri);
        }
    }

    /**
     * Executes definition provider on the virtual document via VS Code's internal LSP delegation.
     */
    public async queryDefinition(
        shadowDoc: ShadowDocumentResult
    ): Promise<vscode.Definition | vscode.LocationLink[] | null> {
        try {
            await vscode.workspace.openTextDocument(shadowDoc.uri);

            const defs = await vscode.commands.executeCommand<vscode.Definition | vscode.LocationLink[]>(
                'vscode.executeDefinitionProvider',
                shadowDoc.uri,
                shadowDoc.position
            );

            return defs || null;
        } catch {
            return null;
        } finally {
            this.virtualProvider.clearDocument(shadowDoc.uri);
        }
    }

    /**
     * Configures the status bar item showing active foreign IntelliSense bridges.
     */
    private setupStatusBar(context: vscode.ExtensionContext): void {
        this.statusBarItem = vscode.window.createStatusBarItem(
            vscode.StatusBarAlignment.Right,
            90
        );
        this.statusBarItem.command = 'skylang.checkForeignExtensions';
        this.updateStatusBar();
        this.statusBarItem.show();

        context.subscriptions.push(this.statusBarItem);

        // Register QuickPick command to inspect and install language extensions
        context.subscriptions.push(
            vscode.commands.registerCommand('skylang.checkForeignExtensions', () => {
                this.showExtensionStatusQuickPick();
            })
        );
    }

    public updateStatusBar(): void {
        if (!this.statusBarItem) return;

        const statuses = this.getLanguageExtensionStatuses();
        const activeCount = statuses.filter(s => s.isInstalled).length;
        const totalCount = statuses.length;

        this.statusBarItem.text = `$(globe) Skylang Foreign LSP: ${activeCount}/${totalCount}`;
        this.statusBarItem.tooltip = new vscode.MarkdownString(
            `### Skylang Universal Foreign Language IntelliSense\n\n` +
            `Directly connected to **${activeCount} of ${totalCount}** official VS Code language extensions:\n\n` +
            statuses.map(s => `- **${s.name}**: ${s.isInstalled ? '✅ Connected (`' + s.activeExtensionId + '`)' : '⚠️ Not installed (Click to install)'}`).join('\n') +
            `\n\n*Click to inspect or install language extensions.*`
        );
    }

    /**
     * Shows an interactive QuickPick modal allowing developers to see status and install language extensions.
     */
    public async showExtensionStatusQuickPick(): Promise<void> {
        const statuses = this.getLanguageExtensionStatuses();
        const items: vscode.QuickPickItem[] = statuses.map(s => {
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
                const action = await vscode.window.showInformationMessage(
                    `Would you like to install the official extension for ${matchingStatus.name} (${extToInstall})?`,
                    'Install Extension',
                    'Cancel'
                );
                if (action === 'Install Extension') {
                    await vscode.commands.executeCommand('workbench.extensions.installExtension', extToInstall);
                    vscode.window.showInformationMessage(`Installing ${extToInstall}... Foreign IntelliSense will activate automatically once ready.`);
                    this.updateStatusBar();
                }
            }
        }
    }
}
