#!/usr/bin/env python3
"""批量修正 06-cpp 下 .md 文件的 markdown 链接路径。

书文件夹从 06-cpp/XX-书名/ 移到了 06-cpp/Mn-xxx/XX-书名/，
本脚本根据当前文件位置和目标书新位置重算相对路径。
"""
import os
import re

ROOT = r"C:\Users\12392\Desktop\hft\06-cpp"

BOOK_TO_MODULE = {
    "01-C++Primer": "M0-entry-syntax",
    "02-Effective-C++": "M3-engineering-standards",
    "03-More-Effective-C++": "M3-engineering-standards",
    "04-Effective-Modern-C++": "M1-modern-cpp",
    "05-Effective-STL": "M3-engineering-standards",
    "06-STL-Source-Analysis": "M3-engineering-standards",
    "07-Cpp-Object-Model": "M2-deep-principles",
    "08-Cpp-Concurrency": "M2-deep-principles",
    "09-C++17-The-Complete-Guide": "M4-advanced-standards",
    "10-C++20-The-Complete-Guide": "M4-advanced-standards",
}

# 只处理 markdown 链接 ](路径) 里的路径
LINK_PATTERN = re.compile(r'\]\(([^)]+)\)')


def fix_link_path(file_path, path):
    """如果路径引用了某本书，重算相对路径；否则原样返回。"""
    for book, module in BOOK_TO_MODULE.items():
        if book not in path:
            continue
        # 找到书名在路径里的位置（避免子串误匹配：要求前面是 / 或路径开头）
        idx = path.find(book)
        # 检查书名前一个字符是否是 / 或开头
        if idx > 0 and path[idx - 1] != '/':
            continue
        # 书名后应该是 / 或结尾
        after_book = path[idx + len(book):]
        if after_book and not after_book.startswith('/'):
            continue

        # 计算从当前文件到目标书的相对路径
        rel = os.path.relpath(file_path, ROOT).replace('\\', '/')
        file_dir = os.path.dirname(rel)  # 如 "M3-engineering-standards/02-Effective-C++/ch01-..."
        if file_dir == '.':
            file_dir = ''

        target = f"{module}/{book}"
        new_prefix = os.path.relpath(target, file_dir).replace('\\', '/')
        if not new_prefix.startswith('.'):
            new_prefix = './' + new_prefix

        return new_prefix + after_book
    return path


def process_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    def replacer(m):
        old_path = m.group(1)
        new_path = fix_link_path(file_path, old_path)
        return f']({new_path})'

    new_content = LINK_PATTERN.sub(replacer, content)

    if new_content != content:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    return False


changed = 0
for root, dirs, files in os.walk(ROOT):
    if '.git' in root:
        continue
    for f in files:
        if f.endswith('.md'):
            fp = os.path.join(root, f)
            if process_file(fp):
                changed += 1
                print(f"FIXED: {os.path.relpath(fp, ROOT)}")

print(f"\nTotal: {changed} files fixed")
