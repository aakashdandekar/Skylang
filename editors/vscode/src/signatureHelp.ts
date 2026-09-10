import * as vscode from 'vscode';
import { BUILTIN_FUNCTIONS, COLLECTION_METHODS } from './data/builtins';
import { STDLIB_MODULES, COMMON_EXTERN_C_FUNCTIONS } from './data/stdlib';
import { getForeignModule, ForeignModuleDoc } from './data/foreign';
import { SkylangCompletionItemProvider } from './completions';
import { ForeignLspBridge, ForeignBridgeType } from './foreignLspBridge';

export class SkylangSignatureHelpProvider implements vscode.SignatureHelpProvider {
    private completionProvider = new SkylangCompletionItemProvider();

    public async provideSignatureHelp(
        document: vscode.TextDocument,
        position: vscode.Position,
        _token: vscode.CancellationToken,
        _context: vscode.SignatureHelpContext
    ): Promise<vscode.SignatureHelp | null> {
        // 0. Check for Embedded Foreign Code signature help (python.exec, cpp.compile, etc.)
        const embeddedContext = this.completionProvider.getEmbeddedCodeContext(document, position);
        if (embeddedContext) {
            const lspBridge = ForeignLspBridge.getInstance();
            if (lspBridge && lspBridge.isLanguageExtensionAvailable(embeddedContext.bridge)) {
                const shadowDoc = lspBridge.createShadowDocumentForEmbeddedCode(
                    embeddedContext.bridge,
                    embeddedContext.code,
                    embeddedContext.offset
                );
                const lspHelp = await lspBridge.querySignatureHelp(shadowDoc, '(');
                if (lspHelp) return lspHelp;
            }
        }

        const textBeforeCursor = document.getText(new vscode.Range(new vscode.Position(0, 0), position));
        const callInfo = this.getCallInfo(textBeforeCursor);
        if (!callInfo) return null;

        const { callee, activeParam, namedParam } = callInfo;

        const help = new vscode.SignatureHelp();
        help.activeParameter = activeParam;
        help.activeSignature = 0;

        // 1. Stdlib / Interop module calls (e.g. math.sqrt, io.readfile, python.load)
        if (callee.includes('.')) {
            const parts = callee.split('.');
            const modOrVar = parts[0];
            const fnName = parts[1];

            if (STDLIB_MODULES[modOrVar] && STDLIB_MODULES[modOrVar].functions[fnName]) {
                const fn = STDLIB_MODULES[modOrVar].functions[fnName];
                const sig = new vscode.SignatureInformation(fn.signature, new vscode.MarkdownString(fn.description));
                if (fn.params) {
                    sig.parameters = fn.params.map(p => new vscode.ParameterInformation(p.name, p.doc));
                }
                help.signatures = [sig];
                return help;
            }

            // Foreign Interop module call (e.g. express.get(...), app.listen(...), np.array(...))
            const parsedSymbols = this.completionProvider.parseDocumentSymbols(document);
            const varSym = parsedSymbols.find(s => s.name === modOrVar);
            let foreignMod: ForeignModuleDoc | undefined = undefined;
            if (varSym && varSym.inferredType && varSym.inferredType.startsWith('foreign_')) {
                const matchBridge = varSym.inferredType.match(/^foreign_([a-z]+):(.*)$/);
                const bridge = (matchBridge ? matchBridge[1] : 'js') as ForeignBridgeType;
                const rawPkg = matchBridge ? matchBridge[2] : varSym.inferredType.replace(/^foreign_[a-z]+:/, '');

                const lspBridge = ForeignLspBridge.getInstance();
                if (lspBridge && lspBridge.isLanguageExtensionAvailable(bridge)) {
                    const shadowDoc = lspBridge.createShadowDocumentForMember(bridge, rawPkg, `${fnName}(`);
                    const lspHelp = await lspBridge.querySignatureHelp(shadowDoc, '(');
                    if (lspHelp) return lspHelp;
                }

                foreignMod = getForeignModule(rawPkg);
            }
            if (!foreignMod) {
                foreignMod = getForeignModule(modOrVar);
            }

            if (foreignMod && foreignMod.methods[fnName]) {
                const fn = foreignMod.methods[fnName];
                const sig = new vscode.SignatureInformation(fn.signature, new vscode.MarkdownString(fn.description));
                if (fn.params) {
                    sig.parameters = fn.params.map(p => new vscode.ParameterInformation(p.name, p.doc));
                    if (namedParam) {
                        const idx = fn.params.findIndex(p => p.name === namedParam);
                        if (idx !== -1) help.activeParameter = idx;
                    }
                }
                help.signatures = [sig];
                return help;
            }

            if (COLLECTION_METHODS[fnName]) {
                const m = COLLECTION_METHODS[fnName];
                const sig = new vscode.SignatureInformation(m.signature, new vscode.MarkdownString(m.description));
                if (m.params) {
                    sig.parameters = m.params.map(p => new vscode.ParameterInformation(p.name, p.doc));
                }
                help.signatures = [sig];
                return help;
            }

            // Check if modOrVar is an instance of a user class: acc.deposit(...)
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
                                if (idx !== -1) help.activeParameter = idx;
                            }
                        }
                        help.signatures = [sig];
                        return help;
                    }
                }
            }
        }

        // 2. Built-in functions (print, println, range, len, type, takes, error, panic, gc, free)
        if (BUILTIN_FUNCTIONS[callee]) {
            const fn = BUILTIN_FUNCTIONS[callee];
            const sig = new vscode.SignatureInformation(fn.signature, new vscode.MarkdownString(fn.description));
            if (fn.params) {
                sig.parameters = fn.params.map(p => new vscode.ParameterInformation(p.name, p.doc));
            }
            help.signatures = [sig];
            return help;
        }

        // 3. Common C extern functions
        if (COMMON_EXTERN_C_FUNCTIONS[callee]) {
            const ext = COMMON_EXTERN_C_FUNCTIONS[callee];
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
                    if (idx !== -1) help.activeParameter = idx;
                }
            }
            help.signatures = [sig];
            return help;
        }

        return null;
    }

    private getCallInfo(text: string): { callee: string; activeParam: number; namedParam?: string } | null {
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
            } else if (ch === '(') {
                if (parenDepth > 0) {
                    parenDepth--;
                } else {
                    // Found opening parenthesis of current call
                    const beforeParen = text.substring(0, i).trim();
                    const calleeMatch = beforeParen.match(/([a-zA-Z_][a-zA-Z0-9_\.]*)$/);
                    if (!calleeMatch) return null;

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
            } else if (ch === ',' && parenDepth === 0) {
                if (lastCommaPos === -1) {
                    lastCommaPos = i;
                }
                commaCount++;
            }
        }
        return null;
    }
}
