"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.SkylangCodeLensProvider = void 0;
const vscode = require("vscode");
class SkylangCodeLensProvider {
    provideCodeLenses(document, _token) {
        const lenses = [];
        const topRange = new vscode.Range(0, 0, 0, 0);
        lenses.push(new vscode.CodeLens(topRange, {
            title: '$(play) Run',
            command: 'skylang.run',
            arguments: [document.uri],
            tooltip: 'Run Skylang program'
        }));
        return lenses;
    }
}
exports.SkylangCodeLensProvider = SkylangCodeLensProvider;
//# sourceMappingURL=codelens.js.map