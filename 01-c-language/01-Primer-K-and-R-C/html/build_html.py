# -*- coding: utf-8 -*-
"""K&R 笔记 md → CFS 风格单文件 HTML 转换器"""
import re, html as html_mod
from pathlib import Path
import markdown as md_lib

BOOK = Path(__file__).resolve().parent.parent
OUT = Path(__file__).resolve().parent
REPO_GH = "https://github.com/cshonor/hft-embedded-linux-study/blob/main/01-c-language/01-Primer-K-and-R-C"
STYLE = (Path(__file__).resolve().parent / "cfs-style.css").read_text(encoding="utf-8")

CHAPTERS = [
    ("ch01-introduction", "ch01.html", "第 1 章 导言", "A Tutorial Introduction"),
    ("ch02-types-operators-expressions", "ch02.html", "第 2 章 类型、运算符与表达式", "Types, Operators and Expressions"),
    ("ch03-control-flow", "ch03.html", "第 3 章 控制流", "Control Flow"),
    ("ch04-functions-and-program-structure", "ch04.html", "第 4 章 函数与程序结构", "Functions and Program Structure"),
    ("ch05-pointers-and-arrays", "ch05.html", "第 5 章 指针与数组", "Pointers and Arrays"),
    ("ch06-structures", "ch06.html", "第 6 章 结构体", "Structures"),
    ("ch07-input-and-output", "ch07.html", "第 7 章 输入与输出", "Input and Output"),
    ("ch08-unix-system-interface", "ch08.html", "第 8 章 UNIX 系统接口", "The UNIX System Interface"),
    ("appendix-a-reference-manual", "appendix-a.html", "附录 A 参考手册", "Reference Manual"),
    ("appendix-b-standard-library", "appendix-b.html", "附录 B 标准库", "Standard Library"),
    ("appendix-c-change-summary", "appendix-c.html", "附录 C 变更总结", "Summary of Changes"),
]
CHDIR2HTML = {d: h for d, h, _, _ in CHAPTERS}

def numkey(name):
    m = re.match(r"^(?:([A-Z])\.)?(\d+(?:\.\d+)*)", name)
    if not m:
        return (9, 9, 9)
    return tuple(int(x) for x in m.group(2).split("."))

# ---------- 全书索引：md 相对路径(book 内) -> (html 文件, 锚点) ----------
def build_index():
    idx = {}
    for chdir, htmlf, _, _ in CHAPTERS:
        chroot = BOOK / chdir
        for f in chroot.rglob("*.md"):
            if f.name == "README.md":
                continue
            rel = f.relative_to(BOOK).as_posix()
            num = re.match(r"^([A-Z]?\.?\d+(?:\.\d+)*)", f.stem)
            anchor = "s-" + num.group(1).replace(".", "-").replace(".", "").rstrip(".") if num else None
            anchor = "s-" + re.sub(r"[^0-9A-Za-z]", "", num.group(1)) if num else "sec"
            idx[rel] = (htmlf, anchor)
    return idx
INDEX = build_index()
# 文件名兜底索引：裸文件名链接（不带子目录前缀）也能命中
FN_INDEX = {Path(k).name: v for k, v in INDEX.items()}

# ---------- md 预处理 ----------
def preprocess(text):
    # details/summary 换哨兵，让内部 markdown 正常渲染；
    # summary 后强制空行，保证哨兵独占段落，还原时不受答案文本粘连影响
    text = re.sub(r"<details>\s*", "\n\n@@D@@\n\n", text)
    text = re.sub(r"<summary>(.*?)</summary>[ \t]*\n?", r"@@S@@\1@@ES@@\n\n", text, flags=re.S)
    text = re.sub(r"</details>\s*", "\n\n@@ED@@\n\n", text)
    return text

def restore_details(body):
    body = re.sub(r"<p>\s*@@D@@\s*</p>", "<details>", body)
    body = re.sub(r"<p>\s*@@ED@@\s*</p>", "</details>", body)
    body = re.sub(r"<p>\s*@@S@@(.*?)@@ES@@\s*</p>", r"<summary>\1</summary>", body, flags=re.S)
    # 兜底：未被段落包裹的哨兵
    body = body.replace("@@D@@", "<details>").replace("@@ED@@", "</details>")
    body = re.sub(r"@@S@@(.*?)@@ES@@", r"<summary>\1</summary>", body, flags=re.S)
    # 清理 details/summary 旁边的空段落
    body = re.sub(r"<p>\s*(</?details>|</?summary>)\s*</p>", r"\1", body)
    return body

LANG_LABEL = {"c": "C", "sh": "shell", "bash": "shell", "shell": "shell", "asm": "asm",
              "text": "text", "": "code", "plaintext": "text", "console": "shell", "makefile": "make"}

def convert_body(md_text, cur_chdir):
    """一段 md 正文 -> HTML 片段（不含 section 外壳）"""
    text = preprocess(md_text)
    body = md_lib.markdown(text, extensions=["tables", "fenced_code", "sane_lists"])
    # 代码块：<pre><code class="language-x"> -> pre.code[data-label]
    def repl_pre(m):
        cls = m.group(1)
        lang = cls.replace("language-", "") if cls else ""
        label = LANG_LABEL.get(lang, lang or "code")
        return '<pre class="code" data-label="%s"><code>' % label
    body = re.sub(r'<pre><code(?: class="(language-[^"]*)")?>', repl_pre, body)
    # 行内 code 加类
    body = re.sub(r"<code>", '<code class="c">', body)
    # 表格包一层
    body = re.sub(r"(<table>)", r'<div class="tbl-wrap">\1', body)
    body = re.sub(r"(</table>)", r"\1</div>", body)
    # details 哨兵还原
    body = restore_details(body)
    # 链接改写
    body = rewrite_links(body, cur_chdir)
    return body

def rewrite_links(body, cur_chdir):
    """作用于 markdown 输出后的 <a href>，改写笔记间链接"""
    def repl(m):
        label, url = m.group(2), m.group(1)
        if url.startswith(("http://", "https://", "#", "mailto:")):
            return m.group(0)
        target = url.split("#")[0]
        if not target:
            return m.group(0)
        norm = re.sub(r"^\./", "", target)
        norm = re.sub(r"^(\.\./)+", "", norm)
        for cand in (norm, f"{cur_chdir}/{norm}"):
            if cand in INDEX:
                htmlf, anchor = INDEX[cand]
                href = f"{htmlf}#{anchor}" if htmlf != CHDIR2HTML[cur_chdir] else f"#{anchor}"
                return f'<a href="{href}">{label}</a>'
        # 裸文件名兜底（链接未写子目录前缀）
        base = norm.split("/")[-1]
        if base in FN_INDEX:
            htmlf, anchor = FN_INDEX[base]
            href = f"{htmlf}#{anchor}" if htmlf != CHDIR2HTML[cur_chdir] else f"#{anchor}"
            return f'<a href="{href}">{label}</a>'
        gh = REPO_GH + "/" + norm
        return f'<a href="{gh}">{label}</a>'
    return re.sub(r'<a href="([^"]+)">(.*?)</a>', repl, body, flags=re.S)

# ---------- 单个小节 -> section ----------
def render_section(path, cur_chdir):
    text = path.read_text(encoding="utf-8")
    lines = text.split("\n")
    # 标题行
    title_m = re.match(r"^#\s+(.*)$", lines[0])
    title = title_m.group(1).strip() if title_m else path.stem
    mm = re.match(r"^([A-Z]?\.?\d+(?:\.\d+)*)\s*(.*)$", title)
    sec_no, sec_title = (mm.group(1), mm.group(2)) if mm else ("", title)
    anchor = "s-" + re.sub(r"[^0-9A-Za-z]", "", sec_no) if sec_no else "sec-" + re.sub(r"[^0-9A-Za-z]", "", path.stem)
    rest = "\n".join(lines[1:])
    body = convert_body(rest, cur_chdir)
    # ## 标题 -> h3，### -> h4
    body = re.sub(r"^<h3([^>]*)>", r'<h3 class="sub"\1>', body)
    no_html = f'<div class="sec-no">{html_mod.escape(sec_no)}</div>' if sec_no else ""
    h = f'<h2>{html_mod.escape(sec_title or title)}</h2>'
    return f'      <section class="section" id="{anchor}">\n        <div class="sec-head">{no_html}{h}</div>\n{body}\n      </section>'

# ---------- README -> 导读 + 自测 ----------
def render_readme(chdir):
    p = BOOK / chdir / "README.md"
    if not p.exists():
        return None, None, [], ("", "")
    text = p.read_text(encoding="utf-8")
    lines = text.split("\n")
    title = ""
    sub_en = ""
    for ln in lines[:4]:
        m = re.match(r"^#\s+(.*)$", ln)
        if m:
            title = m.group(1)
        m2 = re.match(r"^\*\*(.+?)\*\*$", ln.strip())
        if m2:
            sub_en = m2.group(1)
    # 分块：按 ## 标题
    blocks = re.split(r"\n(?=## )", text)
    intro_blocks, quiz_block = [], None
    for b in blocks:
        head = b.split("\n", 1)[0]
        if head.startswith("## 章节自测"):
            quiz_block = b
        elif head.startswith("## 小节"):
            continue  # 目录由真实 section 取代
        elif not head.startswith("# "):
            intro_blocks.append(b)
    intro_html = convert_body("\n".join(intro_blocks), chdir)
    quiz_html = ""
    if quiz_block:
        qbody = "\n".join(quiz_block.split("\n")[1:])
        qhtml = convert_body(qbody, chdir)
        # ### Qn: xxx -> qa 组件
        def qa_repl(m):
            return f'<div class="qa"><div class="qa-q"><span class="qid">{m.group(1)}</span>{m.group(2)}</div><div class="qa-a">'
        qhtml = re.sub(r"<h3>(Q\d+):?\s*(.*?)</h3>", qa_repl, qhtml)
        # 每个 qa-a 在下一个 qa 前闭合：按 qa 开头切分重排
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
    return title, sub_en, intro_blocks, (intro_html, quiz_html)

# ---------- 页面骨架 ----------
EXTRA_CSS = """
/* —— K&R 笔记适配补充 —— */
.section h3.sub{font-family:var(--font-display);font-size:20px;font-weight:600;letter-spacing:-.01em;color:var(--paper);margin:44px 0 14px;padding-bottom:8px;border-bottom:1px solid var(--line)}
.section h3.sub .kn{color:var(--red);font-family:var(--font-mono);font-size:13px;margin-right:10px}
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

def page(chdir, htmlf, ch_title, ch_sub, hero_intro_html, sections_html, quiz_html, nav_items, stats):
    toc = "\n".join(
        f'          <a href="#{a}"><span class="n">{n}</span>{t}</a>' for a, n, t in nav_items
    )
    book_nav = "\n".join(
        f'          <a href="{h}"><span class="n">{i:02d}</span>{t}</a>'
        for i, (d, h, t, _) in enumerate(CHAPTERS, 1)
    )
    meta = (
        f'<span>book <b>K&amp;R</b></span>'
        f'<span>chapter <b>{html_mod.escape(stats["no"])}</b></span>'
        f'<span>sections <b>{stats["secs"]}</b></span>'
        f'<span>quiz <b>{stats["quiz"]}</b></span>'
        f'<span class="hft">hft-embedded-linux-study</span>'
    )
    h1m = re.match(r"^(.*?)([，、].*)?$", ch_title)
    return f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{html_mod.escape(ch_title)} · K&R 笔记</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@500;600;700&family=JetBrains+Mono:wght@400;500;700&display=swap" rel="stylesheet">
<style>
{STYLE}
{EXTRA_CSS}
</style>
</head>
<body>
<div id="progress"></div>
<div class="layout">
  <aside class="sidebar">
    <div class="brand">
      <div class="dot"></div>
      <div>
        <strong>K&amp;R · The C Programming Language</strong>
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
          <div class="eyebrow">K&amp;R · {html_mod.escape(ch_sub)}</div>
          <h1>{html_mod.escape(ch_title)}</h1>
          <div class="content" style="padding:0;max-width:none">
{hero_intro_html}
          </div>
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
        <p>Generated from <code class="c">01-c-language/01-Primer-K-and-R-C/{chdir}</code> · hft-embedded-linux-study</p>
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

def build(chdir, htmlf, ch_title, ch_sub):
    chroot = BOOK / chdir
    files = sorted(
        [f for f in chroot.rglob("*.md") if f.name != "README.md" and f.parent == chroot or (f.name != "README.md")],
        key=lambda f: numkey(f.name),
    )
    files = [f for f in files if f.name != "README.md"]
    # 过滤 README 链接列表里实际不存在但文件在子目录的（如 1.5-character-io/1.5-xxx.md 已被 rglob 覆盖）
    title, sub_en, _, (intro_html, quiz_html) = render_readme(chdir)
    if not title:
        title, sub_en = ch_title, ch_sub
    sections = [render_section(f, chdir) for f in files]
    nav_items, seen = [], set()
    for f, sec in zip(files, sections):
        m = re.search(r'id="(s-[0-9A-Za-z]+)"', sec)
        anchor = m.group(1) if m else "sec"
        mm = re.search(r'<div class="sec-no">(.*?)</div>\s*<h2>(.*?)</h2>', sec, re.S)
        no = mm.group(1) if mm else ""
        t = mm.group(2) if mm else f.stem
        if anchor in seen:
            continue
        seen.add(anchor)
        nav_items.append((anchor, no, t))
    if quiz_html:
        nav_items.append(("quiz", "★", "章节自测"))
    stats = {"no": chdir.split("-")[0].replace("appendix", "A").upper() if chdir.startswith("appendix") else htmlf.replace(".html", "").replace("ch", "Ch "),
             "secs": len(nav_items) - (1 if quiz_html else 0),
             "quiz": "yes" if quiz_html else "—"}
    if chdir.startswith("appendix"):
        stats["no"] = chdir.split("-")[0].replace("appendix", "App ").upper()
    else:
        stats["no"] = "第 " + htmlf[2] + " 章"
    # 导读区：README intro 作为 hero 下方第一个 section
    intro_section = ""
    if intro_html and intro_html.strip():
        intro_section = f'      <section class="section" id="intro">\n        <div class="sec-head"><div class="sec-no">00</div><h2>导读</h2></div>\n{intro_html}\n      </section>\n'
        nav_items.insert(0, ("intro", "00", "导读"))
    all_secs = intro_section + "\n".join(sections)
    html = page(chdir, htmlf, title, sub_en or ch_sub, "", all_secs, quiz_html or "", nav_items, stats)
    return html, files, quiz_html

def main():
    OUT.mkdir(exist_ok=True)
    for chdir, htmlf, ch_title, ch_sub in CHAPTERS:
        html, files, quiz = build(chdir, htmlf, ch_title, ch_sub)
        (OUT / htmlf).write_text(html, encoding="utf-8")
        n = len(files)
        print(f"{htmlf:16s} <- {chdir:40s} {n:2d} sections, {len(html)//1024} KB, quiz={'Y' if quiz else 'N'}")

if __name__ == "__main__":
    main()
