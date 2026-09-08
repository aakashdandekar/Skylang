import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';

export class SkylangRunner {
    private static terminal: vscode.Terminal | null = null;

    public static getExecutablePath(): string {
        const config = vscode.workspace.getConfiguration('skylang');
        const customPath = config.get<string>('executablePath');
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

    private static getOrCreateTerminal(targetDir?: string): vscode.Terminal {
        if (!this.terminal || this.terminal.exitStatus !== undefined) {
            this.terminal = vscode.window.createTerminal({
                name: 'Skylang',
                cwd: targetDir || vscode.workspace.workspaceFolders?.[0]?.uri.fsPath
            });
        }
        return this.terminal;
    }

    private static sendCommand(terminal: vscode.Terminal, cmd: string): void {
        terminal.show(true);
        terminal.sendText(cmd, true);
    }

    public static async runFile(uri?: vscode.Uri): Promise<void> {
        const filePath = this.getTargetFilePath(uri);
        if (!filePath) return;

        const fileDir = path.dirname(filePath);
        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal(fileDir);
        this.sendCommand(terminal, `"${skyExe}" run "${filePath}"`);
    }

    public static async runVM(uri?: vscode.Uri): Promise<void> {
        const filePath = this.getTargetFilePath(uri);
        if (!filePath) return;

        const fileDir = path.dirname(filePath);
        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal(fileDir);
        this.sendCommand(terminal, `"${skyExe}" vm "${filePath}"`);
    }

    public static async profileVM(uri?: vscode.Uri): Promise<void> {
        const filePath = this.getTargetFilePath(uri);
        if (!filePath) return;

        const fileDir = path.dirname(filePath);
        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal(fileDir);
        this.sendCommand(terminal, `"${skyExe}" profile "${filePath}"`);
    }

    public static async buildFile(uri?: vscode.Uri): Promise<void> {
        const filePath = this.getTargetFilePath(uri);
        if (!filePath) return;

        const defaultOut = filePath.replace(/\.sky$/i, '');
        const outBin = await vscode.window.showInputBox({
            prompt: 'Enter output binary path',
            value: defaultOut
        });

        if (!outBin) return;

        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal();
        terminal.show();
        terminal.sendText(`"${skyExe}" build "${filePath}" -o "${outBin}"`);
    }

    public static startRepl(): void {
        const skyExe = this.getExecutablePath();
        const terminal = this.getOrCreateTerminal();
        terminal.show();
        terminal.sendText(`"${skyExe}" repl`);
    }

    private static getTargetFilePath(uri?: vscode.Uri): string | null {
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
