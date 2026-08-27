# -*- coding: utf-8 -*-
"""通用 md → CFS 风格单文件 HTML 转换器
支持 chapter-XX-name/{README, notes/section-X.Y-*.md} 结构 + 独立 appendix-*.md。
跨书链接基于全局 section 编号（s-X-Y / app-X）解析到对应 html。
"""
import re, html as html_mod, os
from pathlib import Path
import markdown as md_lib

# ---------- 路径配置 ----------
SCRIPT = Path(__file__).resolve()
WORKSPACE = SCRIPT.parent  # build_all.py 所在目录（hft-reclone/）
REPO_GH = "https://github.com/cshonor/hft-embedded-linux-study/blob/main"
STYLE = (WORKSPACE / "cfs-style.css").read_text(encoding="utf-8")

# 每本书：root 相对仓库根，out_dir 相对仓库根，title_zh、sub_zh 用于封面
BOOKS = [
    dict(root="05-linux-kernel", out="05-linux-kernel/html",
         title_zh="Linux Kernel Development", sub_zh="Robert Love · LKD 3e 中文笔记"),
    dict(root="05.5-modern-kernel", out="05.5-modern-kernel/html",
         title_zh="现代内核特性", sub_zh="scheduler / RCU / arm64 / PREEMPT_RT · 笔记"),
    dict(root="05.6-kernel-debugging", out="05.6-kernel-debugging/html",
         title_zh="内核调试", sub_zh="printk / kprobes / ftrace / kgdb · 笔记"),
    dict(root="06-linux-mm", out="06-linux-mm/html",
         title_zh="Linux 虚拟内存管理", sub_zh="Mel Gorman ULVM + 附录 A–M 源码导读"),
    dict(root="06.5-modern-mm", out="06.5-modern-mm/html",
         title_zh="现代内存管理", sub_zh="memblock / slub / maple tree / mglru / DAMON"),
    dict(root="06.6-systems-performance", out="06.6-systems-performance/html",
         title_zh="Systems Performance", sub_zh="Brendan Gregg · 企业版中文笔记"),
    dict(root="06.7-bpf-observability/bpf-performance-tools", out="06.7-bpf-observability/bpf-performance-tools/html",
         title_zh="BPF Performance Tools", sub_zh="Brendan Gregg · 上下册笔记"),
    dict(root="06.7-bpf-observability/learning-ebpf", out="06.7-bpf-observability/learning-ebpf/html",
         title_zh="Learning eBPF", sub_zh="O'Reilly · 入门到 verifier"),
]

# ---------- 工具 ----------
def numkey(name):
    m = re.match(r"^(?:([A-Z])\.)?(\d+(?:\.\d+)*)", name)
    if not m:
        return (9, 9, 9)
    return tuple(int(x) for x in m.group(2).split("."))

def letterkey(name):
    m = re.match(r"^appendix-([A-Z])", name, re.I)
    return m.group(1).upper() if m else "Z"

def parse_section_no(stem):
    """section-X.Y-... → ('X.Y', 's-X-Y') ; 6.6-systems-performance 也兼容 2.10 等"""
    m = re.match(r"^section-(\d+(?:\.\d+)*)", stem, re.I)
    if not m:
        return None, None
    no = m.group(1)
    anchor = "s-" + no.replace(".", "-")
    return no, anchor

# ---------- 全局锚点索引（按书分桶，避免同号 section 冲突）----------
# book_root -> { anchor: html_filename_relative_to_book_html_dir }
BOOK_ANCHORS = {}
BOOK_ROOTS = sorted([b["root"] for b in BOOKS], key=lambda r: -len(r))  # 长前缀优先匹配

def register_book_anchors(book):
    """扫描一本书的所有 section 与 appendix，登记到该书的桶。返回该书的章节清单。"""
    root = WORKSPACE / book["root"]
    chapters, appendices = [], []
    book_map = {}
    for d in sorted([d for d in root.iterdir() if d.is_dir() and d.name.startswith("chapter-")], key=lambda x: numkey(x.name)):
        m = re.match(r"^chapter-(\d+)", d.name)
        chno = int(m.group(1))
        notes_dir = d / "notes"
        secs = []
        if notes_dir.is_dir():
            for f in sorted(notes_dir.glob("section-*.md"), key=lambda x: numkey(x.name)):
                _, anchor = parse_section_no(f.stem)
                if anchor:
                    secs.append((f, anchor))
                    book_map[anchor] = f"chapter-{chno:02d}.html"
        book_map[f"ch-{chno}"] = f"chapter-{chno:02d}.html"
        chapters.append((chno, d, secs))
    for f in sorted([f for f in root.iterdir() if f.is_file() and f.name.lower().startswith("appendix-")], key=lambda x: letterkey(x.name)):
        m = re.match(r"^appendix-([A-Z])", f.name, re.I)
        letter = m.group(1).upper() if m else "X"
        anchor = f"app-{letter}"
        book_map[anchor] = f"appendix-{letter}.html"
        appendices.append((letter, f))
    BOOK_ANCHORS[book["root"]] = book_map
    return chapters, appendices

# ---------- md 预处理 / 还原 ----------
def preprocess(text):
    text = re.sub(r"<details>\s*", "\n\n@@D@@\n\n", text)
    text = re.sub(r"<summary>(.*?)</summary>[ \t]*\n?", r"@@S@@\1@@ES@@\n\n", text, flags=re.S)
    text = re.sub(r"</details>\s*", "\n\n@@ED@@\n\n", text)
    return text

def restore_details(body):
    body = re.sub(r"<p>\s*@@D@@\s*</p>", "<details>", body)
    body = re.sub(r"<p>\s*@@ED@@\s*</p>", "</details>", body)
    body = re.sub(r"<p>\s*@@S@@(.*?)@@ES@@\s*</p>", r"<summary>\1</summary>", body, flags=re.S)
    body = body.replace("@@D@@", "<details>").replace("@@ED@@", "</details>")
    body = re.sub(r"@@S@@(.*?)@@ES@@", r"<summary>\1</summary>", body, flags=re.S)
    body = re.sub(r"<p>\s*(</?details>|</?summary>)\s*</p>", r"\1", body)
    return body

LANG_LABEL = {"c": "C", "sh": "shell", "bash": "shell", "shell": "shell", "asm": "asm",
              "text": "text", "": "code", "plaintext": "text", "console": "shell", "makefile": "make",
              "python": "py", "py": "py", "go": "go", "rust": "rust", "yaml": "yaml", "json": "json"}

# ---------- 链接改写 ----------
def rewrite_links(body, cur_html_rel):
    """cur_html_rel：当前页相对仓库根的路径，如 '05-linux-kernel/html/chapter-04.html'"""
    def rel_from_cur(target_rel_repo):
        """target_rel_repo 形如 '05-linux-kernel/html/chapter-03.html'（仓库根相对）"""
        cur_dir = (WORKSPACE / cur_html_rel).parent
        target = WORKSPACE / target_rel_repo
        return os.path.relpath(target, cur_dir).replace("\\", "/")

    # 当前页所在 book（最长前缀匹配）
    cur_book_root = ""
    for r in BOOK_ROOTS:
        if cur_html_rel == r + "/html" or cur_html_rel.startswith(r + "/html/"):
            cur_book_root = r
            break
    cur_book_anchors = BOOK_ANCHORS.get(cur_book_root, {})

    # 章内 section 缓存
    cur_html_name = Path(cur_html_rel).name
    m_cur = re.match(r"chapter-(\d+)", cur_html_name)
    cur_chno = int(m_cur.group(1)) if m_cur else None
    intra_sections = set()
    if cur_chno is not None:
        for k, v in cur_book_anchors.items():
            if k.startswith("s-") and v == cur_html_name:
                intra_sections.add(k)

    def resolve_target(repo_rel):
        """根据 repo_rel 解析出 (target_book_root, anchor_in_book, target_html_path_from_book_html_dir)"""
        # 1) 找最长的已知 book_root 前缀
        for r in BOOK_ROOTS:
            if repo_rel == r or repo_rel.startswith(r + "/"):
                tail = repo_rel[len(r)+1:] if repo_rel != r else ""
                # tail 形如 "chapter-02-hello-world/notes/section-2-..." 或 "chapter-02/"
                return r, tail
        # 2) 无前缀 → 当前书
        return cur_book_root, repo_rel

    def anchor_from_tail(tail, anchors_map):
        """在 tail 里找 section/chapter 锚点"""
        m_sec = re.search(r"notes/section-(\d+(?:\.\d+)*)", tail)
        if m_sec:
            return "s-" + m_sec.group(1).replace(".", "-")
        m_ch = re.match(r"chapter-(\d+)", tail)
        if m_ch:
            return f"ch-{int(m_ch.group(1))}"
        m_app = re.match(r"appendix-([A-Z])", tail, re.I)
        if m_app:
            return f"app-{m_app.group(1).upper()}"
        return None

    def repl(m):
        label, url = m.group(2), m.group(1)
        if url.startswith(("http://", "https://", "#", "mailto:")):
            return m.group(0)
        if "#" in url and not url.startswith("#"):
            path_part, frag = url.split("#", 1)
        else:
            path_part, frag = url, None
        if not path_part:
            return m.group(0)
        norm = re.sub(r"^\./", "", path_part).replace("\\", "/")
        repo_rel = re.sub(r"^(\.\./)+", "", norm)

        # 1) 章内裸 section 引用（无 chapter- 前缀，且指向当前章）
        m_sec_bare = re.match(r"section-(\d+(?:\.\d+)*)", repo_rel)
        if m_sec_bare and cur_chno is not None:
            anchor = "s-" + m_sec_bare.group(1).replace(".", "-")
            if anchor in intra_sections:
                return f'<a href="#{anchor}">{label}</a>'

        # 2) README.md 引用
        if repo_rel.endswith("README.md") and cur_chno is not None and "/" not in repo_rel:
            return f'<a href="#intro">{label}</a>'

        # 3) 解析目标书 + 锚点
        target_book, tail = resolve_target(repo_rel)
        target_anchors = BOOK_ANCHORS.get(target_book, {})
        anchor = anchor_from_tail(tail, target_anchors) if tail else None
        if anchor and anchor in target_anchors:
            html_file = target_anchors[anchor]
            repo_html_path = f"{target_book}/html/{html_file}"
            href = rel_from_cur(repo_html_path) + f"#{anchor}"
            return f'<a href="{href}">{label}</a>'

        # 4) 顶层 book 目录（无具体锚点，但有 html/）：跳到该书封面
        if re.match(r"^\d+(?:\.\d+)?-", repo_rel.split("/")[0]):
            book_rel = repo_rel.split("/")[0]
            idx = f"{book_rel}/html/index.html"
            if (WORKSPACE / book_rel / "html" / "index.html").exists():
                return f'<a href="{rel_from_cur(idx)}">{label}</a>'
            return f'<a href="{REPO_GH}/{repo_rel}">{label}</a>'

        # 5) 兜底：GitHub
        return f'<a href="{REPO_GH}/{repo_rel}">{label}</a>'
    return re.sub(r'<a href="([^"]+)">(.*?)</a>', repl, body, flags=re.S)

def convert_body(md_text, cur_html_rel):
    text = preprocess(md_text)
    body = md_lib.markdown(text, extensions=["tables", "fenced_code", "sane_lists"])
    def repl_pre(m):
        cls = m.group(1)
        lang = cls.replace("language-", "") if cls else ""
        label = LANG_LABEL.get(lang, lang or "code")
        return f'<pre class="code" data-label="{label}"><code>'
    body = re.sub(r'<pre><code(?: class="(language-[^"]*)")?>', repl_pre, body)
    body = re.sub(r"<code>", '<code class="c">', body)
    body = re.sub(r"(<table>)", r'<div class="tbl-wrap">\1', body)
    body = re.sub(r"(</table>)", r"\1</div>", body)
    body = restore_details(body)
    body = rewrite_links(body, cur_html_rel)
    return body

# ---------- 单个 section 渲染 ----------
def render_section(sec_path, anchor, cur_html_rel):
    text = sec_path.read_text(encoding="utf-8")
    lines = text.split("\n")
    title_m = re.match(r"^#\s+(.*)$", lines[0])
    title = title_m.group(1).strip() if title_m else sec_path.stem
    mm = re.match(r"^(\d+(?:\.\d+)*)\s*(.*)$", title)
    sec_no, sec_title = (mm.group(1), mm.group(2)) if mm else ("", title)
    rest = "\n".join(lines[1:])
    body = convert_body(rest, cur_html_rel)
    body = re.sub(r"^<h3([^>]*)>", r'<h3 class="sub"\1>', body)
    no_html = f'<div class="sec-no">{html_mod.escape(sec_no)}</div>' if sec_no else ""
    h = f'<h2>{html_mod.escape(sec_title or title)}</h2>'
    return f'      <section class="section" id="{anchor}">\n        <div class="sec-head">{no_html}{h}</div>\n{body}\n      </section>'

# ---------- README 解析 ----------
def render_readme(chapter_dir, cur_html_rel):
    p = chapter_dir / "README.md"
    if not p.exists():
        return None, None, "", ""
    text = p.read_text(encoding="utf-8")
    title = ""
    for ln in text.split("\n")[:6]:
        m = re.match(r"^#\s+(.*)$", ln)
        if m:
            title = m.group(1).strip()
            break
    blocks = re.split(r"\n(?=## )", text)
    intro_blocks, quiz_block = [], None
    for b in blocks:
        head = b.split("\n", 1)[0]
        if head.startswith("## 章节自测") or head.startswith("## 自测") or "自测" in head:
            quiz_block = b
        elif head.startswith("## 小节") or head.startswith("## 目录"):
            continue
        elif not head.startswith("# "):
            intro_blocks.append(b)
    intro_html = convert_body("\n".join(intro_blocks), cur_html_rel) if intro_blocks else ""
    quiz_html = ""
    if quiz_block:
        qbody = "\n".join(quiz_block.split("\n")[1:])
        qhtml = convert_body(qbody, cur_html_rel)
        def qa_repl(m):
            return f'<div class="qa"><div class="qa-q"><span class="qid">{m.group(1)}</span>{m.group(2)}</div><div class="qa-a">'
        qhtml = re.sub(r"<h3>(Q\d+):?\s*(.*?)</h3>", qa_repl, qhtml)
        parts = re.split(r'(<div class="qa">)', qhtml)
        rebuilt, buf = [], ""
        for part in parts:
            if part == '<div class="qa">':
                if buf:
                    rebuilt.append(buf + "</div></div>")
                buf = part
            else:
                buf += part
        if buf:
            rebuilt.append(buf + "</div></div>")
        qhtml = "".join(rebuilt)
        quiz_html = f'      <section class="section" id="quiz">\n        <div class="sec-head"><div class="sec-no">★</div><h2>章节自测</h2></div>\n{qhtml}\n      </section>'
    return title, None, intro_html, quiz_html

# ---------- 页面骨架 ----------
EXTRA_CSS = """
.section h3.sub{font-family:var(--font-display);font-size:20px;font-weight:600;letter-spacing:-.01em;color:var(--paper);margin:44px 0 14px;padding-bottom:8px;border-bottom:1px solid var(--line)}
.section h4{font-family:var(--font-display);font-size:16.5px;font-weight:600;color:var(--paper);margin:32px 0 10px}
.section h5{font-family:var(--font-mono);font-size:12.5px;letter-spacing:.08em;color:var(--red);text-transform:uppercase;margin:28px 0 8px}
.section .lead{color:var(--muted);font-size:16.5px;margin-bottom:22px}
.section p{margin:14px 0}
.section ul,.section ol{margin:14px 0 14px 4px;padding-left:22px}
.section li{margin:6px 0}
.section ul ul{margin:6px 0}
details{border:1px solid var(--line);border-radius:10px;background:var(--ink-2);margin:14px 0;overflow:hidden}
details summary{cursor:pointer;list-style:none;padding:11px 16px;font-family:var(--font-mono);font-size:12.5px;color:var(--red);letter-spacing:.06em;user-select:none}
details summary::-webkit-details-marker{display:none}
details summary::before{content:"▸ ";color:var(--red)}
details[open] summary::before{content:"▾ "}
details summary:hover{background:var(--ink-3)}
details > *:not(summary){margin:0 16px}
details p, details li{margin:10px 0;color:var(--paper)}
details .code{margin:12px 16px}
details .tbl-wrap{margin:12px 16px}
details blockquote{margin:12px 16px}
hr{border:none;border-top:1px solid var(--line);margin:34px 0}
.section a code.c{color:var(--red)}
.meta span.hft{border-color:var(--red-dim);color:var(--red)}
.nav a .n{min-width:2.6em}
"""

def page(ch_title, ch_sub, sections_html, quiz_html, nav_items, book_nav, stats, css_extra=""):
    toc = "\n".join(
        f'          <a href="#{a}"><span class="n">{n}</span>{t}</a>' for a, n, t in nav_items
    )
    meta = (
        f'<span>book <b>{html_mod.escape(stats["book"])}</b></span>'
        f'<span>chapter <b>{html_mod.escape(stats["no"])}</b></span>'
        f'<span>sections <b>{stats["secs"]}</b></span>'
        f'<span>quiz <b>{stats["quiz"]}</b></span>'
        f'<span class="hft">hft-embedded-linux-study</span>'
    )
    return f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{html_mod.escape(ch_title)}</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@500;600;700&family=JetBrains+Mono:wght@400;500;700&display=swap" rel="stylesheet">
<style>
{STYLE}
{EXTRA_CSS}
{css_extra}
</style>
</head>
<body>
<div id="progress"></div>
<div class="layout">
  <aside class="sidebar">
    <div class="brand">
      <div class="dot"></div>
      <div>
        <strong>{html_mod.escape(stats["book"])}</strong>
        <small>hft-embedded-linux-study</small>
      </div>
    </div>
    <div class="nav-label">本章</div>
    <nav class="nav">
{toc}
    </nav>
    <div class="nav-label">全书</div>
    <nav class="nav">
{book_nav}
    </nav>
  </aside>

  <main class="main">
    <header class="hero">
      <div class="hero-grid"></div>
      <div class="content" style="grid-template-columns:1fr">
        <div>
          <div class="eyebrow">{html_mod.escape(ch_sub)}</div>
          <h1>{html_mod.escape(ch_title)}</h1>
          <div class="meta">
{meta}
          </div>
        </div>
      </div>
    </header>

{sections_html}

{quiz_html}

    <footer class="footer">
      <div class="content">
        <p>Generated from <code class="c">{html_mod.escape(stats["from"])}</code> · hft-embedded-linux-study</p>
      </div>
    </footer>
  </main>
</div>
<script>
(function(){{
  var bar=document.getElementById('progress');
  function onScroll(){{
    var h=document.documentElement;
    var max=h.scrollHeight-h.clientHeight;
    bar.style.width=(max>0?(h.scrollTop/max)*100:0)+'%';
  }}
  document.addEventListener('scroll',onScroll,{{passive:true}});
  onScroll();
  var links=document.querySelectorAll('.sidebar .nav a[href^="#"]');
  var map={{}};
  links.forEach(function(a){{var id=a.getAttribute('href').slice(1);map[id]=a;}});
  var secs=Array.prototype.map.call(document.querySelectorAll('main [id]'),function(el){{return el.id;}});
  var obs=new IntersectionObserver(function(entries){{
    entries.forEach(function(e){{
      if(e.isIntersecting&&map[e.target.id]){{
        links.forEach(function(a){{a.classList.remove('active');}});
        map[e.target.id].classList.add('active');
      }}
    }});
  }},{{rootMargin:'-20% 0px -70% 0px',threshold:0}});
  secs.forEach(function(id){{var el=document.getElementById(id);if(el)obs.observe(el);}});
}})();
</script>
</body>
</html>
"""

# ---------- 构建一本书的所有页 ----------
def build_chapter(book, chno, chapter_dir, secs, book_nav, all_books_nav):
    html_file = f"{book['out']}/chapter-{chno:02d}.html"
    cur_rel = html_file
    nav_items, sections_html = [], []
    for sec_path, anchor in secs:
        sections_html.append(render_section(sec_path, anchor, cur_rel))
        text = sec_path.read_text(encoding="utf-8")
        first = text.split("\n", 1)[0]
        mm = re.match(r"^#\s+(\d+(?:\.\d+)*)\s*(.*)$", first)
        if mm:
            no, t = mm.group(1), mm.group(2).strip()
        else:
            no, t = "", sec_path.stem
        nav_items.append((anchor, no, t))
    title, _, intro_html, quiz_html = render_readme(chapter_dir, cur_rel)
    if not title:
        title = f"Ch {chno} · {chapter_dir.name}"
    if intro_html and intro_html.strip():
        intro_section = f'      <section class="section" id="intro">\n        <div class="sec-head"><div class="sec-no">00</div><h2>导读</h2></div>\n{intro_html}\n      </section>'
        sections_html.insert(0, intro_section)
        nav_items.insert(0, ("intro", "00", "导读"))
    if quiz_html:
        nav_items.append(("quiz", "★", "章节自测"))
    stats = {"book": " · ".join(book["root"].split("/")[-2:]),
             "no": f"Ch {chno}",
             "secs": len(nav_items) - (1 if quiz_html else 0),
             "quiz": "yes" if quiz_html else "—",
             "from": chapter_dir.relative_to(WORKSPACE).as_posix()}
    return page(title, book["sub_zh"], "\n".join(sections_html), quiz_html or "", nav_items, book_nav, stats)

def build_appendix(book, letter, app_path, book_nav):
    html_file = f"{book['out']}/appendix-{letter}.html"
    cur_rel = html_file
    text = app_path.read_text(encoding="utf-8")
    title = ""
    for ln in text.split("\n")[:6]:
        m = re.match(r"^#\s+(.*)$", ln)
        if m:
            title = m.group(1).strip()
            break
    if not title:
        title = f"Appendix {letter} · {app_path.stem}"
    body = convert_body("\n".join(text.split("\n")[1:]), cur_rel)
    body = re.sub(r"^<h3([^>]*)>", r'<h3 class="sub"\1>', body)
    sections_html = f'      <section class="section" id="app-{letter.lower()}">\n        <div class="sec-head"><div class="sec-no">{letter}</div><h2>{html_mod.escape(title)}</h2></div>\n{body}\n      </section>'
    nav_items = [("app-"+letter.lower(), letter, title)]
    stats = {"book": " · ".join(book["root"].split("/")[-2:]),
             "no": f"App {letter}",
             "secs": 1,
             "quiz": "—",
             "from": app_path.relative_to(WORKSPACE).as_posix()}
    return page(title, book["sub_zh"], sections_html, "", nav_items, book_nav, stats)

def build_book(book, chapters, appendices):
    """生成一本书的所有页 + 封面 index.html。返回页数。"""
    book_root = WORKSPACE / book["root"]
    out_dir = WORKSPACE / book["out"]
    out_dir.mkdir(parents=True, exist_ok=True)
    # 书内导航：本章文件 + 其它本文件
    all_pages = []
    for chno, _, _ in chapters:
        all_pages.append((chno, f"chapter-{chno:02d}.html"))
    for letter, _ in appendices:
        all_pages.append((letter, f"appendix-{letter}.html"))
    book_nav = "\n".join(
        f'          <a href="{h}#intro"><span class="n">{str(n).zfill(2) if isinstance(n,int) else n}</span>{html_mod.escape(book.get("chapter_titles",{}).get(n, h))}</a>'
        for n, h in all_pages
    )
    n = 0
    for chno, chapter_dir, secs in chapters:
        html = build_chapter(book, chno, chapter_dir, secs, book_nav, None)
        (out_dir / f"chapter-{chno:02d}.html").write_text(html, encoding="utf-8")
        n += 1
    for letter, app_path in appendices:
        html = build_appendix(book, letter, app_path, book_nav)
        (out_dir / f"appendix-{letter}.html").write_text(html, encoding="utf-8")
        n += 1
    return n, all_pages

# ---------- 顶层封面（跨全部 8 本） ----------
def build_top_index():
    """生成 hft-reclone/html/index.html —— 8 本书封面。"""
    rows = []
    for book in BOOKS:
        rel = book["out"]
        root = WORKSPACE / book["root"]
        nch = sum(1 for d in root.iterdir() if d.is_dir() and d.name.startswith("chapter-"))
        napp = sum(1 for f in root.iterdir() if f.is_file() and f.name.lower().startswith("appendix-"))
        meta = f"{nch} 章" + (f" + {napp} 附录" if napp else "")
        rows.append(
            f'      <a class="book-card" href="{rel}/index.html">'
            f'<div class="sec-no">{len(rows)+1:02d}</div>'
            f'<h2>{html_mod.escape(book["title_zh"])}</h2>'
            f'<p class="lead">{html_mod.escape(book["sub_zh"])} · {meta}</p>'
            f'<code class="c">{book["root"]}</code>'
            f'</a>'
        )
    grid = "\n".join(rows)
    out = WORKSPACE / "html" / "index.html"
    out.parent.mkdir(exist_ok=True)
    css_extra = """
.book-card{display:block;padding:24px 28px;border:1px solid var(--line);border-radius:12px;background:var(--ink-2);text-decoration:none;color:var(--paper);transition:all .15s;margin:10px 0}
.book-card:hover{border-color:var(--red);background:var(--ink-3);transform:translateX(4px)}
.book-card h2{font-family:var(--font-display);font-size:22px;font-weight:600;margin:8px 0 4px;color:var(--paper)}
.book-card .lead{color:var(--muted);font-size:14px;margin:0 0 10px}
.book-card .sec-no{color:var(--red);font-family:var(--font-mono);font-size:13px;letter-spacing:.06em}
"""
    body = f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>hft-embedded-linux-study · 笔记阅读版</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@500;600;700&family=JetBrains+Mono:wght@400;500;700&display=swap" rel="stylesheet">
<style>{STYLE}{EXTRA_CSS}{css_extra}</style>
</head>
<body>
<div id="progress"></div>
<div class="layout">
  <main class="main">
    <header class="hero">
      <div class="hero-grid"></div>
      <div class="content" style="grid-template-columns:1fr">
        <div>
          <div class="eyebrow">hft-embedded-linux-study</div>
          <h1>Notes<br><span class="accent">Reading Edition</span></h1>
          <p class="sub">HFT / 嵌入式 Linux / C / 内核 / 内存 / 性能 / eBPF — 全套逐章精读笔记，每章一单文件 HTML。</p>
          <div class="meta">
            <span>books <b>{len(BOOKS)}</b></span>
            <span>chapters <b>{sum(sum(1 for d in (WORKSPACE/b['root']).iterdir() if d.is_dir() and d.name.startswith('chapter-')) for b in BOOKS)}</b></span>
            <span class="hft">github.com/cshonor</span>
          </div>
        </div>
      </div>
    </header>
    <div class="content">
{grid}
    </div>
    <footer class="footer"><div class="content"><p>K&amp;R 笔记在 <a href="../01-c-language/01-Primer-K-and-R-C/html/index.html"><code class="c">01-c-language/01-Primer-K-and-R-C/html/</code></a> 单独维护。</p></div></footer>
  </main>
</div>
</body></html>"""
    out.write_text(body, encoding="utf-8")
    return len(BOOKS)

# ---------- 每本书封面 index.html ----------
def build_book_index(book, n_pages, all_pages):
    out_dir = WORKSPACE / book["out"]
    cards = []
    chapter_titles = build_chapter_titles(book)
    for n, h in all_pages:
        is_ch = isinstance(n, int)
        title = chapter_titles.get(n, h)
        no_str = f"{n:02d}" if is_ch else str(n)
        cards.append(
            f'      <a class="book-card" href="{h}">'
            f'<div class="sec-no">{no_str}</div>'
            f'<h2>{html_mod.escape(title)}</h2>'
            f'<p class="lead">{h}</p>'
            f'</a>'
        )
    grid = "\n".join(cards)
    css_extra = """
.book-card{display:block;padding:18px 22px;border:1px solid var(--line);border-radius:10px;background:var(--ink-2);text-decoration:none;color:var(--paper);transition:all .15s;margin:8px 0}
.book-card:hover{border-color:var(--red);background:var(--ink-3);transform:translateX(4px)}
.book-card h2{font-family:var(--font-display);font-size:18px;font-weight:600;margin:6px 0 2px;color:var(--paper)}
.book-card .lead{color:var(--muted);font-family:var(--font-mono);font-size:12px;margin:0}
.book-card .sec-no{color:var(--red);font-family:var(--font-mono);font-size:12px;letter-spacing:.06em}
"""
    body = f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{html_mod.escape(book["title_zh"])}</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@500;600;700&family=JetBrains+Mono:wght@400;500;700&display=swap" rel="stylesheet">
<style>{STYLE}{EXTRA_CSS}{css_extra}</style>
</head>
<body>
<div id="progress"></div>
<div class="layout">
  <main class="main">
    <header class="hero">
      <div class="hero-grid"></div>
      <div class="content" style="grid-template-columns:1fr">
        <div>
          <div class="eyebrow">{html_mod.escape(book["sub_zh"])}</div>
          <h1>{html_mod.escape(book["title_zh"])}</h1>
          <p class="sub">{n_pages} 份单文件 HTML · 全部章节聚合，离线可读。</p>
          <div class="meta">
            <span>book <b>{html_mod.escape(book["root"])}</b></span>
            <span>pages <b>{n_pages}</b></span>
            <span class="hft">hft-embedded-linux-study</span>
          </div>
        </div>
      </div>
    </header>
    <div class="content">
{grid}
    </div>
    <footer class="footer"><div class="content"><p><a href="../../../html/index.html"><code class="c">← 全套笔记总目录</code></a></p></div></footer>
  </main>
</div>
</body></html>"""
    (out_dir / "index.html").write_text(body, encoding="utf-8")

def build_chapter_titles(book):
    """从 README 头一行解析章节标题，作为封面卡片的副标题。"""
    titles = {}
    root = WORKSPACE / book["root"]
    for d in sorted([d for d in root.iterdir() if d.is_dir() and d.name.startswith("chapter-")], key=lambda x: numkey(x.name)):
        m = re.match(r"^chapter-(\d+)", d.name)
        chno = int(m.group(1))
        rd = d / "README.md"
        title = d.name
        if rd.exists():
            for ln in rd.read_text(encoding="utf-8").split("\n")[:6]:
                mm = re.match(r"^#\s+(.*)$", ln)
                if mm:
                    title = mm.group(1).strip()
                    # 去掉序号前缀 Ch X · 或 第 X 章
                    title = re.sub(r"^(Ch\s*\d+\s*[·\-:：]\s*|第\s*\d+\s*章\s*[·\-:：]?\s*)", "", title)
                    break
        titles[chno] = title
    for f in sorted([f for f in root.iterdir() if f.is_file() and f.name.lower().startswith("appendix-")], key=lambda x: letterkey(x.name)):
        m = re.match(r"^appendix-([A-Z])", f.name, re.I)
        letter = m.group(1).upper()
        title = f.stem
        for ln in f.read_text(encoding="utf-8").split("\n")[:6]:
            mm = re.match(r"^#\s+(.*)$", ln)
            if mm:
                title = mm.group(1).strip()
                title = re.sub(r"^附录\s*[A-Z]\s*[·\-:：]?\s*", "", title)
                break
        titles[letter] = title
    return titles

# ---------- main ----------
def main():
    # 1. 注册全局锚点（依赖全部书，但 books 间无交叉）
    book_meta = []  # (book, chapters, appendices, all_pages)
    for book in BOOKS:
        chapters, appendices = register_book_anchors(book)
        book_meta.append((book, chapters, appendices))

    # 2. 重新生成（先建章节需要先有锚点）
    total = 0
    for book, chapters, appendices in book_meta:
        # 把 chapter titles 注入到 book dict（封面用）
        book["chapter_titles"] = build_chapter_titles(book)
        n, all_pages = build_book(book, chapters, appendices)
        build_book_index(book, n, all_pages)
        total += n
        print(f"[{book['root']}] {n} 页")
    nb = build_top_index()
    print(f"\n总计 {total} 章/附录页 + {nb} 本书封面 + 1 顶层封面")

if __name__ == "__main__":
    main()
