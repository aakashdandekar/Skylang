"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.SkylangRunner = void 0;
const vscode = require("vscode");
const path = require("path");
const fs = require("fs");
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
    static async runVM(uri) {
        const filePath = this.getTargetFilePath(uri);
        if (!filePath)
            return;
        const fileDir = path.dirname(filePath);
        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal(fileDir);
        this.sendCommand(terminal, `"${skyExe}" vm "${filePath}"`);
    }
    static async profileVM(uri) {
        const filePath = this.getTargetFilePath(uri);
        if (!filePath)
            return;
        const fileDir = path.dirname(filePath);
        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal(fileDir);
        this.sendCommand(terminal, `"${skyExe}" profile "${filePath}"`);
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
    static startRepl() {
        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal();
        terminal.show();
        terminal.sendText(`"${skyExe}" repl`);
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