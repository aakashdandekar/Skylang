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
exports.SkylangRunner = void 0;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
const fs = __importStar(require("fs"));
class SkylangRunner {
    static terminal = null;
    static getExecutablePath() {
        const config = vscode.workspace.getConfiguration('skylang');
        const customPath = config.get('executablePath');
        if (customPath && customPath.trim().length > 0) {
            return customPath;
        }
        // Check common default locations
        const candidatePaths = [
            '/home/aakashdandekar/Projects/programming_lang/bin/sky',
            path.join(process.env.HOME || '', '.local', 'bin', 'sky'),
            path.join(process.env.HOME || '', '.local', 'bin', 'skylang'),
            '/usr/local/bin/sky',
            '/usr/bin/sky',
            'sky'
        ];
        for (const candidate of candidatePaths) {
            if (fs.existsSync(candidate)) {
                return candidate;
            }
        }
        return 'sky';
    }
    static getOrCreateTerminal(targetDir) {
        if (!this.terminal || this.terminal.exitStatus !== undefined) {
            this.terminal = vscode.window.createTerminal({
                name: 'Skylang',
                cwd: targetDir || vscode.workspace.workspaceFolders?.[0]?.uri.fsPath
            });
        }
        return this.terminal;
    }
    static sendCommand(terminal, cmd) {
        terminal.show(true);
        terminal.sendText(cmd, true);
    }
    static async runFile(uri) {
        const filePath = this.getTargetFilePath(uri);
        if (!filePath)
            return;
        const fileDir = path.dirname(filePath);
        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal(fileDir);
        this.sendCommand(terminal, `"${skyExe}" run "${filePath}"`);
    }
    static async buildFile(uri) {
        const filePath = this.getTargetFilePath(uri);
        if (!filePath)
            return;
        const defaultOut = filePath.replace(/\.sky$/i, '');
        const outBin = await vscode.window.showInputBox({
            prompt: 'Enter output binary path',
            value: defaultOut
        });
        if (!outBin)
            return;
        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal();
        terminal.show();
        terminal.sendText(`"${skyExe}" build "${filePath}" -o "${outBin}"`);
    }
    static getTargetFilePath(uri) {
        if (uri && uri.fsPath) {
            return uri.fsPath;
        }
        const activeEditor = vscode.window.activeTextEditor;
        if (activeEditor && activeEditor.document.languageId === 'skylang') {
            return activeEditor.document.fileName;
        }
        vscode.window.showWarningMessage('No active Skylang (.sky) file open to execute.');
        return null;
    }
}
exports.SkylangRunner = SkylangRunner;
//# sourceMappingURL=runner.js.map