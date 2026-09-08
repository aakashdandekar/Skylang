import * as vscode from 'vscode';
import { KEYWORDS, TYPES, CONSTANTS } from './data/keywords';
import { BUILTIN_FUNCTIONS, COLLECTION_METHODS, PROPERTIES, CONTAINER_TYPE_MEMBERS } from './data/builtins';
import {
    STDLIB_MODULES,
    COMMON_PYTHON_PACKAGES,
    COMMON_JAVA_CLASSES,
    COMMON_NPM_PACKAGES,
    COMMON_C_HEADERS,
    COMMON_EXTERN_C_FUNCTIONS,
    COMMON_GO_PACKAGES,
    COMMON_RUST_CRATES
} from './data/stdlib';

export interface DocumentSymbolInfo {
    name: string;
    kind: vscode.CompletionItemKind;
    detail: string;
    doc?: string;
    params?: string[];
    line: number;
    containerName?: string;
    inferredType?: string; // 'list' | 'dict' | 'set' | 'string' | 'array' | 'tuple' | 'sortedList' | 'int' | 'double' | 'bool' | 'char' | ClassName | InteropName
}

export class SkylangCompletionItemProvider implements vscode.CompletionItemProvider {
    public async provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position,
        token: vscode.CancellationToken,
        context: vscode.CompletionContext
    ): Promise<vscode.CompletionItem[] | vscode.CompletionList> {
        const lineText = document.lineAt(position.line).text;
        const linePrefix = lineText.substring(0, position.character);

        // Check if inside a comment
        if (this.isInsideComment(lineText, position.character)) {
            return [];
        }

        const items: vscode.CompletionItem[] = [];

        // 1. Context: extern f ...
        const externMatch = linePrefix.match(/extern\s+f\s+([a-zA-Z0-9_]*)$/);
        if (externMatch) {
            for (const [fnName, fnInfo] of Object.entries(COMMON_EXTERN_C_FUNCTIONS)) {
                const item = new vscode.CompletionItem(fnName, vscode.CompletionItemKind.Function);
                item.detail = fnInfo.signature;
                item.documentation = new vscode.MarkdownString(`${fnInfo.doc}\n\n**Header:** \`${fnInfo.header}\``);
                item.insertText = new vscode.SnippetString(`${fnInfo.signature.replace('extern f ', '')}`);
                item.sortText = `0_${fnName}`;
                items.push(item);
            }
            return items;
        }

        // 3. Context: import mod1, mod2, ...
        const importMatch = linePrefix.match(/(?:^|\s)import\s+([^;\n]*)$/);
        if (importMatch) {
            const rawList = importMatch[1];
            const parts = rawList.split(',').map(s => s.trim().toLowerCase());
            const currentQuery = parts[parts.length - 1];
            const alreadyImported = parts.slice(0, -1);

            const allModules = ['python', 'js', 'cpp', 'java', 'go', 'golang', 'rust', 'math', 'fmt', 'io'];
            const importItems: vscode.CompletionItem[] = [];
            for (const mod of allModules) {
                if (alreadyImported.includes(mod)) continue;
                const modDoc = STDLIB_MODULES[mod];
                const item = new vscode.CompletionItem(mod, vscode.CompletionItemKind.Module);
                item.detail = modDoc ? modDoc.name : `import ${mod}`;
                item.documentation = new vscode.MarkdownString(modDoc ? modDoc.description : `Import ${mod} module.`);
                item.insertText = mod;
                if (currentQuery.length > 0 && mod.toLowerCase().startsWith(currentQuery)) {
                    item.sortText = `0000_${mod}`;
                    item.preselect = true;
                } else {
                    item.sortText = `0001_${mod}`;
                }
                importItems.push(item);
            }
            return new vscode.CompletionList(importItems, false);
        }

        // 4. Context: python.load("..."), java.load("..."), js.load("..."), go.load("..."), rust.load("...")
        const pyLoadMatch = linePrefix.match(/python\.load\s*\(\s*["']([^"']*)$/);
        if (pyLoadMatch) {
            for (const pkg of COMMON_PYTHON_PACKAGES) {
                const item = new vscode.CompletionItem(pkg, vscode.CompletionItemKind.Module);
                item.detail = `Python package: ${pkg}`;
                item.documentation = new vscode.MarkdownString(`Load Python package \`${pkg}\` into Skylang.`);
                item.insertText = pkg;
                item.sortText = `0_${pkg}`;
                items.push(item);
            }
            return items;
        }

        const javaLoadMatch = linePrefix.match(/java\.load\s*\(\s*["']([^"']*)$/);
        if (javaLoadMatch) {
            for (const cls of COMMON_JAVA_CLASSES) {
                const item = new vscode.CompletionItem(cls, vscode.CompletionItemKind.Class);
                item.detail = `Java class: ${cls}`;
                item.documentation = new vscode.MarkdownString(`Load Java class \`${cls}\` via JVM reflection.`);
                item.insertText = cls;
                item.sortText = `0_${cls}`;
                items.push(item);
            }
            return items;
        }

        const jsLoadMatch = linePrefix.match(/js\.load\s*\(\s*["']([^"']*)$/);
        if (jsLoadMatch) {
            for (const pkg of COMMON_NPM_PACKAGES) {
                const item = new vscode.CompletionItem(pkg, vscode.CompletionItemKind.Module);
                item.detail = `NPM package / JS global: ${pkg}`;
                item.documentation = new vscode.MarkdownString(`Load JS global or NPM package \`${pkg}\` into Skylang.`);
                item.insertText = pkg;
                item.sortText = `0_${pkg}`;
                items.push(item);
            }
            return items;
        }

        const goLoadMatch = linePrefix.match(/(?:go|golang)\.load\s*\(\s*["']([^"']*)$/);
        if (goLoadMatch) {
            for (const pkg of COMMON_GO_PACKAGES) {
                const item = new vscode.CompletionItem(pkg, vscode.CompletionItemKind.Module);
                item.detail = `Go package: ${pkg}`;
                item.documentation = new vscode.MarkdownString(`Load Go package \`${pkg}\` into Skylang.`);
                item.insertText = pkg;
                item.sortText = `0_${pkg}`;
                items.push(item);
            }
            return items;
        }

        const rustLoadMatch = linePrefix.match(/rust\.load\s*\(\s*["']([^"']*)$/);
        if (rustLoadMatch) {
            for (const crate of COMMON_RUST_CRATES) {
                const item = new vscode.CompletionItem(crate, vscode.CompletionItemKind.Module);
                item.detail = `Rust crate / module: ${crate}`;
                item.documentation = new vscode.MarkdownString(`Load Rust module/crate \`${crate}\` into Skylang.`);
                item.insertText = crate;
                item.sortText = `0_${crate}`;
                items.push(item);
            }
            return items;
        }

        // Check if inside string (and not one of the load contexts above)
        if (this.isInsideString(lineText, position.character)) {
            return [];
        }

        // 5. Member completion triggered by '.' (e.g. math., receiver., this.)
        const dotMatch = linePrefix.match(/([a-zA-Z_][a-zA-Z0-9_]*)\.\s*$/);
        if (dotMatch) {
            const receiver = dotMatch[1];
            const parsedSymbols = this.parseDocumentSymbols(document);

            // A. Standard Library & Interop Modules (math., io., fmt., str., python., js., cpp., java., go., rust.)
            if (STDLIB_MODULES[receiver]) {
                const mod = STDLIB_MODULES[receiver];
                for (const [fnName, fnDoc] of Object.entries(mod.functions)) {
                    const item = new vscode.CompletionItem(fnName, vscode.CompletionItemKind.Function);
                    item.detail = fnDoc.signature;
                    item.documentation = new vscode.MarkdownString(
                        `${fnDoc.description}\n\n**Returns:** \`${fnDoc.returns || 'none'}\`${
                            fnDoc.example ? `\n\n\`\`\`skylang\n${fnDoc.example}\n\`\`\`` : ''
                        }`
                    );
                    item.insertText = new vscode.SnippetString(this.generateSnippet(fnName, fnDoc.params));
                    item.sortText = `0_${fnName}`;
                    items.push(item);
                }
                if (mod.constants) {
                    for (const [constName, constDoc] of Object.entries(mod.constants)) {
                        const item = new vscode.CompletionItem(constName, vscode.CompletionItemKind.Constant);
                        item.detail = constDoc.signature;
                        item.documentation = new vscode.MarkdownString(
                            `${constDoc.description}\n\n**Type:** \`${constDoc.returns || 'double'}\`${
                                constDoc.example ? `\n\n\`\`\`skylang\n${constDoc.example}\n\`\`\`` : ''
                            }`
                        );
                        item.sortText = `1_${constName}`;
                        items.push(item);
                    }
                }
                return items;
            }

            // B. `this.` member completion inside a class
            if (receiver === 'this') {
                const classContext = this.findEnclosingClass(document, position.line);
                if (classContext) {
                    for (const field of classContext.fields) {
                        const item = new vscode.CompletionItem(field, vscode.CompletionItemKind.Field);
                        item.detail = `(field) this.${field}`;
                        item.documentation = new vscode.MarkdownString(`Field \`${field}\` of class \`${classContext.name}\``);
                        item.sortText = `0_${field}`;
                        items.push(item);
                    }
                    for (const method of classContext.methods) {
                        const item = new vscode.CompletionItem(method.name, vscode.CompletionItemKind.Method);
                        item.detail = `(method) ${method.name}(${method.params.join(', ')})`;
                        item.documentation = new vscode.MarkdownString(`Method \`${method.name}\` of class \`${classContext.name}\``);
                        item.insertText = new vscode.SnippetString(this.generateSnippetFromParamNames(method.name, method.params));
                        item.sortText = `1_${method.name}`;
                        items.push(item);
                    }
                }
                return items;
            }

            // C. Inferred variable type member completion
            const varSymbol = parsedSymbols.find(s => s.name === receiver);
            if (varSymbol && varSymbol.inferredType) {
                const infType = varSymbol.inferredType;

                // Check container types (list, dict, set, sortedList, string, array, tuple)
                if (CONTAINER_TYPE_MEMBERS[infType]) {
                    const memberDefs = CONTAINER_TYPE_MEMBERS[infType];
                    for (const mName of memberDefs.methods) {
                        if (COLLECTION_METHODS[mName]) {
                            const mDoc = COLLECTION_METHODS[mName];
                            const item = new vscode.CompletionItem(mName, vscode.CompletionItemKind.Method);
                            item.detail = mDoc.signature;
                            item.documentation = new vscode.MarkdownString(
                                `${mDoc.description}\n\n**Returns:** \`${mDoc.returns || 'none'}\`${
                                    mDoc.example ? `\n\n\`\`\`skylang\n${mDoc.example}\n\`\`\`` : ''
                                }`
                            );
                            item.insertText = new vscode.SnippetString(this.generateSnippet(mName, mDoc.params));
                            item.sortText = `0_${mName}`;
                            items.push(item);
                        }
                    }
                    for (const pName of memberDefs.properties) {
                        if (PROPERTIES[pName]) {
                            const pDoc = PROPERTIES[pName];
                            const item = new vscode.CompletionItem(pName, vscode.CompletionItemKind.Property);
                            item.detail = pDoc.signature;
                            item.documentation = new vscode.MarkdownString(
                                `${pDoc.description}\n\n**Returns:** \`${pDoc.returns || 'type'}\``
                            );
                            item.sortText = `1_${pName}`;
                            items.push(item);
                        }
                    }
                    return items;
                }

                // Check class instance (e.g. c := Dog("Rex"))
                const classDef = this.findClassByName(document, infType);
                if (classDef) {
                    for (const method of classDef.methods) {
                        const item = new vscode.CompletionItem(method.name, vscode.CompletionItemKind.Method);
                        item.detail = `(method) ${classDef.name}.${method.name}(${method.params.join(', ')})`;
                        item.documentation = new vscode.MarkdownString(`Method \`${method.name}\` of class \`${classDef.name}\``);
                        item.insertText = new vscode.SnippetString(this.generateSnippetFromParamNames(method.name, method.params));
                        item.sortText = `0_${method.name}`;
                        items.push(item);
                    }
                    for (const field of classDef.fields) {
                        const item = new vscode.CompletionItem(field, vscode.CompletionItemKind.Field);
                        item.detail = `(field) ${classDef.name}.${field}`;
                        item.documentation = new vscode.MarkdownString(`Field \`${field}\` of class \`${classDef.name}\``);
                        item.sortText = `1_${field}`;
                        items.push(item);
                    }
                    // Add generic properties (.T)
                    const tProp = PROPERTIES['T'];
                    const tItem = new vscode.CompletionItem('T', vscode.CompletionItemKind.Property);
                    tItem.detail = tProp.signature;
                    tItem.documentation = new vscode.MarkdownString(tProp.description);
                    tItem.sortText = `2_T`;
                    items.push(tItem);
                    return items;
                }
            }

            // D. Fallback: Generic Collection Methods & Properties
            for (const [methodName, methodDoc] of Object.entries(COLLECTION_METHODS)) {
                const item = new vscode.CompletionItem(methodName, vscode.CompletionItemKind.Method);
                item.detail = methodDoc.signature;
                item.documentation = new vscode.MarkdownString(
                    `${methodDoc.description}\n\n**Returns:** \`${methodDoc.returns || 'none'}\`${
                        methodDoc.example ? `\n\n\`\`\`skylang\n${methodDoc.example}\n\`\`\`` : ''
                    }`
                );
                item.insertText = new vscode.SnippetString(this.generateSnippet(methodName, methodDoc.params));
                item.sortText = `0_${methodName}`;
                items.push(item);
            }

            for (const [propName, propDoc] of Object.entries(PROPERTIES)) {
                const item = new vscode.CompletionItem(propName, vscode.CompletionItemKind.Property);
                item.detail = propDoc.signature;
                item.documentation = new vscode.MarkdownString(
                    `${propDoc.description}\n\n**Returns:** \`${propDoc.returns || 'type'}\``
                );
                item.sortText = `1_${propName}`;
                items.push(item);
            }

            return items;
        }

        // 6. Named argument completion inside function / method / constructor calls: fn(a=..., b=...)
        const callMatch = linePrefix.match(/([a-zA-Z_][a-zA-Z0-9_\.]*)\s*\(\s*([^)]*)$/);
        if (callMatch) {
            const rawCallee = callMatch[1];
            const parsedSymbols = this.parseDocumentSymbols(document);

            let params: string[] = [];

            // Builtin function parameter names
            if (BUILTIN_FUNCTIONS[rawCallee] && BUILTIN_FUNCTIONS[rawCallee].params) {
                params = BUILTIN_FUNCTIONS[rawCallee].params!.map(p => p.name).filter(n => !n.startsWith('...'));
            }

            // User function or class constructor
            if (params.length === 0) {
                const targetSymbol = parsedSymbols.find(
                    s =>
                        s.name === rawCallee &&
                        (s.kind === vscode.CompletionItemKind.Function ||
                            s.kind === vscode.CompletionItemKind.Class ||
                            s.kind === vscode.CompletionItemKind.Method)
                );
                if (targetSymbol && targetSymbol.params && targetSymbol.params.length > 0) {
                    params = targetSymbol.params;
                }
            }

            // Class method call: inst.method(...)
            if (params.length === 0 && rawCallee.includes('.')) {
                const [recName, methodName] = rawCallee.split('.');
                const varSym = parsedSymbols.find(s => s.name === recName);
                if (varSym && varSym.inferredType) {
                    const classDef = this.findClassByName(document, varSym.inferredType);
                    if (classDef) {
                        const m = classDef.methods.find(m => m.name === methodName);
                        if (m && m.params) {
                            params = m.params;
                        }
                    }
                }
            }

            if (params.length > 0) {
                for (const param of params) {
                    const item = new vscode.CompletionItem(`${param}=`, vscode.CompletionItemKind.Variable);
                    item.detail = `Named parameter: ${param}=`;
                    item.documentation = new vscode.MarkdownString(`Pass named argument \`${param}\` to \`${rawCallee}\``);
                    item.insertText = new vscode.SnippetString(`${param}=\${1}`);
                    item.sortText = `0_${param}`;
                    items.push(item);
                }
            }
        }

        // 7. Keywords & Control Flow
        for (const [kw, doc] of Object.entries(KEYWORDS)) {
            const item = new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword);
            item.detail = doc.name;
            item.documentation = new vscode.MarkdownString(
                `${doc.description}\n\n**Syntax:** \`${doc.syntax}\`${
                    doc.example ? `\n\n\`\`\`skylang\n${doc.example}\n\`\`\`` : ''
                }`
            );
            item.sortText = `3_${kw}`;
            items.push(item);
        }

        // 8. Built-in Types (I, D, B, C, S, L, T, DICT, SET, SL, int, double, etc.)
        for (const [typeName, doc] of Object.entries(TYPES)) {
            const item = new vscode.CompletionItem(typeName, vscode.CompletionItemKind.TypeParameter);
            item.detail = doc.name;
            item.documentation = new vscode.MarkdownString(
                `${doc.description}\n\n**Syntax:** \`${doc.syntax}\`${
                    doc.example ? `\n\n\`\`\`skylang\n${doc.example}\n\`\`\`` : ''
                }`
            );
            item.sortText = `4_${typeName}`;
            items.push(item);
        }

        // 9. Constants & Special Identifiers (true, false, none, nil, args, _)
        for (const [cName, doc] of Object.entries(CONSTANTS)) {
            const item = new vscode.CompletionItem(cName, vscode.CompletionItemKind.Constant);
            item.detail = doc.name;
            item.documentation = new vscode.MarkdownString(
                `${doc.description}${doc.example ? `\n\n\`\`\`skylang\n${doc.example}\n\`\`\`` : ''}`
            );
            item.sortText = `2_${cName}`;
            items.push(item);
        }

        // 10. Built-in Functions (print, println, range, len, type, takes, error, panic, gc, free)
        for (const [fnName, fnDoc] of Object.entries(BUILTIN_FUNCTIONS)) {
            const item = new vscode.CompletionItem(fnName, vscode.CompletionItemKind.Function);
            item.detail = fnDoc.signature;
            item.documentation = new vscode.MarkdownString(
                `${fnDoc.description}\n\n**Returns:** \`${fnDoc.returns || 'none'}\`${
                    fnDoc.example ? `\n\n\`\`\`skylang\n${fnDoc.example}\n\`\`\`` : ''
                }`
            );
            item.insertText = new vscode.SnippetString(this.generateSnippet(fnName, fnDoc.params));
            item.sortText = `1_${fnName}`;
            items.push(item);
        }

        // 11. Standard Library & Interop Modules (math, io, fmt, str, python, js, cpp, java)
        for (const [modName, modDoc] of Object.entries(STDLIB_MODULES)) {
            const item = new vscode.CompletionItem(modName, vscode.CompletionItemKind.Module);
            item.detail = modDoc.name;
            item.documentation = new vscode.MarkdownString(modDoc.description);
            item.additionalTextEdits = this.getAutoImportEdits(document, modName);
            item.sortText = `00_${modName}`;
            items.push(item);
        }

        // 12. Document Defined Symbols (Functions, Classes, Methods, Variables, Parameters)
        const docSymbols = this.parseDocumentSymbols(document);
        for (const sym of docSymbols) {
            // Avoid duplicate with keywords
            if (KEYWORDS[sym.name] || TYPES[sym.name] || BUILTIN_FUNCTIONS[sym.name] || CONSTANTS[sym.name]) continue;

            const item = new vscode.CompletionItem(sym.name, sym.kind);
            item.detail = sym.detail;
            if (sym.doc) {
                item.documentation = new vscode.MarkdownString(sym.doc);
            }
            if (sym.kind === vscode.CompletionItemKind.Function || sym.kind === vscode.CompletionItemKind.Class) {
                if (sym.params && sym.params.length > 0) {
                    item.insertText = new vscode.SnippetString(this.generateSnippetFromParamNames(sym.name, sym.params));
                } else {
                    item.insertText = new vscode.SnippetString(`${sym.name}()`);
                }
            }
            item.sortText = `0_${sym.name}`;
            items.push(item);
        }

        // 13. Workspace Symbols from other .sky files
        try {
            const workspaceFiles = await vscode.workspace.findFiles('**/*.sky', '**/node_modules/**', 20, token);
            for (const file of workspaceFiles) {
                if (file.toString() === document.uri.toString()) continue;
                const otherDoc = await vscode.workspace.openTextDocument(file);
                const otherSymbols = this.parseDocumentSymbols(otherDoc);
                for (const sym of otherSymbols) {
                    if (
                        sym.kind === vscode.CompletionItemKind.Function ||
                        sym.kind === vscode.CompletionItemKind.Class
                    ) {
                        if (!items.some(i => i.label === sym.name)) {
                            const item = new vscode.CompletionItem(sym.name, sym.kind);
                            item.detail = `${sym.detail} (from ${vscode.workspace.asRelativePath(file)})`;
                            item.documentation = new vscode.MarkdownString(sym.doc || '');
                            if (sym.params && sym.params.length > 0) {
                                item.insertText = new vscode.SnippetString(
                                    this.generateSnippetFromParamNames(sym.name, sym.params)
                                );
                            } else {
                                item.insertText = new vscode.SnippetString(`${sym.name}()`);
                            }
                            item.sortText = `5_${sym.name}`;
                            items.push(item);
                        }
                    }
                }
            }
        } catch {
            // Workspace search is optional
        }

        return items;
    }

    private generateSnippet(name: string, params?: { name: string; doc: string }[]): string {
        if (!params || params.length === 0) {
            return `${name}()`;
        }

        // Special handling for load/import: place cursor directly inside string quotes without parameter text
        if (name === 'load' || name === 'import') {
            return `${name}("\${1}")`;
        }

        if (name === 'eval' || name === 'exec' || name === 'compile') {
            return `${name}("\${1}")`;
        }

        if (name === 'readfile' || name === 'readlines' || name === 'exists' || name === 'remove') {
            return `${name}("\${1}")`;
        }

        const filtered = params.filter(p => !p.name.startsWith('...'));
        if (filtered.length === 0) {
            return `${name}(\${1})`;
        }
        if (filtered.length === 1) {
            const p = filtered[0].name.toLowerCase();
            if (p.includes('str') || p.includes('path') || p.includes('file') || p.includes('name') || p.includes('pkg') || p.includes('code')) {
                return `${name}("\${1}")`;
            }
            return `${name}(\${1})`;
        }

        const paramSnippets = filtered.map((_, idx) => `\${${idx + 1}}`);
        return `${name}(${paramSnippets.join(', ')})`;
    }

    private generateSnippetFromParamNames(name: string, paramNames: string[]): string {
        if (!paramNames || paramNames.length === 0) {
            return `${name}()`;
        }
        const paramSnippets = paramNames.map((_, idx) => `\${${idx + 1}}`);
        return `${name}(${paramSnippets.join(', ')})`;
    }

    private isInsideComment(lineText: string, col: number): boolean {
        let inSingleQuote = false;
        let inDoubleQuote = false;
        for (let i = 0; i < col; i++) {
            const ch = lineText[i];
            const prev = i > 0 ? lineText[i - 1] : '';
            if (!inSingleQuote && !inDoubleQuote) {
                if (ch === '/' && i + 1 < lineText.length && lineText[i + 1] === '/') {
                    return true;
                }
                if (ch === '#') {
                    return true;
                }
            }
            if (ch === '"' && prev !== '\\' && !inSingleQuote) {
                inDoubleQuote = !inDoubleQuote;
            } else if (ch === '\'' && prev !== '\\' && !inDoubleQuote) {
                inSingleQuote = !inSingleQuote;
            }
        }
        return false;
    }

    private isInsideString(lineText: string, col: number): boolean {
        let inSingleQuote = false;
        let inDoubleQuote = false;
        for (let i = 0; i < col; i++) {
            const ch = lineText[i];
            const prev = i > 0 ? lineText[i - 1] : '';
            if (ch === '/' && prev === '/' && !inSingleQuote && !inDoubleQuote) {
                return false;
            }
            if (ch === '#' && !inSingleQuote && !inDoubleQuote) {
                return false;
            }
            if (ch === '"' && prev !== '\\' && !inSingleQuote) {
                inDoubleQuote = !inDoubleQuote;
            } else if (ch === '\'' && prev !== '\\' && !inDoubleQuote) {
                inSingleQuote = !inSingleQuote;
            }
        }
        return inSingleQuote || inDoubleQuote;
    }

    public parseDocumentSymbols(document: vscode.TextDocument): DocumentSymbolInfo[] {
        const symbols: DocumentSymbolInfo[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        let currentClass: { name: string; line: number } | null = null;
        let braceDepth = 0;

        for (let i = 0; i < lines.length; i++) {
            const rawLine = lines[i];
            const line = rawLine.replace(/\/\/.*$/, '').replace(/#.*$/, '').trim();
            if (!line) continue;

            // Class declaration: class Dog {
            const classMatch = line.match(/^class\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
            if (classMatch) {
                const className = classMatch[1];
                currentClass = { name: className, line: i };
                symbols.push({
                    name: className,
                    kind: vscode.CompletionItemKind.Class,
                    detail: `class ${className}`,
                    doc: `Class \`${className}\` declared on line ${i + 1}`,
                    line: i
                });
            }

            // Standalone function declaration: f add {
            const fnMatch = line.match(/^f\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
            if (fnMatch) {
                const fnName = fnMatch[1];
                const params = this.extractTakesParams(lines, i + 1);
                symbols.push({
                    name: fnName,
                    kind: vscode.CompletionItemKind.Function,
                    detail: `f ${fnName}(${params.join(', ')})`,
                    doc: `Function \`${fnName}\` declared on line ${i + 1}`,
                    params,
                    line: i
                });
            }

            // Extern C function: extern f cos(x)
            const externMatch = line.match(/^extern\s+f\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)/);
            if (externMatch) {
                const extName = externMatch[1];
                const rawParams = externMatch[2].split(',').map(p => p.trim()).filter(p => p.length > 0);
                symbols.push({
                    name: extName,
                    kind: vscode.CompletionItemKind.Function,
                    detail: `extern f ${extName}(${rawParams.join(', ')})`,
                    doc: `External C function \`${extName}\` declared on line ${i + 1}`,
                    params: rawParams,
                    line: i
                });
            }

            // Class body parsing
            if (currentClass) {
                // Class method: bark { takes(...) ... }
                const methodMatch = line.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s*\{/);
                if (
                    methodMatch &&
                    methodMatch[1] !== 'init' &&
                    methodMatch[1] !== 'if' &&
                    methodMatch[1] !== 'for' &&
                    methodMatch[1] !== 'else' &&
                    methodMatch[1] !== 'elif'
                ) {
                    const methodName = methodMatch[1];
                    const params = this.extractTakesParams(lines, i + 1);
                    symbols.push({
                        name: methodName,
                        kind: vscode.CompletionItemKind.Method,
                        detail: `(method) ${currentClass.name}.${methodName}(${params.join(', ')})`,
                        doc: `Method \`${methodName}\` of class \`${currentClass.name}\``,
                        params,
                        line: i,
                        containerName: currentClass.name
                    });
                }

                // Class constructor: init { takes(...) ... }
                if (line.startsWith('init') && line.includes('{')) {
                    const initParams = this.extractTakesParams(lines, i + 1);
                    const classSym = symbols.find(
                        s => s.name === currentClass?.name && s.kind === vscode.CompletionItemKind.Class
                    );
                    if (classSym) {
                        classSym.params = initParams;
                        classSym.detail = `class ${currentClass.name}(${initParams.join(', ')})`;
                    }
                }

                // Class field: this.fieldName
                const fieldMatch = line.match(/^this\.([a-zA-Z_][a-zA-Z0-9_]*)/);
                if (fieldMatch) {
                    const fieldName = fieldMatch[1];
                    symbols.push({
                        name: fieldName,
                        kind: vscode.CompletionItemKind.Field,
                        detail: `(field) ${currentClass.name}.${fieldName}`,
                        doc: `Field \`${fieldName}\` of class \`${currentClass.name}\``,
                        line: i,
                        containerName: currentClass.name
                    });
                }
            }

            // Parameter gateway takes(...) inside standalone function or constructor
            const takesMatch = line.match(/^takes\s*\(([^)]*)\)/);
            if (takesMatch) {
                const paramNames = takesMatch[1].split(',').map(p => p.trim()).filter(p => p.length > 0);
                for (const p of paramNames) {
                    symbols.push({
                        name: p,
                        kind: vscode.CompletionItemKind.Variable,
                        detail: `(parameter) ${p}`,
                        doc: `Parameter \`${p}\` defined on line ${i + 1}`,
                        line: i
                    });
                }
            }

            // Loop iterator: for item in coll
            const forInMatch = line.match(/for\s+([a-zA-Z_][a-zA-Z0-9_]*)\s+in\s+/);
            if (forInMatch) {
                const iterVar = forInMatch[1];
                symbols.push({
                    name: iterVar,
                    kind: vscode.CompletionItemKind.Variable,
                    detail: `(loop variable) ${iterVar}`,
                    doc: `Loop iterator \`${iterVar}\` defined on line ${i + 1}`,
                    line: i
                });
            }

            // Walrus variable declaration: x := value or a, b := val1, val2
            const walrusMatch = line.match(/^([a-zA-Z_][a-zA-Z0-9_]*(?:\s*,\s*[a-zA-Z_][a-zA-Z0-9_]*)*)\s*:=\s*(.*)$/);
            if (walrusMatch) {
                const varNames = walrusMatch[1].split(',').map(s => s.trim());
                const rhs = walrusMatch[2].trim();

                for (const v of varNames) {
                    if (v && v !== '_') {
                        let inferredType: string | undefined = undefined;
                        if (rhs.startsWith('[') && rhs.endsWith(']')) inferredType = 'list';
                        else if (rhs.startsWith('{') && rhs.endsWith('}')) inferredType = 'dict';
                        else if (rhs.startsWith('(') && rhs.endsWith(')')) inferredType = 'tuple';
                        else if (rhs.startsWith('"') && rhs.endsWith('"')) inferredType = 'string';
                        else if (rhs.startsWith('<') && rhs.endsWith('>')) inferredType = 'array';
                        else if (/^\d+$/.test(rhs)) inferredType = 'int';
                        else if (/^\d+\.\d+$/.test(rhs)) inferredType = 'double';
                        else if (rhs === 'true' || rhs === 'false') inferredType = 'bool';
                        else {
                            // Check if rhs matches a known class constructor: Dog(...)
                            const instMatch = rhs.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s*\(/);
                            if (instMatch) {
                                inferredType = instMatch[1];
                            }
                        }

                        symbols.push({
                            name: v,
                            kind: vscode.CompletionItemKind.Variable,
                            detail: `(variable) ${v}${inferredType ? `: ${inferredType}` : ''}`,
                            doc: `Variable \`${v}\` declared with \`:=\` on line ${i + 1}`,
                            line: i,
                            inferredType
                        });
                    }
                }
            }

            // Typed scalar declaration: I count = 10 or D pi = 3.14
            const typedDeclMatch = line.match(/^([IDBCS])\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
            if (typedDeclMatch) {
                const typeTok = typedDeclMatch[1];
                const varName = typedDeclMatch[2];
                const typeMap: Record<string, string> = { I: 'int', D: 'double', B: 'bool', C: 'char', S: 'string' };
                const infType = typeMap[typeTok];
                symbols.push({
                    name: varName,
                    kind: vscode.CompletionItemKind.Variable,
                    detail: `(${infType}) ${varName}`,
                    doc: `Variable \`${varName}\` of type \`${infType}\` declared on line ${i + 1}`,
                    line: i,
                    inferredType: infType
                });
            }

            // Collection declaration: tasks L or scores DICT or tags SET or grades SL or point T
            const collDeclMatch = line.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s+(L|T|DICT|SET|SL)\b/);
            if (collDeclMatch) {
                const varName = collDeclMatch[1];
                const typeTok = collDeclMatch[2];
                const typeMap: Record<string, string> = {
                    L: 'list',
                    T: 'tuple',
                    DICT: 'dict',
                    SET: 'set',
                    SL: 'sortedList'
                };
                const infType = typeMap[typeTok] || typeTok;
                symbols.push({
                    name: varName,
                    kind: vscode.CompletionItemKind.Variable,
                    detail: `(${infType}) ${varName}`,
                    doc: `Collection \`${varName}\` of type \`${infType}\` declared on line ${i + 1}`,
                    line: i,
                    inferredType: infType
                });
            }

            // Fixed Array declaration: numbers I 5 or primes I = <2, 3, 5, 7>
            const arrDeclMatch = line.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s+([IDBCS])\s+(\d+|=)/);
            if (arrDeclMatch) {
                const varName = arrDeclMatch[1];
                symbols.push({
                    name: varName,
                    kind: vscode.CompletionItemKind.Variable,
                    detail: `(array) ${varName}`,
                    doc: `Fixed array \`${varName}\` declared on line ${i + 1}`,
                    line: i,
                    inferredType: 'array'
                });
            }

            // Track braces to determine class end
            for (const ch of rawLine) {
                if (ch === '{') braceDepth++;
                if (ch === '}') {
                    braceDepth--;
                    if (braceDepth <= 0 && currentClass) {
                        currentClass = null;
                    }
                }
            }
        }

        return symbols;
    }

    private extractTakesParams(lines: string[], startLine: number): string[] {
        for (let i = startLine; i < Math.min(startLine + 6, lines.length); i++) {
            const line = lines[i].trim();
            const takesMatch = line.match(/takes\s*\(([^)]*)\)/);
            if (takesMatch) {
                return takesMatch[1]
                    .split(',')
                    .map(p => p.trim())
                    .filter(p => p.length > 0);
            }
            if (line.includes('}')) break;
        }
        return [];
    }

    private findEnclosingClass(
        document: vscode.TextDocument,
        currentLineNumber: number
    ): { name: string; fields: string[]; methods: { name: string; params: string[] }[] } | null {
        const lines = document.getText().split('\n');
        let currentClass: { name: string; startLine: number } | null = null;
        let braceDepth = 0;
        let classDepth = -1;

        for (let i = 0; i <= currentLineNumber; i++) {
            const line = lines[i];
            const classMatch = line.match(/^\s*class\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
            if (classMatch) {
                currentClass = { name: classMatch[1], startLine: i };
                classDepth = braceDepth;
            }
            for (const ch of line) {
                if (ch === '{') braceDepth++;
                if (ch === '}') {
                    braceDepth--;
                    if (classDepth !== -1 && braceDepth <= classDepth) {
                        currentClass = null;
                        classDepth = -1;
                    }
                }
            }
        }

        if (!currentClass) return null;
        return this.parseClassDetails(lines, currentClass.name, currentClass.startLine);
    }

    public findClassByName(
        document: vscode.TextDocument,
        className: string
    ): { name: string; fields: string[]; methods: { name: string; params: string[] }[] } | null {
        const lines = document.getText().split('\n');
        for (let i = 0; i < lines.length; i++) {
            const classMatch = lines[i].match(new RegExp(`^\\s*class\\s+${className}\\b`));
            if (classMatch) {
                return this.parseClassDetails(lines, className, i);
            }
        }
        return null;
    }

    private parseClassDetails(
        lines: string[],
        className: string,
        startLine: number
    ): { name: string; fields: string[]; methods: { name: string; params: string[] }[] } {
        const fields: string[] = [];
        const methods: { name: string; params: string[] }[] = [];
        let depth = 0;

        for (let i = startLine; i < lines.length; i++) {
            const line = lines[i].trim();

            for (const ch of line) {
                if (ch === '{') depth++;
                if (ch === '}') depth--;
            }

            if (i > startLine && depth <= 0) break;

            const fieldMatch = line.match(/^this\.([a-zA-Z_][a-zA-Z0-9_]*)/);
            if (fieldMatch) {
                fields.push(fieldMatch[1]);
            }
            const methodMatch = line.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s*\{/);
            if (
                methodMatch &&
                methodMatch[1] !== 'init' &&
                methodMatch[1] !== 'if' &&
                methodMatch[1] !== 'for' &&
                methodMatch[1] !== 'else' &&
                methodMatch[1] !== 'elif'
            ) {
                const params = this.extractTakesParams(lines, i + 1);
                methods.push({ name: methodMatch[1], params });
            }
        }

        return { name: className, fields, methods };
    }

    private getAutoImportEdits(document: vscode.TextDocument, modName: string): vscode.TextEdit[] {
        const text = document.getText();
        const importRegex = new RegExp(`^\\s*import\\s+[^;\\n]*\\b${modName}\\b`, 'm');
        if (importRegex.test(text)) {
            return [];
        }

        const lines = text.split('\n');
        let insertLine = 0;
        let foundExistingImport = false;

        for (let i = 0; i < lines.length; i++) {
            const l = lines[i].trim();
            if (l.startsWith('import ')) {
                insertLine = i + 1;
                foundExistingImport = true;
            } else if (!foundExistingImport && (l.startsWith('//') || l.startsWith('#') || l === '')) {
                insertLine = i + 1;
            } else {
                if (foundExistingImport) break;
            }
        }

        if (insertLine > lines.length) insertLine = lines.length;

        return [vscode.TextEdit.insert(new vscode.Position(insertLine, 0), `import ${modName}\n`)];
    }
}
