#!/usr/bin/env python3
import re
import html
import subprocess
import os
import sys

def markdown_to_html(md_text):
    lines = md_text.split('\n')
    html_out = []
    
    in_code_block = False
    code_lang = ""
    code_lines = []
    
    in_table = False
    table_headers = []
    table_rows = []
    
    in_list = False
    list_type = None

    def highlight_code(code_str, lang):
        if not lang or lang not in ('skylang', 'sky', 'c', 'javascript', 'js', 'python', 'py', 'go', 'rust', 'java', 'bash', 'sh', 'cmd'):
            return html.escape(code_str)

        token_re = re.compile(
            r'(?P<cmt>//[^\n]*|#[^\n]*)'
            r'|(?P<str>f?"(?:\\.|[^"\\])*"|f?\'(?:\\.|[^\'\\])*\')'
            r'|(?P<word>\b[A-Za-z_][A-Za-z0-9_]*\b)'
            r'|(?P<other>.)',
            re.DOTALL
        )

        keywords = {
            'import', 'from', 'as', 'cimport', 'extern', 'f', 'class', 'this', 'super',
            'return', 'if', 'else', 'while', 'for', 'in', 'break', 'continue',
            'async', 'await', 'spawn', 'eval', 'panic', 'takes', 'true', 'false', 'none'
        }
        types = {
            'int', 'double', 'bool', 'char', 'string', 'type', 'array', 'list', 'tuple', 'dict', 'set', 'sortedList',
            'I', 'D', 'B', 'C', 'S', 'L', 'T', 'SL', 'DICT', 'SET'
        }

        out = []
        for m in token_re.finditer(code_str):
            if m.group('cmt'):
                out.append(f'<span class="cmt">{html.escape(m.group("cmt"))}</span>')
            elif m.group('str'):
                out.append(f'<span class="str">{html.escape(m.group("str"))}</span>')
            elif m.group('word'):
                w = m.group('word')
                if w in keywords:
                    out.append(f'<span class="kwd">{html.escape(w)}</span>')
                elif w in types:
                    out.append(f'<span class="typ">{html.escape(w)}</span>')
                else:
                    out.append(html.escape(w))
            else:
                out.append(html.escape(m.group('other')))

        return "".join(out)

    def close_list():
        nonlocal in_list, list_type
        if in_list:
            html_out.append(f"</{list_type}>")
            in_list = False
            list_type = None

    def close_table():
        nonlocal in_table, table_headers, table_rows
        if in_table:
            t_html = ["<div class='table-container'><table>"]
            if table_headers:
                t_html.append("<thead><tr>")
                for th in table_headers:
                    t_html.append(f"<th>{process_inline(th)}</th>")
                t_html.append("</tr></thead>")
            if table_rows:
                t_html.append("<tbody>")
                for row in table_rows:
                    t_html.append("<tr>")
                    for td in row:
                        t_html.append(f"<td>{process_inline(td)}</td>")
                    t_html.append("</tr>")
                t_html.append("</tbody>")
            t_html.append("</table></div>")
            html_out.append("\n".join(t_html))
            in_table = False
            table_headers = []
            table_rows = []

    def process_inline(text):
        t = html.escape(text)
        t = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', t)
        t = re.sub(r'\*(.+?)\*', r'<em>\1</em>', t)
        t = re.sub(r'`([^`]+)`', r'<code>\1</code>', t)
        t = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', r'<a href="\2">\1</a>', t)
        return t

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if stripped.startswith("```"):
            if in_code_block:
                close_list()
                close_table()
                raw_code = "\n".join(code_lines)
                highlighted = highlight_code(raw_code, code_lang)
                badge_html = f"<div class='code-header'>{html.escape(code_lang.upper())}</div>" if code_lang else ""
                block_html = (
                    f"<table class='code-table' width='100%' cellpadding='0' cellspacing='0'>"
                    f"<tr><td>{badge_html}<pre class='code-content'>{highlighted}</pre></td></tr>"
                    f"</table>"
                )
                html_out.append(block_html)
                in_code_block = False
                code_lines = []
                code_lang = ""
            else:
                close_list()
                close_table()
                in_code_block = True
                code_lang = stripped[3:].strip()
                code_lines = []
            i += 1
            continue

        if in_code_block:
            code_lines.append(line)
            i += 1
            continue

        if not stripped:
            close_list()
            close_table()
            i += 1
            continue

        if stripped.startswith("|") and stripped.endswith("|"):
            cells = [c.strip() for c in stripped.split("|")[1:-1]]
            if all(re.match(r'^:?-+:?$', c) for c in cells):
                i += 1
                continue
            if not in_table:
                close_list()
                in_table = True
                table_headers = cells
            else:
                table_rows.append(cells)
            i += 1
            continue
        else:
            close_table()

        if stripped in ("---", "***", "___"):
            close_list()
            html_out.append("<hr/>")
            i += 1
            continue

        if stripped.startswith("#"):
            close_list()
            m = re.match(r'^(#{1,6})\s+(.*)$', stripped)
            if m:
                level = len(m.group(1))
                h_text = process_inline(m.group(2))
                h_id = re.sub(r'[^a-zA-Z0-9_-]', '', m.group(2).lower().replace(' ', '-'))
                html_out.append(f"<h{level} id='{h_id}'>{h_text}</h{level}>")
                i += 1
                continue

        if stripped.startswith("- ") or stripped.startswith("* "):
            close_table()
            if not in_list or list_type != 'ul':
                close_list()
                in_list = True
                list_type = 'ul'
                html_out.append("<ul>")
            item_text = process_inline(stripped[2:])
            html_out.append(f"<li>{item_text}</li>")
            i += 1
            continue

        m_num = re.match(r'^(\d+)\.\s+(.*)$', stripped)
        if m_num:
            close_table()
            if not in_list or list_type != 'ol':
                close_list()
                in_list = True
                list_type = 'ol'
                html_out.append("<ol>")
            item_text = process_inline(m_num.group(2))
            html_out.append(f"<li>{item_text}</li>")
            i += 1
            continue

        close_list()
        close_table()
        html_out.append(f"<p>{process_inline(stripped)}</p>")
        i += 1

    close_list()
    close_table()
    return "\n".join(html_out)

def main():
    doc_path = os.path.join(os.path.dirname(__file__), "..", "DOC.md")
    out_pdf = os.path.join(os.path.dirname(__file__), "..", "Skylang_Complete_Manual.pdf")
    
    if not os.path.exists(doc_path):
        print(f"Error: {doc_path} not found")
        sys.exit(1)
        
    with open(doc_path, "r", encoding="utf-8") as f:
        doc_md = f.read()

    body_html = markdown_to_html(doc_md)

    full_html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Skylang Programming Language - Complete Manual</title>
<style>
    @page {{
        size: A4;
        margin: 20mm 15mm 20mm 15mm;
        @bottom-right {{
            content: counter(page);
        }}
    }}
    body {{
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        font-size: 10.5pt;
        line-height: 1.55;
        color: #1a202c;
        background: #ffffff;
        margin: 0;
        padding: 0;
    }}
    h1 {{
        color: #1e3a8a;
        font-size: 22pt;
        border-bottom: 3px solid #2563eb;
        padding-bottom: 8px;
        margin-top: 15px;
        margin-bottom: 15px;
        page-break-inside: avoid;
    }}
    h2 {{
        color: #1e40af;
        font-size: 15pt;
        border-bottom: 1.5px solid #93c5fd;
        padding-bottom: 5px;
        margin-top: 22px;
        margin-bottom: 10px;
        page-break-after: avoid;
        page-break-inside: avoid;
    }}
    h3 {{
        color: #1d4ed8;
        font-size: 12pt;
        margin-top: 16px;
        margin-bottom: 6px;
        page-break-after: avoid;
        page-break-inside: avoid;
    }}
    h4 {{
        color: #3b82f6;
        font-size: 11pt;
        margin-top: 12px;
        margin-bottom: 4px;
        page-break-after: avoid;
    }}
    p {{
        margin-top: 4px;
        margin-bottom: 8px;
        text-align: justify;
    }}
    ul, ol {{
        margin-top: 4px;
        margin-bottom: 8px;
        padding-left: 22px;
    }}
    li {{
        margin-bottom: 3px;
    }}
    hr {{
        border: none;
        border-top: 1px solid #e2e8f0;
        margin: 18px 0;
    }}
    code {{
        font-family: "DejaVu Sans Mono", "Liberation Mono", Consolas, "Courier New", monospace;
        font-size: 9pt;
        background-color: #f1f5f9;
        color: #0f172a;
        padding: 1.5px 4.5px;
        border-radius: 4px;
        border: 1px solid #e2e8f0;
    }}
    .code-table {{
        background-color: #f8fafc;
        border: 1px solid #cbd5e1;
        border-collapse: separate;
        border-radius: 4px;
        margin: 6px 0 10px 0;
        page-break-inside: avoid;
        width: 100%;
    }}
    .code-table td {{
        background-color: #f8fafc;
        border: none;
        padding: 0;
    }}
    .code-header {{
        background-color: #e2e8f0;
        color: #1e3a8a;
        font-family: "DejaVu Sans Mono", "Liberation Mono", Consolas, "Courier New", monospace;
        font-size: 7.5pt;
        font-weight: bold;
        padding: 3px 8px;
        letter-spacing: 0.5px;
        border-bottom: 1px solid #cbd5e1;
    }}
    pre.code-content {{
        margin: 0;
        padding: 8px 10px;
        background-color: #f8fafc;
        color: #0f172a;
        font-family: "DejaVu Sans Mono", "Liberation Mono", Consolas, "Courier New", monospace;
        font-size: 8.5pt;
        line-height: 1.45;
        border: none;
        white-space: pre-wrap;
    }}
    .kwd {{ color: #be123c; font-weight: bold; }}
    .typ {{ color: #6b21a8; font-weight: 600; }}
    .cmt {{ color: #64748b; font-style: italic; }}
    .str {{ color: #047857; }}
    
    .table-container {{
        margin: 10px 0 14px 0;
        page-break-inside: avoid;
    }}
    table {{
        width: 100%;
        border-collapse: collapse;
        font-size: 9.5pt;
    }}
    th, td {{
        border: 1px solid #cbd5e1;
        padding: 6px 10px;
        text-align: left;
    }}
    th {{
        background-color: #f1f5f9;
        color: #1e293b;
        font-weight: 600;
    }}
    tr:nth-child(even) {{
        background-color: #f8fafc;
    }}
    a {{
        color: #2563eb;
        text-decoration: none;
    }}
</style>
</head>
<body>
{body_html}
</body>
</html>
"""
    tmp_html = "/tmp/skylang_manual_temp.html"
    with open(tmp_html, "w", encoding="utf-8") as f:
        f.write(full_html)
        
    cmd = ["libreoffice", "--headless", "--convert-to", "pdf", tmp_html, "--outdir", "/tmp"]
    subprocess.run(cmd, check=True)
    
    tmp_pdf = "/tmp/skylang_manual_temp.pdf"
    if os.path.exists(tmp_pdf):
        import shutil
        shutil.copyfile(tmp_pdf, out_pdf)
        print(f"Successfully generated {out_pdf}")

if __name__ == "__main__":
    main()
