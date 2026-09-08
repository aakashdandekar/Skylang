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
exports.SkylangHoverProvider = void 0;
const vscode = __importStar(require("vscode"));
const keywords_1 = require("./data/keywords");
const builtins_1 = require("./data/builtins");
const stdlib_1 = require("./data/stdlib");
const completions_1 = require("./completions");
class SkylangHoverProvider {
    completionProvider = new completions_1.SkylangCompletionItemProvider();
    provideHover(document, position, _token) {
        const lineText = document.lineAt(position.line).text;
        // 1. Check for C Header hover in `cimport "header.h"`
        const headerRange = document.getWordRangeAtPosition(position, /[a-zA-Z0-9_\.]+\.h/);
        if (headerRange) {
            const header = document.getText(headerRange);
            if (stdlib_1.COMMON_C_HEADERS.includes(header)) {
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
            if (stdlib_1.STDLIB_MODULES[receiver]) {
                const mod = stdlib_1.STDLIB_MODULES[receiver];
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
            // Collection method (e.g. items.push, dict.has, str.value)
            if (builtins_1.COLLECTION_METHODS[member]) {
                const m = builtins_1.COLLECTION_METHODS[member];
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
            if (builtins_1.PROPERTIES[member]) {
                const p = builtins_1.PROPERTIES[member];
                const content = new vscode.MarkdownString();
                content.appendCodeblock(p.signature, 'skylang');
                content.appendMarkdown(`\n\n${p.description}\n\n**Returns:** \`${p.returns}\``);
                if (p.example) {
                    content.appendMarkdown(`\n\n\`\`\`skylang\n${p.example}\n\`\`\``);
                }
                return new vscode.Hover(content, memberRange);
            }
            // User class instance member or `this.field`
            const docSymbols = this.completionProvider.parseDocumentSymbols(document);
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
            }
            else {
                const varSym = docSymbols.find(s => s.name === receiver);
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
        if (!singleWordRange)
            return null;
        const word = document.getText(singleWordRange);
        // Keywords
        if (keywords_1.KEYWORDS[word]) {
            const kw = keywords_1.KEYWORDS[word];
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
        if (keywords_1.TYPES[word]) {
            const t = keywords_1.TYPES[word];
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
        if (keywords_1.CONSTANTS[word]) {
            const c = keywords_1.CONSTANTS[word];
            const content = new vscode.MarkdownString();
            content.appendMarkdown(`### ${c.name}\n\n`);
            content.appendMarkdown(`${c.description}\n`);
            if (c.example) {
                content.appendMarkdown(`\n**Example:**\n\`\`\`skylang\n${c.example}\n\`\`\``);
            }
            return new vscode.Hover(content, singleWordRange);
        }
        // Built-in Functions
        if (builtins_1.BUILTIN_FUNCTIONS[word]) {
            const fn = builtins_1.BUILTIN_FUNCTIONS[word];
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
        if (stdlib_1.STDLIB_MODULES[word]) {
            const mod = stdlib_1.STDLIB_MODULES[word];
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
            if (['python', 'js', 'cpp', 'java'].includes(word)) {
                const text = document.getText();
                const importRegex = new RegExp(`^\\s*import\\s+[^;\\n]*\\b${word}\\b`, 'm');
                if (!importRegex.test(text)) {
                    content.appendMarkdown(`\n\n> 💡 **Notice:** \`${word}\` is not yet imported in this file. Add \`import ${word}\` at the top of your file to use it.`);
                }
            }
            return new vscode.Hover(content, singleWordRange);
        }
        // Common C extern functions
        if (stdlib_1.COMMON_EXTERN_C_FUNCTIONS[word]) {
            const ext = stdlib_1.COMMON_EXTERN_C_FUNCTIONS[word];
            const content = new vscode.MarkdownString();
            content.appendCodeblock(ext.signature, 'skylang');
            content.appendMarkdown(`\n\n${ext.doc}\n\n**Header:** \`<${ext.header}>\``);
            return new vscode.Hover(content, singleWordRange);
        }
        // Collection Methods & Properties
        if (builtins_1.COLLECTION_METHODS[word]) {
            const m = builtins_1.COLLECTION_METHODS[word];
            const content = new vscode.MarkdownString();
            content.appendCodeblock(m.signature, 'skylang');
            content.appendMarkdown(`\n\n${m.description}\n\n**Returns:** \`${m.returns || 'none'}\``);
            if (m.example) {
                content.appendMarkdown(`\n\n\`\`\`skylang\n${m.example}\n\`\`\``);
            }
            return new vscode.Hover(content, singleWordRange);
        }
        if (builtins_1.PROPERTIES[word]) {
            const p = builtins_1.PROPERTIES[word];
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
            }
            return new vscode.Hover(content, singleWordRange);
        }
        return null;
    }
}
exports.SkylangHoverProvider = SkylangHoverProvider;
//# sourceMappingURL=hover.js.map