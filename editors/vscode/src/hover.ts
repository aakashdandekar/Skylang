import * as vscode from 'vscode';
import { KEYWORDS, TYPES, CONSTANTS } from './data/keywords';
import { BUILTIN_FUNCTIONS, COLLECTION_METHODS, PROPERTIES } from './data/builtins';
import { STDLIB_MODULES, COMMON_EXTERN_C_FUNCTIONS, COMMON_C_HEADERS } from './data/stdlib';
import { getForeignModule, ForeignModuleDoc } from './data/foreign';
import { SkylangCompletionItemProvider } from './completions';
import { ForeignLspBridge, ForeignBridgeType } from './foreignLspBridge';

export class SkylangHoverProvider implements vscode.HoverProvider {
    private completionProvider = new SkylangCompletionItemProvider();

    public async provideHover(
        document: vscode.TextDocument,
        position: vscode.Position,
        _token: vscode.CancellationToken
    ): Promise<vscode.Hover | null> {
        const lineText = document.lineAt(position.line).text;

        // 0. Check for Embedded Foreign Code hover (python.exec, cpp.compile, etc.)
        const embeddedContext = this.completionProvider.getEmbeddedCodeContext(document, position);
        if (embeddedContext) {
            const lspBridge = ForeignLspBridge.getInstance();
            if (lspBridge && lspBridge.isLanguageExtensionAvailable(embeddedContext.bridge)) {
                const shadowDoc = lspBridge.createShadowDocumentForEmbeddedCode(
                    embeddedContext.bridge,
                    embeddedContext.code,
                    embeddedContext.offset
                );
                const lspHover = await lspBridge.queryHover(shadowDoc);
                if (lspHover) return lspHover;
            }
        }

        // 1. Check for C Header hover in `cimport "header.h"`
        const headerRange = document.getWordRangeAtPosition(position, /[a-zA-Z0-9_\.]+\.h/);
        if (headerRange) {
            const header = document.getText(headerRange);
            if (COMMON_C_HEADERS.includes(header)) {
                const content = new vscode.MarkdownString();
                content.appendMarkdown(`### C Header: \`<${header}>\`\n\n`);
                content.appendMarkdown(`Standard C library header file imported via \`cimport "${header}"\` for Foreign Function Interface (FFI) bindings.`);
                return new vscode.Hover(content, headerRange);
            }
        }

        // 2. Check for member access like `math.sqrt`, `io.readfile`, `python.load`, `this.field`
        const memberRange = document.getWordRangeAtPosition(position, /[a-zA-Z0-9_]+\.[a-zA-Z0-9_]+/);
        if (memberRange) {
            const rawWord = document.getText(memberRange);
            const [receiver, member] = rawWord.split('.');

            // Stdlib or Interop module member
            if (STDLIB_MODULES[receiver]) {
                const mod = STDLIB_MODULES[receiver];
                if (mod.functions[member]) {
                    const fn = mod.functions[member];
                    const content = new vscode.MarkdownString();
                    content.appendCodeblock(fn.signature, 'skylang');
                    content.appendMarkdown(`\n\n${fn.description}\n\n`);
                    if (fn.params && fn.params.length > 0) {
                        content.appendMarkdown('**Parameters:**\n');
                        for (const p of fn.params) {
                            content.appendMarkdown(`- \`${p.name}\`: ${p.doc}\n`);
                        }
                    }
                    content.appendMarkdown(`\n**Returns:** \`${fn.returns || 'none'}\`\n`);
                    if (fn.example) {
                        content.appendMarkdown(`\n**Example:**\n\`\`\`skylang\n${fn.example}\n\`\`\``);
                    }
                    return new vscode.Hover(content, memberRange);
                }

                if (mod.constants && mod.constants[member]) {
                    const c = mod.constants[member];
                    const content = new vscode.MarkdownString();
                    content.appendCodeblock(c.signature, 'skylang');
                    content.appendMarkdown(`\n\n${c.description}\n\n**Type:** \`${c.returns || 'double'}\``);
                    if (c.example) {
                        content.appendMarkdown(`\n\n\`\`\`skylang\n${c.example}\n\`\`\``);
                    }
                    return new vscode.Hover(content, memberRange);
                }
            }

            // Foreign module member (e.g. express.get, app.listen, lodash.map, axios.post, np.array, Math_js.sqrt)
            const docSymbols = this.completionProvider.parseDocumentSymbols(document);
            let foreignMod: ForeignModuleDoc | undefined = undefined;
            const varSym = docSymbols.find(s => s.name === receiver);
            if (varSym && varSym.inferredType && varSym.inferredType.startsWith('foreign_')) {
                const matchBridge = varSym.inferredType.match(/^foreign_([a-z]+):(.*)$/);
                const bridge = (matchBridge ? matchBridge[1] : 'js') as ForeignBridgeType;
                const rawPkg = matchBridge ? matchBridge[2] : varSym.inferredType.replace(/^foreign_[a-z]+:/, '');

                const lspBridge = ForeignLspBridge.getInstance();
                if (lspBridge && lspBridge.isLanguageExtensionAvailable(bridge)) {
                    const shadowDoc = lspBridge.createShadowDocumentForMember(bridge, rawPkg, member);
                    const lspHover = await lspBridge.queryHover(shadowDoc);
                    if (lspHover) return lspHover;
                }

                foreignMod = getForeignModule(rawPkg);
            }
            if (!foreignMod) {
                foreignMod = getForeignModule(receiver);
            }

            if (foreignMod) {
                if (foreignMod.methods[member]) {
                    const fn = foreignMod.methods[member];
                    const content = new vscode.MarkdownString();
                    content.appendMarkdown(`### ${foreignMod.name}\n\n`);
                    content.appendCodeblock(fn.signature, 'skylang');
                    content.appendMarkdown(`\n\n${fn.description}\n\n`);
                    if (fn.params && fn.params.length > 0) {
                        content.appendMarkdown('**Parameters:**\n');
                        for (const p of fn.params) {
                            content.appendMarkdown(`- \`${p.name}\`: ${p.doc}\n`);
                        }
                    }
                    content.appendMarkdown(`\n**Returns:** \`${fn.returns || 'any'}\`\n`);
                    if (fn.example) {
                        content.appendMarkdown(`\n**Example:**\n\`\`\`skylang\n${fn.example}\n\`\`\``);
                    }
                    return new vscode.Hover(content, memberRange);
                }

                if (foreignMod.properties && foreignMod.properties[member]) {
                    const p = foreignMod.properties[member];
                    const content = new vscode.MarkdownString();
                    content.appendMarkdown(`### ${foreignMod.name}\n\n`);
                    content.appendCodeblock(p.signature, 'skylang');
                    content.appendMarkdown(`\n\n${p.description}\n\n**Type:** \`${p.returns || 'any'}\``);
                    return new vscode.Hover(content, memberRange);
                }
            }

            // Collection method (e.g. items.push, dict.has, str.value)
            if (COLLECTION_METHODS[member]) {
                const m = COLLECTION_METHODS[member];
                const content = new vscode.MarkdownString();
                content.appendCodeblock(m.signature, 'skylang');
                content.appendMarkdown(`\n\n${m.description}\n\n`);
                if (m.params && m.params.length > 0) {
                    content.appendMarkdown('**Parameters:**\n');
                    for (const p of m.params) {
                        content.appendMarkdown(`- \`${p.name}\`: ${p.doc}\n`);
                    }
                }
                content.appendMarkdown(`\n**Returns:** \`${m.returns || 'none'}\``);
                if (m.example) {
                    content.appendMarkdown(`\n\n\`\`\`skylang\n${m.example}\n\`\`\``);
                }
                return new vscode.Hover(content, memberRange);
            }

            // Property (e.g. .size, .T)
            if (PROPERTIES[member]) {
                const p = PROPERTIES[member];
                const content = new vscode.MarkdownString();
                content.appendCodeblock(p.signature, 'skylang');
                content.appendMarkdown(`\n\n${p.description}\n\n**Returns:** \`${p.returns}\``);
                if (p.example) {
                    content.appendMarkdown(`\n\n\`\`\`skylang\n${p.example}\n\`\`\``);
                }
                return new vscode.Hover(content, memberRange);
            }

            // User class instance member or `this.field`
            if (receiver === 'this') {
                const classField = docSymbols.find(s => s.name === member && s.kind === vscode.CompletionItemKind.Field);
                if (classField) {
                    const content = new vscode.MarkdownString();
                    content.appendCodeblock(classField.detail, 'skylang');
                    content.appendMarkdown(`\n\n${classField.doc || 'Class field'}`);
                    return new vscode.Hover(content, memberRange);
                }
                const classMethod = docSymbols.find(s => s.name === member && s.kind === vscode.CompletionItemKind.Method);
                if (classMethod) {
                    const content = new vscode.MarkdownString();
                    content.appendCodeblock(classMethod.detail, 'skylang');
                    content.appendMarkdown(`\n\n${classMethod.doc || 'Class method'}`);
                    return new vscode.Hover(content, memberRange);
                }
            } else {
                if (varSym && varSym.inferredType) {
                    const classDef = this.completionProvider.findClassByName(document, varSym.inferredType);
                    if (classDef) {
                        const m = classDef.methods.find(m => m.name === member);
                        if (m) {
                            const content = new vscode.MarkdownString();
                            content.appendCodeblock(`(method) ${classDef.name}.${m.name}(${m.params.join(', ')})`, 'skylang');
                            content.appendMarkdown(`\n\nMethod \`${m.name}\` of class \`${classDef.name}\``);
                            return new vscode.Hover(content, memberRange);
                        }
                    }
                }
            }
        }

        // 3. Single word token
        const singleWordRange = document.getWordRangeAtPosition(position, /[a-zA-Z0-9_]+/);
        if (!singleWordRange) return null;
        const word = document.getText(singleWordRange);

        // Keywords
        if (KEYWORDS[word]) {
            const kw = KEYWORDS[word];
            const content = new vscode.MarkdownString();
            content.appendMarkdown(`### ${kw.name}\n\n`);
            content.appendCodeblock(kw.syntax, 'skylang');
            content.appendMarkdown(`\n\n${kw.description}\n`);
            if (kw.example) {
                content.appendMarkdown(`\n**Example:**\n\`\`\`skylang\n${kw.example}\n\`\`\``);
            }
            return new vscode.Hover(content, singleWordRange);
        }

        // Types
        if (TYPES[word]) {
            const t = TYPES[word];
            const content = new vscode.MarkdownString();
            content.appendMarkdown(`### ${t.name}\n\n`);
            content.appendCodeblock(t.syntax, 'skylang');
            content.appendMarkdown(`\n\n${t.description}\n`);
            if (t.example) {
                content.appendMarkdown(`\n**Example:**\n\`\`\`skylang\n${t.example}\n\`\`\``);
            }
            return new vscode.Hover(content, singleWordRange);
        }

        // Constants
        if (CONSTANTS[word]) {
            const c = CONSTANTS[word];
            const content = new vscode.MarkdownString();
            content.appendMarkdown(`### ${c.name}\n\n`);
            content.appendMarkdown(`${c.description}\n`);
            if (c.example) {
                content.appendMarkdown(`\n**Example:**\n\`\`\`skylang\n${c.example}\n\`\`\``);
            }
            return new vscode.Hover(content, singleWordRange);
        }

        // Built-in Functions
        if (BUILTIN_FUNCTIONS[word]) {
            const fn = BUILTIN_FUNCTIONS[word];
            const content = new vscode.MarkdownString();
            content.appendCodeblock(fn.signature, 'skylang');
            content.appendMarkdown(`\n\n${fn.description}\n\n`);
            if (fn.params && fn.params.length > 0) {
                content.appendMarkdown('**Parameters:**\n');
                for (const p of fn.params) {
                    content.appendMarkdown(`- \`${p.name}\`: ${p.doc}\n`);
                }
            }
            content.appendMarkdown(`\n**Returns:** \`${fn.returns || 'none'}\`\n`);
            if (fn.example) {
                content.appendMarkdown(`\n**Example:**\n\`\`\`skylang\n${fn.example}\n\`\`\``);
            }
            return new vscode.Hover(content, singleWordRange);
        }

        // Standard Library Modules
        if (STDLIB_MODULES[word]) {
            const mod = STDLIB_MODULES[word];
            const content = new vscode.MarkdownString();
            content.appendMarkdown(`### ${mod.name}\n\n`);
            content.appendMarkdown(`${mod.description}\n\n**Available Methods:**\n`);
            const fnList = Object.keys(mod.functions).map(f => `\`${f}\``).join(', ');
            content.appendMarkdown(fnList);
            if (mod.constants) {
                const cList = Object.keys(mod.constants).map(c => `\`${c}\``).join(', ');
                content.appendMarkdown(`\n\n**Constants:** ${cList}`);
            }

            // Check if interop module needs import
            if (['python', 'js', 'cpp', 'java', 'golang', 'rust'].includes(word)) {
                const text = document.getText();
                const importRegex = new RegExp(`^\\s*import\\s+[^;\\n]*\\b${word}\\b`, 'm');
                if (!importRegex.test(text)) {
                    content.appendMarkdown(`\n\n> 💡 **Notice:** \`${word}\` is not yet imported in this file. Add \`import ${word}\` at the top of your file to use it.`);
                }
            }

            return new vscode.Hover(content, singleWordRange);
        }

        // Common C extern functions
        if (COMMON_EXTERN_C_FUNCTIONS[word]) {
            const ext = COMMON_EXTERN_C_FUNCTIONS[word];
            const content = new vscode.MarkdownString();
            content.appendCodeblock(ext.signature, 'skylang');
            content.appendMarkdown(`\n\n${ext.doc}\n\n**Header:** \`<${ext.header}>\``);
            return new vscode.Hover(content, singleWordRange);
        }

        // Collection Methods & Properties
        if (COLLECTION_METHODS[word]) {
            const m = COLLECTION_METHODS[word];
            const content = new vscode.MarkdownString();
            content.appendCodeblock(m.signature, 'skylang');
            content.appendMarkdown(`\n\n${m.description}\n\n**Returns:** \`${m.returns || 'none'}\``);
            if (m.example) {
                content.appendMarkdown(`\n\n\`\`\`skylang\n${m.example}\n\`\`\``);
            }
            return new vscode.Hover(content, singleWordRange);
        }

        if (PROPERTIES[word]) {
            const p = PROPERTIES[word];
            const content = new vscode.MarkdownString();
            content.appendCodeblock(p.signature, 'skylang');
            content.appendMarkdown(`\n\n${p.description}\n\n**Returns:** \`${p.returns}\``);
            return new vscode.Hover(content, singleWordRange);
        }

        // User Defined Document Symbols
        const symbols = this.completionProvider.parseDocumentSymbols(document);
        const matchedSymbol = symbols.find(s => s.name === word);
        if (matchedSymbol) {
            const content = new vscode.MarkdownString();
            content.appendCodeblock(matchedSymbol.detail, 'skylang');
            if (matchedSymbol.doc) {
                content.appendMarkdown(`\n\n${matchedSymbol.doc}`);
            }
            if (matchedSymbol.inferredType) {
                content.appendMarkdown(`\n\n**Inferred Type:** \`${matchedSymbol.inferredType}\``);
                if (matchedSymbol.inferredType.startsWith('foreign_')) {
                    const rawPkg = matchedSymbol.inferredType.replace(/^foreign_[a-z]+:/, '');
                    const fMod = getForeignModule(rawPkg);
                    if (fMod) {
                        content.appendMarkdown(`\n\n**Available Methods:**\n${Object.keys(fMod.methods).map(m => `\`${m}\``).join(', ')}`);
                    }
                }
            }
            return new vscode.Hover(content, singleWordRange);
        }

        // Direct Foreign Module (e.g. express, lodash, axios, numpy, etc.)
        const directFMod = getForeignModule(word);
        if (directFMod) {
            const content = new vscode.MarkdownString();
            content.appendMarkdown(`### ${directFMod.name}\n\n`);
            content.appendMarkdown(`${directFMod.description}\n\n**Available Methods:**\n`);
            const fnList = Object.keys(directFMod.methods).map(f => `\`${f}\``).join(', ');
            content.appendMarkdown(fnList);
            return new vscode.Hover(content, singleWordRange);
        }

        return null;
    }
}
