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
exports.SkylangCompletionItemProvider = void 0;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
const fs = __importStar(require("fs"));
const keywords_1 = require("./data/keywords");
const builtins_1 = require("./data/builtins");
const stdlib_1 = require("./data/stdlib");
class SkylangCompletionItemProvider {
    async provideCompletionItems(document, position, token, context) {
        const lineText = document.lineAt(position.line).text;
        const linePrefix = lineText.substring(0, position.character);
        // Check if inside a comment
        if (this.isInsideComment(lineText, position.character)) {
            return [];
        }
        const items = [];
        // 1. Context: cimport "..."
        const cimportMatch = linePrefix.match(/(?:^|\s)cimport\s+([^;\n]*)$/);
        if (cimportMatch) {
            const rawList = cimportMatch[1];
            const parts = rawList.split(',').map(s => s.trim().replace(/^["']|["']$/g, '').toLowerCase());
            const alreadyImported = parts.slice(0, -1);
            for (const header of stdlib_1.COMMON_C_HEADERS) {
                if (alreadyImported.includes(header.toLowerCase()))
                    continue;
                const item = new vscode.CompletionItem(header, vscode.CompletionItemKind.File);
                item.detail = `C standard header <${header}>`;
                item.documentation = new vscode.MarkdownString(`Import C standard library header \`${header}\` for native FFI binding.`);
                item.insertText = header;
                item.sortText = `0_${header}`;
                items.push(item);
            }
            return items;
        }
        // 2. Context: extern f ...
        const externMatch = linePrefix.match(/extern\s+f\s+([a-zA-Z0-9_]*)$/);
        if (externMatch) {
            for (const [fnName, fnInfo] of Object.entries(stdlib_1.COMMON_EXTERN_C_FUNCTIONS)) {
                const item = new vscode.CompletionItem(fnName, vscode.CompletionItemKind.Function);
                item.detail = fnInfo.signature;
                item.documentation = new vscode.MarkdownString(`${fnInfo.doc}\n\n**Header:** \`${fnInfo.header}\``);
                item.insertText = new vscode.SnippetString(`${fnInfo.signature.replace('extern f ', '')}`);
                item.sortText = `0_${fnName}`;
                items.push(item);
            }
            return items;
        }
        // 3. Context: from <module> import <symbols>
        const fromImportMatch = linePrefix.match(/(?:^|\s)from\s+([a-zA-Z0-9_\.\/"']+)\s+import\s+([^;\n]*)$/);
        if (fromImportMatch) {
            const modName = fromImportMatch[1].trim();
            const rawSymbols = fromImportMatch[2];
            const symbolItems = [];
            // If user typed `symbol `, offer `as`
            const lastPart = rawSymbols.split(',').pop()?.trim() || '';
            if (lastPart.length > 0 && !lastPart.includes(' ') && linePrefix.endsWith(' ')) {
                const asItem = new vscode.CompletionItem('as', vscode.CompletionItemKind.Keyword);
                asItem.detail = 'as (Import alias)';
                asItem.insertText = 'as ';
                asItem.sortText = '0000_as';
                symbolItems.push(asItem);
            }
            // Wildcard *
            const starItem = new vscode.CompletionItem('*', vscode.CompletionItemKind.Keyword);
            starItem.detail = 'Wildcard import all exported symbols';
            starItem.documentation = new vscode.MarkdownString(`Import all functions, classes, and variables from \`${modName}\` into the current scope.`);
            starItem.insertText = '*';
            starItem.sortText = '0001_*';
            symbolItems.push(starItem);
            const modDoc = await this.resolveModuleDocument(document, modName);
            if (modDoc) {
                const docSymbols = this.parseDocumentSymbols(modDoc);
                for (const sym of docSymbols) {
                    if (sym.kind === vscode.CompletionItemKind.Function ||
                        sym.kind === vscode.CompletionItemKind.Class ||
                        sym.kind === vscode.CompletionItemKind.Variable) {
                        const item = new vscode.CompletionItem(sym.name, sym.kind);
                        item.detail = `${sym.detail} (from ${modName})`;
                        item.insertText = sym.name;
                        item.sortText = `0002_${sym.name}`;
                        symbolItems.push(item);
                    }
                }
            }
            return new vscode.CompletionList(symbolItems, false);
        }
        // 4. Context: from <module>
        const fromModMatch = linePrefix.match(/(?:^|\s)from\s+([^;\n]*)$/);
        if (fromModMatch) {
            const rawText = fromModMatch[1].trim();
            // If user typed a module name and trailing space, suggest `import`
            if (rawText.length > 0 && !rawText.includes(' ') && linePrefix.endsWith(' ')) {
                const importItem = new vscode.CompletionItem('import', vscode.CompletionItemKind.Keyword);
                importItem.detail = 'import keyword';
                importItem.insertText = 'import ';
                importItem.sortText = '0000_import';
                return new vscode.CompletionList([importItem], false);
            }
            const available = await this.findAvailableModules(document, token);
            const fromItems = [];
            for (const mod of available) {
                const item = new vscode.CompletionItem(mod.name, mod.isFile ? vscode.CompletionItemKind.File : vscode.CompletionItemKind.Module);
                item.detail = mod.detail;
                item.insertText = mod.name;
                item.sortText = `0001_${mod.name}`;
                fromItems.push(item);
            }
            return new vscode.CompletionList(fromItems, false);
        }
        // 5. Context: import mod1, mod2, ...
        const importMatch = linePrefix.match(/(?:^|\s)import\s+([^;\n]*)$/);
        if (importMatch) {
            const rawList = importMatch[1];
            const parts = rawList.split(',');
            const lastPart = parts[parts.length - 1].trim();
            const importItems = [];
            // If user typed a module name and trailing space, suggest `as`
            if (lastPart.length > 0 && !lastPart.includes(' ') && linePrefix.endsWith(' ')) {
                const asItem = new vscode.CompletionItem('as', vscode.CompletionItemKind.Keyword);
                asItem.detail = 'as (Import alias)';
                asItem.insertText = 'as ';
                asItem.sortText = '0000_as';
                importItems.push(asItem);
            }
            const available = await this.findAvailableModules(document, token);
            const alreadyImported = parts.slice(0, -1).map(p => p.trim().split(/\s+as\s+/)[0]);
            for (const mod of available) {
                if (alreadyImported.includes(mod.name))
                    continue;
                const modDoc = stdlib_1.STDLIB_MODULES[mod.name];
                const item = new vscode.CompletionItem(mod.name, mod.isFile ? vscode.CompletionItemKind.File : vscode.CompletionItemKind.Module);
                item.detail = modDoc ? modDoc.name : mod.detail;
                item.documentation = new vscode.MarkdownString(modDoc ? modDoc.description : `Import module \`${mod.name}\`.`);
                item.insertText = mod.name;
                if (lastPart.length > 0 && mod.name.toLowerCase().startsWith(lastPart.toLowerCase())) {
                    item.sortText = `0001_${mod.name}`;
                    item.preselect = true;
                }
                else {
                    item.sortText = `0002_${mod.name}`;
                }
                importItems.push(item);
            }
            return new vscode.CompletionList(importItems, false);
        }
        // 4. Context: python.load("..."), java.load("..."), js.load("..."), go.load("..."), rust.load("...")
        const pyLoadMatch = linePrefix.match(/python\.load\s*\(\s*["']([^"']*)$/);
        if (pyLoadMatch) {
            for (const pkg of stdlib_1.COMMON_PYTHON_PACKAGES) {
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
            for (const cls of stdlib_1.COMMON_JAVA_CLASSES) {
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
            for (const pkg of stdlib_1.COMMON_NPM_PACKAGES) {
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
            for (const pkg of stdlib_1.COMMON_GO_PACKAGES) {
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
            for (const crate of stdlib_1.COMMON_RUST_CRATES) {
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
            if (stdlib_1.STDLIB_MODULES[receiver]) {
                const mod = stdlib_1.STDLIB_MODULES[receiver];
                for (const [fnName, fnDoc] of Object.entries(mod.functions)) {
                    const item = new vscode.CompletionItem(fnName, vscode.CompletionItemKind.Function);
                    item.detail = fnDoc.signature;
                    item.documentation = new vscode.MarkdownString(`${fnDoc.description}\n\n**Returns:** \`${fnDoc.returns || 'none'}\`${fnDoc.example ? `\n\n\`\`\`skylang\n${fnDoc.example}\n\`\`\`` : ''}`);
                    item.insertText = new vscode.SnippetString(this.generateSnippet(fnName, fnDoc.params));
                    item.sortText = `0_${fnName}`;
                    items.push(item);
                }
                if (mod.constants) {
                    for (const [constName, constDoc] of Object.entries(mod.constants)) {
                        const item = new vscode.CompletionItem(constName, vscode.CompletionItemKind.Constant);
                        item.detail = constDoc.signature;
                        item.documentation = new vscode.MarkdownString(`${constDoc.description}\n\n**Type:** \`${constDoc.returns || 'double'}\`${constDoc.example ? `\n\n\`\`\`skylang\n${constDoc.example}\n\`\`\`` : ''}`);
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
                if (builtins_1.CONTAINER_TYPE_MEMBERS[infType]) {
                    const memberDefs = builtins_1.CONTAINER_TYPE_MEMBERS[infType];
                    for (const mName of memberDefs.methods) {
                        if (builtins_1.COLLECTION_METHODS[mName]) {
                            const mDoc = builtins_1.COLLECTION_METHODS[mName];
                            const item = new vscode.CompletionItem(mName, vscode.CompletionItemKind.Method);
                            item.detail = mDoc.signature;
                            item.documentation = new vscode.MarkdownString(`${mDoc.description}\n\n**Returns:** \`${mDoc.returns || 'none'}\`${mDoc.example ? `\n\n\`\`\`skylang\n${mDoc.example}\n\`\`\`` : ''}`);
                            item.insertText = new vscode.SnippetString(this.generateSnippet(mName, mDoc.params));
                            item.sortText = `0_${mName}`;
                            items.push(item);
                        }
                    }
                    for (const pName of memberDefs.properties) {
                        if (builtins_1.PROPERTIES[pName]) {
                            const pDoc = builtins_1.PROPERTIES[pName];
                            const item = new vscode.CompletionItem(pName, vscode.CompletionItemKind.Property);
                            item.detail = pDoc.signature;
                            item.documentation = new vscode.MarkdownString(`${pDoc.description}\n\n**Returns:** \`${pDoc.returns || 'type'}\``);
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
                    const tProp = builtins_1.PROPERTIES['T'];
                    const tItem = new vscode.CompletionItem('T', vscode.CompletionItemKind.Property);
                    tItem.detail = tProp.signature;
                    tItem.documentation = new vscode.MarkdownString(tProp.description);
                    tItem.sortText = `2_T`;
                    items.push(tItem);
                    return items;
                }
            }
            // D. Fallback: Generic Collection Methods & Properties
            for (const [methodName, methodDoc] of Object.entries(builtins_1.COLLECTION_METHODS)) {
                const item = new vscode.CompletionItem(methodName, vscode.CompletionItemKind.Method);
                item.detail = methodDoc.signature;
                item.documentation = new vscode.MarkdownString(`${methodDoc.description}\n\n**Returns:** \`${methodDoc.returns || 'none'}\`${methodDoc.example ? `\n\n\`\`\`skylang\n${methodDoc.example}\n\`\`\`` : ''}`);
                item.insertText = new vscode.SnippetString(this.generateSnippet(methodName, methodDoc.params));
                item.sortText = `0_${methodName}`;
                items.push(item);
            }
            for (const [propName, propDoc] of Object.entries(builtins_1.PROPERTIES)) {
                const item = new vscode.CompletionItem(propName, vscode.CompletionItemKind.Property);
                item.detail = propDoc.signature;
                item.documentation = new vscode.MarkdownString(`${propDoc.description}\n\n**Returns:** \`${propDoc.returns || 'type'}\``);
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
            let params = [];
            // Builtin function parameter names
            if (builtins_1.BUILTIN_FUNCTIONS[rawCallee] && builtins_1.BUILTIN_FUNCTIONS[rawCallee].params) {
                params = builtins_1.BUILTIN_FUNCTIONS[rawCallee].params.map(p => p.name).filter(n => !n.startsWith('...'));
            }
            // User function or class constructor
            if (params.length === 0) {
                const targetSymbol = parsedSymbols.find(s => s.name === rawCallee &&
                    (s.kind === vscode.CompletionItemKind.Function ||
                        s.kind === vscode.CompletionItemKind.Class ||
                        s.kind === vscode.CompletionItemKind.Method));
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
        for (const [kw, doc] of Object.entries(keywords_1.KEYWORDS)) {
            const item = new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword);
            item.detail = doc.name;
            item.documentation = new vscode.MarkdownString(`${doc.description}\n\n**Syntax:** \`${doc.syntax}\`${doc.example ? `\n\n\`\`\`skylang\n${doc.example}\n\`\`\`` : ''}`);
            item.sortText = `3_${kw}`;
            items.push(item);
        }
        // 8. Built-in Types (I, D, B, C, S, L, T, DICT, SET, SL, int, double, etc.)
        for (const [typeName, doc] of Object.entries(keywords_1.TYPES)) {
            const item = new vscode.CompletionItem(typeName, vscode.CompletionItemKind.TypeParameter);
            item.detail = doc.name;
            item.documentation = new vscode.MarkdownString(`${doc.description}\n\n**Syntax:** \`${doc.syntax}\`${doc.example ? `\n\n\`\`\`skylang\n${doc.example}\n\`\`\`` : ''}`);
            item.sortText = `4_${typeName}`;
            items.push(item);
        }
        // 9. Constants & Special Identifiers (true, false, none, nil, args, _)
        for (const [cName, doc] of Object.entries(keywords_1.CONSTANTS)) {
            const item = new vscode.CompletionItem(cName, vscode.CompletionItemKind.Constant);
            item.detail = doc.name;
            item.documentation = new vscode.MarkdownString(`${doc.description}${doc.example ? `\n\n\`\`\`skylang\n${doc.example}\n\`\`\`` : ''}`);
            item.sortText = `2_${cName}`;
            items.push(item);
        }
        // 10. Built-in Functions (print, println, range, len, type, takes, error, panic, gc, free)
        for (const [fnName, fnDoc] of Object.entries(builtins_1.BUILTIN_FUNCTIONS)) {
            const item = new vscode.CompletionItem(fnName, vscode.CompletionItemKind.Function);
            item.detail = fnDoc.signature;
            item.documentation = new vscode.MarkdownString(`${fnDoc.description}\n\n**Returns:** \`${fnDoc.returns || 'none'}\`${fnDoc.example ? `\n\n\`\`\`skylang\n${fnDoc.example}\n\`\`\`` : ''}`);
            item.insertText = new vscode.SnippetString(this.generateSnippet(fnName, fnDoc.params));
            item.sortText = `1_${fnName}`;
            items.push(item);
        }
        // 11. Standard Library & Interop Modules (math, io, fmt, python, js, cpp, java, go, rust)
        const interopModuleSet = new Set(['python', 'js', 'cpp', 'java', 'go', 'golang', 'rust']);
        for (const [modName, modDoc] of Object.entries(stdlib_1.STDLIB_MODULES)) {
            const item = new vscode.CompletionItem(modName, vscode.CompletionItemKind.Module);
            item.detail = modDoc.name;
            item.documentation = new vscode.MarkdownString(modDoc.description);
            if (interopModuleSet.has(modName)) {
                item.additionalTextEdits = this.getAutoImportEdits(document, modName);
            }
            item.sortText = `00_${modName}`;
            items.push(item);
        }
        // 12. Document Defined Symbols (Functions, Classes, Methods, Variables, Parameters)
        const docSymbols = this.parseDocumentSymbols(document);
        for (const sym of docSymbols) {
            // Avoid duplicate with keywords
            if (keywords_1.KEYWORDS[sym.name] || keywords_1.TYPES[sym.name] || builtins_1.BUILTIN_FUNCTIONS[sym.name] || keywords_1.CONSTANTS[sym.name])
                continue;
            const item = new vscode.CompletionItem(sym.name, sym.kind);
            item.detail = sym.detail;
            if (sym.doc) {
                item.documentation = new vscode.MarkdownString(sym.doc);
            }
            if (sym.kind === vscode.CompletionItemKind.Function || sym.kind === vscode.CompletionItemKind.Class) {
                if (sym.params && sym.params.length > 0) {
                    item.insertText = new vscode.SnippetString(this.generateSnippetFromParamNames(sym.name, sym.params));
                }
                else {
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
                if (file.toString() === document.uri.toString())
                    continue;
                const otherDoc = await vscode.workspace.openTextDocument(file);
                const otherSymbols = this.parseDocumentSymbols(otherDoc);
                for (const sym of otherSymbols) {
                    if (sym.kind === vscode.CompletionItemKind.Function ||
                        sym.kind === vscode.CompletionItemKind.Class) {
                        if (!items.some(i => i.label === sym.name)) {
                            const item = new vscode.CompletionItem(sym.name, sym.kind);
                            item.detail = `${sym.detail} (from ${vscode.workspace.asRelativePath(file)})`;
                            item.documentation = new vscode.MarkdownString(sym.doc || '');
                            if (sym.params && sym.params.length > 0) {
                                item.insertText = new vscode.SnippetString(this.generateSnippetFromParamNames(sym.name, sym.params));
                            }
                            else {
                                item.insertText = new vscode.SnippetString(`${sym.name}()`);
                            }
                            item.sortText = `5_${sym.name}`;
                            items.push(item);
                        }
                    }
                }
            }
        }
        catch {
            // Workspace search is optional
        }
        return items;
    }
    generateSnippet(name, params) {
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
        if (name === 'open') {
            return `open("\${1:filepath}", "\${2:r}")`;
        }
        if (name === 'input') {
            return `input("\${1:prompt}")`;
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
    generateSnippetFromParamNames(name, paramNames) {
        if (!paramNames || paramNames.length === 0) {
            return `${name}()`;
        }
        const paramSnippets = paramNames.map((_, idx) => `\${${idx + 1}}`);
        return `${name}(${paramSnippets.join(', ')})`;
    }
    isInsideComment(lineText, col) {
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
            }
            else if (ch === '\'' && prev !== '\\' && !inDoubleQuote) {
                inSingleQuote = !inSingleQuote;
            }
        }
        return false;
    }
    isInsideString(lineText, col) {
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
            }
            else if (ch === '\'' && prev !== '\\' && !inDoubleQuote) {
                inSingleQuote = !inSingleQuote;
            }
        }
        return inSingleQuote || inDoubleQuote;
    }
    parseDocumentSymbols(document) {
        const symbols = [];
        const text = document.getText();
        const lines = text.split('\n');
        let currentClass = null;
        let braceDepth = 0;
        for (let i = 0; i < lines.length; i++) {
            const rawLine = lines[i];
            const line = rawLine.replace(/\/\/.*$/, '').replace(/#.*$/, '').trim();
            if (!line)
                continue;
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
                if (methodMatch &&
                    methodMatch[1] !== 'init' &&
                    methodMatch[1] !== 'if' &&
                    methodMatch[1] !== 'for' &&
                    methodMatch[1] !== 'else' &&
                    methodMatch[1] !== 'elif') {
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
                    const classSym = symbols.find(s => s.name === currentClass?.name && s.kind === vscode.CompletionItemKind.Class);
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
                        let inferredType = undefined;
                        if (rhs.startsWith('[') && rhs.endsWith(']'))
                            inferredType = 'list';
                        else if (rhs.startsWith('{') && rhs.endsWith('}'))
                            inferredType = 'dict';
                        else if (rhs.startsWith('(') && rhs.endsWith(')'))
                            inferredType = 'tuple';
                        else if (rhs.startsWith('"') && rhs.endsWith('"'))
                            inferredType = 'string';
                        else if (rhs.startsWith('<') && rhs.endsWith('>'))
                            inferredType = 'array';
                        else if (rhs.startsWith('open(') || rhs.startsWith('open ('))
                            inferredType = 'file';
                        else if (/^\d+$/.test(rhs))
                            inferredType = 'int';
                        else if (/^\d+\.\d+$/.test(rhs))
                            inferredType = 'double';
                        else if (rhs === 'true' || rhs === 'false')
                            inferredType = 'bool';
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
                const typeMap = { I: 'int', D: 'double', B: 'bool', C: 'char', S: 'string' };
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
                const typeMap = {
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
                if (ch === '{')
                    braceDepth++;
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
    extractTakesParams(lines, startLine) {
        for (let i = startLine; i < Math.min(startLine + 6, lines.length); i++) {
            const line = lines[i].trim();
            const takesMatch = line.match(/takes\s*\(([^)]*)\)/);
            if (takesMatch) {
                return takesMatch[1]
                    .split(',')
                    .map(p => p.trim())
                    .filter(p => p.length > 0);
            }
            if (line.includes('}'))
                break;
        }
        return [];
    }
    findEnclosingClass(document, currentLineNumber) {
        const lines = document.getText().split('\n');
        let currentClass = null;
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
                if (ch === '{')
                    braceDepth++;
                if (ch === '}') {
                    braceDepth--;
                    if (classDepth !== -1 && braceDepth <= classDepth) {
                        currentClass = null;
                        classDepth = -1;
                    }
                }
            }
        }
        if (!currentClass)
            return null;
        return this.parseClassDetails(lines, currentClass.name, currentClass.startLine);
    }
    findClassByName(document, className) {
        const lines = document.getText().split('\n');
        for (let i = 0; i < lines.length; i++) {
            const classMatch = lines[i].match(new RegExp(`^\\s*class\\s+${className}\\b`));
            if (classMatch) {
                return this.parseClassDetails(lines, className, i);
            }
        }
        return null;
    }
    parseClassDetails(lines, className, startLine) {
        const fields = [];
        const methods = [];
        let depth = 0;
        for (let i = startLine; i < lines.length; i++) {
            const line = lines[i].trim();
            for (const ch of line) {
                if (ch === '{')
                    depth++;
                if (ch === '}')
                    depth--;
            }
            if (i > startLine && depth <= 0)
                break;
            const fieldMatch = line.match(/^this\.([a-zA-Z_][a-zA-Z0-9_]*)/);
            if (fieldMatch) {
                fields.push(fieldMatch[1]);
            }
            const methodMatch = line.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s*\{/);
            if (methodMatch &&
                methodMatch[1] !== 'init' &&
                methodMatch[1] !== 'if' &&
                methodMatch[1] !== 'for' &&
                methodMatch[1] !== 'else' &&
                methodMatch[1] !== 'elif') {
                const params = this.extractTakesParams(lines, i + 1);
                methods.push({ name: methodMatch[1], params });
            }
        }
        return { name: className, fields, methods };
    }
    getAutoImportEdits(document, modName) {
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
            if (l.startsWith('import ') || l.startsWith('cimport ')) {
                insertLine = i + 1;
                foundExistingImport = true;
            }
            else if (!foundExistingImport && (l.startsWith('//') || l.startsWith('#') || l === '')) {
                insertLine = i + 1;
            }
            else {
                if (foundExistingImport)
                    break;
            }
        }
        if (insertLine > lines.length)
            insertLine = lines.length;
        return [vscode.TextEdit.insert(new vscode.Position(insertLine, 0), `import ${modName}\n`)];
    }
    async resolveModuleDocument(document, modulePath) {
        const cleanPath = modulePath.trim().replace(/^["']|["']$/g, '');
        const relFilePath = cleanPath.endsWith('.sky') ? cleanPath : cleanPath.replace(/\./g, '/') + '.sky';
        // 1. Try relative to current document folder
        try {
            const currentDir = path.dirname(document.uri.fsPath);
            const resolvedPath = path.resolve(currentDir, relFilePath);
            if (fs.existsSync(resolvedPath)) {
                return await vscode.workspace.openTextDocument(vscode.Uri.file(resolvedPath));
            }
        }
        catch {
            // ignore
        }
        // 2. Try workspace root / examples
        try {
            const workspaceFolder = vscode.workspace.getWorkspaceFolder(document.uri);
            if (workspaceFolder) {
                const rootCandidate = path.resolve(workspaceFolder.uri.fsPath, relFilePath);
                if (fs.existsSync(rootCandidate)) {
                    return await vscode.workspace.openTextDocument(vscode.Uri.file(rootCandidate));
                }
                const examplesCandidate = path.resolve(workspaceFolder.uri.fsPath, 'examples', relFilePath);
                if (fs.existsSync(examplesCandidate)) {
                    return await vscode.workspace.openTextDocument(vscode.Uri.file(examplesCandidate));
                }
            }
        }
        catch {
            // ignore
        }
        return null;
    }
    async findAvailableModules(document, token) {
        const result = [];
        const seen = new Set();
        const bridges = ['python', 'js', 'cpp', 'java', 'go', 'golang', 'rust'];
        for (const b of bridges) {
            seen.add(b);
            result.push({ name: b, isFile: false, detail: `Language bridge: ${b}` });
        }
        try {
            const workspaceFiles = await vscode.workspace.findFiles('**/*.sky', '**/node_modules/**', 50, token);
            const currentDir = path.dirname(document.uri.fsPath);
            const wsFolder = vscode.workspace.getWorkspaceFolder(document.uri);
            for (const file of workspaceFiles) {
                if (file.toString() === document.uri.toString())
                    continue;
                const fsPath = file.fsPath;
                // Rel to current dir
                const relToCurrent = path.relative(currentDir, fsPath).replace(/\\/g, '/');
                if (!relToCurrent.startsWith('../') && relToCurrent.endsWith('.sky')) {
                    const modName = relToCurrent.slice(0, -4).replace(/\//g, '.');
                    if (!seen.has(modName)) {
                        seen.add(modName);
                        result.push({ name: modName, isFile: true, detail: `Local module: ${modName}` });
                    }
                }
                // Rel to workspace root
                if (wsFolder) {
                    const relToRoot = path.relative(wsFolder.uri.fsPath, fsPath).replace(/\\/g, '/');
                    if (relToRoot.endsWith('.sky') && !relToRoot.startsWith('../')) {
                        const modName = relToRoot.slice(0, -4).replace(/\//g, '.');
                        if (!seen.has(modName)) {
                            seen.add(modName);
                            result.push({ name: modName, isFile: true, detail: `Workspace module: ${modName}` });
                        }
                    }
                }
            }
        }
        catch {
            // ignore
        }
        return result;
    }
}
exports.SkylangCompletionItemProvider = SkylangCompletionItemProvider;
//# sourceMappingURL=completions.js.map