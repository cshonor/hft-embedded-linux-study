# -*- coding: utf-8 -*-
"""Fix all refs after moving 04.5-network-sockets -> 04-cpp/M5-cpp-network-programming."""
import io, os

ROOT = r"c:\Users\12392\Desktop\hft"
NEW = "04-cpp/M5-cpp-network-programming"
MOVED = os.path.join(ROOT, NEW)
changed = 0

for dirpath, dirnames, filenames in os.walk(ROOT):
    if ".git" in dirpath:
        continue
    for fn in filenames:
        if not fn.endswith(".md"):
            continue
        p = os.path.join(dirpath, fn)
        c = io.open(p, encoding="utf-8").read()
        orig = c

        if p.startswith(MOVED):
            # module moved one level deeper: ../X -> ../../X, ../04-cpp/ -> ./
            c = c.replace("](../04-cpp/)", "](./)")
            c = c.replace("](../", "](../../")
            # self-name mentions in structure diagrams / headers
            c = c.replace("04.5-network-sockets", "04-cpp/M5-cpp-network-programming")
        elif os.path.normpath(dirpath).startswith(os.path.join(ROOT, "04-cpp")):
            # refs from inside 04-cpp: strip one ../ level
            c = c.replace("](../../04.5-network-sockets", "](../M5-cpp-network-programming")
            c = c.replace("](../04.5-network-sockets", "](./M5-cpp-network-programming")
            c = c.replace("04.5-network-sockets", "M5-cpp-network-programming")
        else:
            c = c.replace("04.5-network-sockets", NEW)

        if c != orig:
            io.open(p, "w", encoding="utf-8", newline="\n").write(c)
            changed += 1

print("changed:", changed)
