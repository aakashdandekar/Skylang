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
exports.SkylangDocumentFormattingProvider = void 0;
const vscode = __importStar(require("vscode"));
class SkylangDocumentFormattingProvider {
    provideDocumentFormattingEdits(document, options, _token) {
        const edits = [];
        const lineCount = document.lineCount;
        const tabSize = options.tabSize || 4;
        const indentUnit = options.insertSpaces ? ' '.repeat(tabSize) : '\t';
        let indentLevel = 0;
        const formattedLines = [];
        for (let i = 0; i < lineCount; i++) {
            const rawLine = document.lineAt(i).text;
            const trimmed = rawLine.trim();
            if (trimmed.length === 0) {
                formattedLines.push('');
                continue;
            }
            // If line starts with closing brace, decrement indent before applying
            let leadingCloses = 0;
            for (let j = 0; j < trimmed.length; j++) {
                if (trimmed[j] === '}')
                    leadingCloses++;
                else
                    break;
            }
            const currentIndent = Math.max(0, indentLevel - leadingCloses);
            const formattedLine = indentUnit.repeat(currentIndent) + this.formatLineContent(trimmed);
            formattedLines.push(formattedLine);
            // Update indent level for subsequent lines based on braces
            const braceDelta = this.calculateBraceDelta(trimmed);
            indentLevel = Math.max(0, indentLevel + braceDelta);
        }
        const fullRange = new vscode.Range(new vscode.Position(0, 0), new vscode.Position(lineCount - 1, document.lineAt(lineCount - 1).text.length));
        edits.push(vscode.TextEdit.replace(fullRange, formattedLines.join('\n')));
        return edits;
    }
    formatLineContent(line) {
        // Leave pure comment lines untouched
        if (line.startsWith('//') || line.startsWith('#') || line.startsWith('/*')) {
            return line;
        }
        // Normalize spaces around commas
        let res = line.replace(/\s*,\s*/g, ', ');
        // Normalize space after keywords: if, elif, for, class, f, import, cimport, extern
        res = res.replace(/\b(if|elif|for|class|f|import|cimport|extern|return)\s+/g, '$1 ');
        // Normalize space before opening brace `{`
        res = res.replace(/\s*\{/g, ' {');
        // Normalize compound assignment operators
        res = res.replace(/\s*(\:\=|\+\=|\-\=|\*\=|\/\=|\=\=|\!\=|\<\=|\>\=)\s*/g, ' $1 ');
        return res;
    }
    calculateBraceDelta(line) {
        let delta = 0;
        let inString = false;
        let quoteChar = '';
        for (let i = 0; i < line.length; i++) {
            const ch = line[i];
            const prev = i > 0 ? line[i - 1] : '';
            // Ignore line comments
            if (!inString && ch === '/' && i + 1 < line.length && line[i + 1] === '/') {
                break;
            }
            if (!inString && ch === '#') {
                break;
            }
            if (inString) {
                if (ch === quoteChar && prev !== '\\') {
                    inString = false;
                }
                continue;
            }
            if (ch === '"' || ch === '\'') {
                inString = true;
                quoteChar = ch;
                continue;
            }
            if (ch === '{')
                delta++;
            else if (ch === '}')
                delta--;
        }
        return delta;
    }
}
exports.SkylangDocumentFormattingProvider = SkylangDocumentFormattingProvider;
//# sourceMappingURL=formatter.js.map