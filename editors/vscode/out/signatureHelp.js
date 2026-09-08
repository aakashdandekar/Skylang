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
exports.SkylangSignatureHelpProvider = void 0;
const vscode = __importStar(require("vscode"));
const builtins_1 = require("./data/builtins");
const stdlib_1 = require("./data/stdlib");
const completions_1 = require("./completions");
class SkylangSignatureHelpProvider {
    completionProvider = new completions_1.SkylangCompletionItemProvider();
    provideSignatureHelp(document, position, _token, _context) {
        const textBeforeCursor = document.getText(new vscode.Range(new vscode.Position(0, 0), position));
        const callInfo = this.getCallInfo(textBeforeCursor);
        if (!callInfo)
            return null;
        const { callee, activeParam, namedParam } = callInfo;
        const help = new vscode.SignatureHelp();
        help.activeParameter = activeParam;
        help.activeSignature = 0;
        // 1. Stdlib / Interop module calls (e.g. math.sqrt, io.readfile, python.load)
        if (callee.includes('.')) {
            const parts = callee.split('.');
            const modOrVar = parts[0];
            const fnName = parts[1];
            if (stdlib_1.STDLIB_MODULES[modOrVar] && stdlib_1.STDLIB_MODULES[modOrVar].functions[fnName]) {
                const fn = stdlib_1.STDLIB_MODULES[modOrVar].functions[fnName];
                const sig = new vscode.SignatureInformation(fn.signature, new vscode.MarkdownString(fn.description));
                if (fn.params) {
                    sig.parameters = fn.params.map(p => new vscode.ParameterInformation(p.name, p.doc));
                }
                help.signatures = [sig];
                return help;
            }
            if (builtins_1.COLLECTION_METHODS[fnName]) {
                const m = builtins_1.COLLECTION_METHODS[fnName];
                const sig = new vscode.SignatureInformation(m.signature, new vscode.MarkdownString(m.description));
                if (m.params) {
                    sig.parameters = m.params.map(p => new vscode.ParameterInformation(p.name, p.doc));
                }
                help.signatures = [sig];
                return help;
            }
            // Check if modOrVar is an instance of a user class: acc.deposit(...)
            const parsedSymbols = this.completionProvider.parseDocumentSymbols(document);
            const varSym = parsedSymbols.find(s => s.name === modOrVar);
            if (varSym && varSym.inferredType) {
                const classDef = this.completionProvider.findClassByName(document, varSym.inferredType);
                if (classDef) {
                    const method = classDef.methods.find(m => m.name === fnName);
                    if (method) {
                        const sigStr = `${fnName}(${method.params.join(', ')})`;
                        const sig = new vscode.SignatureInformation(sigStr, new vscode.MarkdownString(`Method \`${fnName}\` of class \`${classDef.name}\``));
                        if (method.params.length > 0) {
                            sig.parameters = method.params.map(p => new vscode.ParameterInformation(p, `Parameter ${p}`));
                            if (namedParam) {
                                const idx = method.params.indexOf(namedParam);
                                if (idx !== -1)
                                    help.activeParameter = idx;
                            }
                        }
                        help.signatures = [sig];
                        return help;
                    }
                }
            }
        }
        // 2. Built-in functions (print, println, range, len, type, takes, error, panic, gc, free)
        if (builtins_1.BUILTIN_FUNCTIONS[callee]) {
            const fn = builtins_1.BUILTIN_FUNCTIONS[callee];
            const sig = new vscode.SignatureInformation(fn.signature, new vscode.MarkdownString(fn.description));
            if (fn.params) {
                sig.parameters = fn.params.map(p => new vscode.ParameterInformation(p.name, p.doc));
            }
            help.signatures = [sig];
            return help;
        }
        // 3. Common C extern functions
        if (stdlib_1.COMMON_EXTERN_C_FUNCTIONS[callee]) {
            const ext = stdlib_1.COMMON_EXTERN_C_FUNCTIONS[callee];
            const sig = new vscode.SignatureInformation(ext.signature, new vscode.MarkdownString(ext.doc));
            help.signatures = [sig];
            return help;
        }
        // 4. User-defined functions, constructors, or classes
        const symbols = this.completionProvider.parseDocumentSymbols(document);
        const match = symbols.find(s => s.name === callee);
        if (match) {
            let sigText = match.detail;
            if (match.kind === vscode.CompletionItemKind.Class) {
                sigText = `${match.name}(${match.params ? match.params.join(', ') : ''})`;
            }
            const sig = new vscode.SignatureInformation(sigText, new vscode.MarkdownString(match.doc || ''));
            if (match.params && match.params.length > 0) {
                sig.parameters = match.params.map(p => new vscode.ParameterInformation(p, `Parameter ${p}`));
                if (namedParam) {
                    const idx = match.params.indexOf(namedParam);
                    if (idx !== -1)
                        help.activeParameter = idx;
                }
            }
            help.signatures = [sig];
            return help;
        }
        return null;
    }
    getCallInfo(text) {
        let parenDepth = 0;
        let commaCount = 0;
        let inString = false;
        let stringChar = '';
        let lastCommaPos = -1;
        for (let i = text.length - 1; i >= 0; i--) {
            const ch = text[i];
            const prev = i > 0 ? text[i - 1] : '';
            if (inString) {
                if (ch === stringChar && prev !== '\\') {
                    inString = false;
                }
                continue;
            }
            if (ch === '"' || ch === '\'') {
                inString = true;
                stringChar = ch;
                continue;
            }
            if (ch === ')') {
                parenDepth++;
            }
            else if (ch === '(') {
                if (parenDepth > 0) {
                    parenDepth--;
                }
                else {
                    // Found opening parenthesis of current call
                    const beforeParen = text.substring(0, i).trim();
                    const calleeMatch = beforeParen.match(/([a-zA-Z_][a-zA-Z0-9_\.]*)$/);
                    if (!calleeMatch)
                        return null;
                    // Extract current param text to detect named argument `param=`
                    const paramSlice = text.substring(lastCommaPos !== -1 ? lastCommaPos + 1 : i + 1).trim();
                    const namedMatch = paramSlice.match(/^([a-zA-Z_][a-zA-Z0-9_]*)\s*=/);
                    const namedParam = namedMatch ? namedMatch[1] : undefined;
                    return {
                        callee: calleeMatch[1],
                        activeParam: commaCount,
                        namedParam
                    };
                }
            }
            else if (ch === ',' && parenDepth === 0) {
                if (lastCommaPos === -1) {
                    lastCommaPos = i;
                }
                commaCount++;
            }
        }
        return null;
    }
}
exports.SkylangSignatureHelpProvider = SkylangSignatureHelpProvider;
//# sourceMappingURL=signatureHelp.js.map