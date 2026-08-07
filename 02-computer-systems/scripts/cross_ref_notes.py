#!/usr/bin/env python3
"""
Add note-level cross-references between Harris (00) and CSAPP (02).
- Harris side: update "Link Target" field with clickable markdown links
- CSAPP side: add "↔ Harris" line after navigation header
"""

import os

HARRIS_BASE = "C:/Users/12392/Desktop/hft/00-digital-logic-cpu"
CSAPP_BASE  = "C:/Users/12392/Desktop/hft/02-computer-systems"

# ── Harris → CSAPP link data ──────────────────────────────────────────
# Each entry: (harris_rel_path, [(link_text, csapp_rel_path_from_harris), ...])
# csapp_rel_path_from_harris = ../../02-computer-systems/chapter-XX/notes/section-X.Y.md

HARRIS_LINKS = {
    "ch01_binary/1.4_数字系统.md": [
        ("CSAPP \u00a72.1.1 \u5341\u516d\u8fdb\u5236", "../../02-computer-systems/chapter-02-representing-information/notes/section-2.1.1-\u5341\u516d\u8fdb\u5236\u8868\u793a\u6cd5.md"),
        ("\u00a72.2 \u6574\u6570\u8868\u793a", "../../02-computer-systems/chapter-02-representing-information/notes/section-2.2-\u6574\u6570\u8868\u793a\u4e0e\u7c7b\u578b\u8f6c\u6362.md"),
    ],
    "ch02_combinational/2.8.3_MUX.md": [
        ("CSAPP \u00a74.2 HCL", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.2-HCL\u903b\u8f91\u4e0e\u7ec4\u5408\u7535\u8def.md"),
    ],
    "ch02_combinational/2.8.5_\u52a0\u6cd5\u5668.md": [
        ("CSAPP \u00a74.2 HCL", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.2-HCL\u903b\u8f91\u4e0e\u7ec4\u5408\u7535\u8def.md"),
    ],
    "ch02_combinational/2.9_\u65f6\u5e8f.md": [
        ("CSAPP \u00a74.2 HCL", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.2-HCL\u903b\u8f91\u4e0e\u7ec4\u5408\u7535\u8def.md"),
    ],
    "ch03_sequential/3.2_\u9501\u5b58\u5668\u548c\u89e6\u53d1\u5668.md": [
        ("CSAPP \u00a74.2 HCL", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.2-HCL\u903b\u8f91\u4e0e\u7ec4\u5408\u7535\u8def.md"),
    ],
    "ch03_sequential/3.5_\u65f6\u5e8f\u903b\u8f91\u7535\u8def\u7684\u65f6\u5e8f.md": [
        ("CSAPP \u00a74.4 \u6d41\u6c34\u7ebf", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.4-\u6d41\u6c34\u7ebf\u539f\u7406\u4e0e\u5c40\u9650.md"),
    ],
    "ch05_digital_blocks/5.2_\u7b97\u672f\u7535\u8def.md": [
        ("CSAPP \u00a74.2 HCL", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.2-HCL\u903b\u8f91\u4e0e\u7ec4\u5408\u7535\u8def.md"),
    ],
    "ch05_digital_blocks/5.5_\u5b58\u50a8\u5668\u9635\u5217.md": [
        ("CSAPP \u00a74.2 HCL", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.2-HCL\u903b\u8f91\u4e0e\u7ec4\u5408\u7535\u8def.md"),
    ],
    "ch06_architecture/6.2_\u6c47\u7f16\u8bed\u8a00.md": [
        ("CSAPP \u00a73.2.3 AT&T", "../../02-computer-systems/chapter-03-machine-level-programs/notes/section-3.2.3-AT&T\u6c47\u7f16\u8bed\u6cd5.md"),
    ],
    "ch06_architecture/6.6_\u5176\u4ed6\u4e3b\u9898.md": [
        ("CSAPP \u00a73.6 \u63a7\u5236\u6d41", "../../02-computer-systems/chapter-03-machine-level-programs/notes/section-3.6-\u63a7\u5236\u6d41.md"),
    ],
    "ch06_architecture/6.8_\u53e6\u4e00\u4e2a\u89c6\u89d2-x86\u4f53\u7cfb\u7ed3\u6784.md": [
        ("CSAPP \u00a73.3 \u6570\u636e\u683c\u5f0f", "../../02-computer-systems/chapter-03-machine-level-programs/notes/section-3.3-\u6570\u636e\u683c\u5f0f.md"),
    ],
    "ch07_microarchitecture/7.2_\u6027\u80fd\u5206\u6790.md": [
        ("CSAPP \u00a74.4 \u6d41\u6c34\u7ebf", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.4-\u6d41\u6c34\u7ebf\u539f\u7406\u4e0e\u5c40\u9650.md"),
    ],
    "ch07_microarchitecture/7.3_\u5355\u5468\u671f\u5904\u7406\u5668.md": [
        ("CSAPP \u00a74.3 SEQ", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.3-SEQ\u987a\u5e8f\u5904\u7406\u5668.md"),
    ],
    "ch07_microarchitecture/7.5_\u6d41\u6c34\u7ebf\u5904\u7406\u5668.md": [
        ("CSAPP \u00a74.5 PIPE", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.5-PIPE\u6d41\u6c34\u7ebf\u4e0e\u5192\u9669.md"),
    ],
    "ch07_microarchitecture/7.7_\u9ad8\u7ea7\u5fae\u7ed3\u6784.md": [
        ("CSAPP \u00a74.5 PIPE", "../../02-computer-systems/chapter-04-processor-architecture/notes/section-4.5-PIPE\u6d41\u6c34\u7ebf\u4e0e\u5192\u9669.md"),
    ],
    "ch08_memory/8.2_\u5b58\u50a8\u5668\u7cfb\u7edf\u6027\u80fd\u5206\u6790.md": [
        ("CSAPP \u00a76.4.7 Cache\u6027\u80fd", "../../02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.4.7-Cache\u53c2\u6570\u7684\u6027\u80fd\u5f71\u54cd.md"),
        ("\u00a76.6 \u5b58\u50a8\u5668\u5c71", "../../02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.6-\u5b58\u50a8\u5668\u5c71.md"),
    ],
    "ch08_memory/8.3_\u9ad8\u901f\u7f13\u5b58.md": [
        ("CSAPP \u00a76.4.2 \u76f4\u63a5\u6620\u5c04", "../../02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.4.2-\u76f4\u63a5\u6620\u5c04.md"),
    ],
    "ch08_memory/8.4_\u865a\u62df\u5b58\u50a8\u5668.md": [
        ("CSAPP \u00a79.6 \u5730\u5740\u7ffb\u8bd1", "../../02-computer-systems/chapter-09-virtual-memory/notes/section-9.6-\u5730\u5740\u7ffb\u8bd1.md"),
        ("\u00a79.3 VM\u4f5c\u4e3a\u7f13\u5b58", "../../02-computer-systems/chapter-09-virtual-memory/notes/section-9.3-\u865a\u62df\u5185\u5b58\u4f5c\u4e3a\u7f13\u5b58\u5de5\u5177.md"),
    ],
}

# ── CSAPP → Harris link data (grouped by CSAPP note) ──────────────────
# Each entry: (csapp_rel_path, [(link_text, harris_rel_path_from_csapp), ...])
# harris_rel_path_from_csapp = ../../../00-digital-logic-cpu/chXX/X.Y.md

CSAPP_LINKS = {
    "chapter-02-representing-information/notes/section-2.1.1-\u5341\u516d\u8fdb\u5236\u8868\u793a\u6cd5.md": [
        ("Harris \u00a71.4 \u6570\u5b57\u7cfb\u7edf", "../../../00-digital-logic-cpu/ch01_binary/1.4_\u6570\u5b57\u7cfb\u7edf.md"),
    ],
    "chapter-03-machine-level-programs/notes/section-3.2.3-AT&T\u6c47\u7f16\u8bed\u6cd5.md": [
        ("Harris \u00a76.2 \u6c47\u7f16\u8bed\u8a00", "../../../00-digital-logic-cpu/ch06_architecture/6.2_\u6c47\u7f16\u8bed\u8a00.md"),
    ],
    "chapter-03-machine-level-programs/notes/section-3.3-\u6570\u636e\u683c\u5f0f.md": [
        ("Harris \u00a76.8 x86\u4f53\u7cfb\u7ed3\u6784", "../../../00-digital-logic-cpu/ch06_architecture/6.8_\u53e6\u4e00\u4e2a\u89c6\u89d2-x86\u4f53\u7cfb\u7ed3\u6784.md"),
    ],
    "chapter-03-machine-level-programs/notes/section-3.6-\u63a7\u5236\u6d41.md": [
        ("Harris \u00a76.6 \u5f02\u5e38/\u4e2d\u65ad", "../../../00-digital-logic-cpu/ch06_architecture/6.6_\u5176\u4ed6\u4e3b\u9898.md"),
    ],
    "chapter-04-processor-architecture/notes/section-4.2-HCL\u903b\u8f91\u4e0e\u7ec4\u5408\u7535\u8def.md": [
        ("Harris \u00a72.8.3 MUX", "../../../00-digital-logic-cpu/ch02_combinational/2.8.3_MUX.md"),
        ("\u00a72.8.5 \u52a0\u6cd5\u5668", "../../../00-digital-logic-cpu/ch02_combinational/2.8.5_\u52a0\u6cd5\u5668.md"),
        ("\u00a72.9 \u65f6\u5e8f", "../../../00-digital-logic-cpu/ch02_combinational/2.9_\u65f6\u5e8f.md"),
        ("\u00a73.2 \u89e6\u53d1\u5668", "../../../00-digital-logic-cpu/ch03_sequential/3.2_\u9501\u5b58\u5668\u548c\u89e6\u53d1\u5668.md"),
        ("\u00a75.2 ALU", "../../../00-digital-logic-cpu/ch05_digital_blocks/5.2_\u7b97\u672f\u7535\u8def.md"),
        ("\u00a75.5 \u5b58\u50a8\u9635\u5217", "../../../00-digital-logic-cpu/ch05_digital_blocks/5.5_\u5b58\u50a8\u5668\u9635\u5217.md"),
    ],
    "chapter-04-processor-architecture/notes/section-4.3-SEQ\u987a\u5e8f\u5904\u7406\u5668.md": [
        ("Harris \u00a77.3 \u5355\u5468\u671f", "../../../00-digital-logic-cpu/ch07_microarchitecture/7.3_\u5355\u5468\u671f\u5904\u7406\u5668.md"),
    ],
    "chapter-04-processor-architecture/notes/section-4.4-\u6d41\u6c34\u7ebf\u539f\u7406\u4e0e\u5c40\u9650.md": [
        ("Harris \u00a73.5 \u65f6\u5e8f\u7ea6\u675f", "../../../00-digital-logic-cpu/ch03_sequential/3.5_\u65f6\u5e8f\u903b\u8f91\u7535\u8def\u7684\u65f6\u5e8f.md"),
        ("\u00a77.2 \u6027\u80fd\u5206\u6790", "../../../00-digital-logic-cpu/ch07_microarchitecture/7.2_\u6027\u80fd\u5206\u6790.md"),
    ],
    "chapter-04-processor-architecture/notes/section-4.5-PIPE\u6d41\u6c34\u7ebf\u4e0e\u5192\u9669.md": [
        ("Harris \u00a77.5 \u6d41\u6c34\u7ebf", "../../../00-digital-logic-cpu/ch07_microarchitecture/7.5_\u6d41\u6c34\u7ebf\u5904\u7406\u5668.md"),
        ("\u00a77.7 \u9ad8\u7ea7\u5fae\u7ed3\u6784", "../../../00-digital-logic-cpu/ch07_microarchitecture/7.7_\u9ad8\u7ea7\u5fae\u7ed3\u6784.md"),
    ],
    "chapter-06-memory-hierarchy/notes/section-6.4.2-\u76f4\u63a5\u6620\u5c04.md": [
        ("Harris \u00a78.3 \u9ad8\u901f\u7f13\u5b58", "../../../00-digital-logic-cpu/ch08_memory/8.3_\u9ad8\u901f\u7f13\u5b58.md"),
    ],
    "chapter-06-memory-hierarchy/notes/section-6.4.7-Cache\u53c2\u6570\u7684\u6027\u80fd\u5f71\u54cd.md": [
        ("Harris \u00a78.2 \u6027\u80fd\u5206\u6790", "../../../00-digital-logic-cpu/ch08_memory/8.2_\u5b58\u50a8\u5668\u7cfb\u7edf\u6027\u80fd\u5206\u6790.md"),
    ],
    "chapter-06-memory-hierarchy/notes/section-6.6-\u5b58\u50a8\u5668\u5c71.md": [
        ("Harris \u00a78.2 \u6027\u80fd\u5206\u6790", "../../../00-digital-logic-cpu/ch08_memory/8.2_\u5b58\u50a8\u5668\u7cfb\u7edf\u6027\u80fd\u5206\u6790.md"),
    ],
    "chapter-09-virtual-memory/notes/section-9.3-\u865a\u62df\u5185\u5b58\u4f5c\u4e3a\u7f13\u5b58\u5de5\u5177.md": [
        ("Harris \u00a78.4 \u865a\u62df\u5b58\u50a8\u5668", "../../../00-digital-logic-cpu/ch08_memory/8.4_\u865a\u62df\u5b58\u50a8\u5668.md"),
    ],
    "chapter-09-virtual-memory/notes/section-9.6-\u5730\u5740\u7ffb\u8bd1.md": [
        ("Harris \u00a78.4 \u865a\u62df\u5b58\u50a8\u5668", "../../../00-digital-logic-cpu/ch08_memory/8.4_\u865a\u62df\u5b58\u50a8\u5668.md"),
    ],
}


def make_harris_link_md(links):
    """Build the markdown link string for Harris Link Target."""
    parts = []
    for text, path in links:
        parts.append(f"[{text}]({path})")
    return " \u00b7 ".join(parts)


def make_csapp_link_md(links):
    """Build the markdown link string for CSAPP nav header."""
    parts = []
    for text, path in links:
        parts.append(f"[{text}]({path})")
    return " \u00b7 ".join(parts)


def update_harris_note(rel_path, links):
    """Update a Harris note's Link Target field with clickable CSAPP links."""
    fpath = os.path.join(HARRIS_BASE, rel_path)
    with open(fpath, "r", encoding="utf-8") as f:
        content = f.read()

    link_md = make_harris_link_md(links)
    # Check if there's already a CSAPP link to avoid duplication
    if link_md.split("]")[0].strip("[").split(" ")[0] in content:
        # Check if the first link target is already in the file
        first_path = links[0][1]
        if first_path in content:
            print(f"  SKIP (already linked): {rel_path}")
            return False

    lines = content.split("\n")
    modified = False

    for i, line in enumerate(lines):
        if "**Link Target:**" in line:
            # Append the CSAPP link to the end of the Link Target value
            if link_md not in line:
                lines[i] = line.rstrip() + " \u00b7 \u2194 " + link_md
                modified = True
            break
    else:
        # No "Link Target" field found — this is a split file (2.8.3, 2.8.5)
        # Add a cross-reference line after the first blockquote line
        for i, line in enumerate(lines):
            if line.startswith("> ") and "\u62c6\u81ea" in line:
                # Insert after the split note line
                lines.insert(i + 1, f"> \u2194 {link_md}")
                modified = True
                break
        else:
            # Fallback: add after the first heading
            for i, line in enumerate(lines):
                if line.startswith("#"):
                    lines.insert(i + 2, f"> \u2194 {link_md}")
                    modified = True
                    break

    if modified:
        with open(fpath, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
        print(f"  OK: {rel_path}")
        return True
    else:
        print(f"  NOCHANGE: {rel_path}")
        return False


def update_csapp_note(rel_path, links):
    """Add a Harris cross-reference line to a CSAPP note's navigation header."""
    fpath = os.path.join(CSAPP_BASE, rel_path)
    with open(fpath, "r", encoding="utf-8") as f:
        content = f.read()

    link_md = make_csapp_link_md(links)
    harris_line = f"> \u2194 {link_md}"

    # Check if already added
    if "\u2194 Harris" in content or harris_line in content:
        print(f"  SKIP (already linked): {rel_path}")
        return False

    lines = content.split("\n")
    modified = False

    # Find the navigation header line (contains [章导读])
    for i, line in enumerate(lines):
        if "[\u7ae0\u5bfc\u8bfb]" in line:
            # Insert after this line
            lines.insert(i + 1, harris_line)
            modified = True
            break
    else:
        # Fallback: find the first ">" blockquote line and insert after it
        for i, line in enumerate(lines):
            if line.startswith("> ") and i > 0:
                lines.insert(i + 1, harris_line)
                modified = True
                break

    if modified:
        with open(fpath, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
        print(f"  OK: {rel_path}")
        return True
    else:
        print(f"  NOCHANGE: {rel_path}")
        return False


# ── Main ──────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("=== Harris side (00 → 02 links) ===")
    h_count = 0
    for rel_path, links in HARRIS_LINKS.items():
        if update_harris_note(rel_path, links):
            h_count += 1
    print(f"  Harris: {h_count}/{len(HARRIS_LINKS)} files updated\n")

    print("=== CSAPP side (02 → 00 links) ===")
    c_count = 0
    for rel_path, links in CSAPP_LINKS.items():
        if update_csapp_note(rel_path, links):
            c_count += 1
    print(f"  CSAPP: {c_count}/{len(CSAPP_LINKS)} files updated\n")

    print(f"Total: {h_count + c_count} files updated")
