#!/usr/bin/env python3
"""
clangd-snippet-proxy -- make clangd analyse non-self-contained headers in context

OpenFOAM-style code includes "snippet" headers (createFields.H, UEqn.H,
pEqn.H, ...) in the middle of a function body. Opened on their own, clangd
parses them at file scope and reports bogus errors; locals like 'mesh' or 'U'
are unknown. clangd cannot be told to parse a header in its includer's
context (and an '-include'd context does not work, because clangd seals
-include'd files into its preamble and closes any open function there).

This script is a transparent LSP proxy in front of the real clangd. When a
snippet is opened, it

  * finds the file that includes it (same directory first, then parent),
  * prepends the includer's text up to the '#include' line to the buffer
    clangd sees (recursively, if the includer is itself a snippet) and
    appends the closing braces / #endif's,
  * shifts all line numbers between client and clangd, so the editor only
    ever sees the snippet; locations inside the prepended context (e.g. the
    declaration of 'mesh') are mapped back to the real includer file.

A header counts as a snippet if it has no include guard and no
'#pragma once'. All other files are passed through unchanged.

Usage with Qt Creator:
  Preferences -> C++ -> Clangd -> "Path to executable": this script.
  The real clangd is taken from $INSIGHT_REAL_CLANGD, a '--real-clangd=PATH'
  argument or the file ~/.config/clangd-snippet-proxy (containing the path).
  Otherwise the clangd bundled with the calling Qt Creator is used, then
  PATH and the usual Qt Creator / LLVM locations are searched.
  After changing an includer, save it: open snippets are refreshed on save.
  Qt Creator cannot locate clang's builtin headers (limits.h, ...) next to
  this script; the proxy then gives clangd a patched copy of the compilation
  database in ~/.cache/clangd-snippet-proxy (kept in sync automatically).
  Proxy messages go to stderr (visible in Qt Creator's language client log).
"""

import faulthandler
import glob
import hashlib
import json
import os
import re
import shlex
import signal
import subprocess
import sys
import threading
import time
import urllib.parse
import urllib.request


LOG_FILE = None  # opened in main(): ~/.cache/clangd-snippet-proxy/proxy.log


def log(*args):
    print("[snippet-proxy]", *args, file=sys.stderr, flush=True)
    if LOG_FILE is not None:
        print(time.strftime("%H:%M:%S"), os.getpid(), *args, file=LOG_FILE, flush=True)


# ---------------------------------------------------------------------------
# locating the real clangd
# ---------------------------------------------------------------------------

CONFIG_FILE = os.path.expanduser("~/.config/clangd-snippet-proxy")


def find_real_clangd(argv):
    args = []
    explicit = os.environ.get("INSIGHT_REAL_CLANGD")
    for a in argv:
        if a.startswith("--real-clangd="):
            explicit = a.split("=", 1)[1]
        else:
            args.append(a)
    if not explicit:
        try:
            with open(CONFIG_FILE) as f:
                explicit = f.read().strip() or None
        except OSError:
            pass
    if explicit:
        return os.path.expanduser(explicit), args

    me = os.path.realpath(__file__)
    candidates = []
    # the clangd bundled with the Qt Creator that started us
    # (<prefix>/bin/qtcreator -> <prefix>/libexec/qtcreator/clang/bin/clangd)
    try:
        parent = os.path.realpath("/proc/%d/exe" % os.getppid())
        prefix = os.path.dirname(os.path.dirname(parent))
        candidates += [
            os.path.join(prefix, "libexec/qtcreator/clang/bin/clangd"),
            os.path.join(prefix, "lib/qtcreator/libexec/clang/bin/clangd"),
        ]
    except OSError:
        pass
    for d in os.environ.get("PATH", "").split(os.pathsep):
        candidates.append(os.path.join(d, "clangd"))
    candidates += [
        os.path.expanduser("~/Qt/Tools/QtCreator/libexec/qtcreator/clang/bin/clangd"),
        "/opt/Qt/Tools/QtCreator/libexec/qtcreator/clang/bin/clangd",
        "/opt/qtcreator/libexec/qtcreator/clang/bin/clangd",
        "/usr/libexec/qtcreator/clang/bin/clangd",
    ]
    candidates += glob.glob(os.path.expanduser(
        "~/qtcreator*/libexec/qtcreator/clang/bin/clangd"))
    candidates += glob.glob("/opt/*/libexec/qtcreator/clang/bin/clangd")

    def llvm_version(p):
        m = re.search(r"llvm-(\d+)", p)
        return int(m.group(1)) if m else 0

    candidates += sorted(glob.glob("/usr/lib/llvm-*/bin/clangd"),
                         key=llvm_version, reverse=True)
    for c in candidates:
        if os.path.isfile(c) and os.access(c, os.X_OK) \
                and os.path.realpath(c) != me:
            return c, args
    return None, args



# ---------------------------------------------------------------------------
# clang builtin headers
#
# Qt Creator passes -nostdinc plus '-isystem <clangd>/../lib/clang/<v>/include'
# derived from the configured clangd executable. With this proxy configured,
# that directory does not exist and Qt Creator leaves it out, so 'limits.h',
# 'stddef.h' etc. are not found. The proxy therefore hands clangd a patched
# copy of the compilation database that contains the real clangd's directory.
# ---------------------------------------------------------------------------

BUILTIN_RE = re.compile(r".*/lib/clang/[^/]+/include/?$")
CACHE_DIR = os.path.expanduser("~/.cache/clangd-snippet-proxy")


def find_builtin_include(clangd):
    root = os.path.dirname(os.path.dirname(os.path.realpath(clangd)))
    found = [d for d in glob.glob(os.path.join(root, "lib", "clang", "*", "include"))
             if os.path.isfile(os.path.join(d, "limits.h"))]

    def version(d):
        v = os.path.basename(os.path.dirname(d))
        return [int(x) if x.isdigit() else 0 for x in v.split(".")]

    return max(found, key=version) if found else None


def patch_arguments(args, builtin):
    """Return args with a usable builtin include dir, or None if unchanged."""
    isys = [i for i in range(len(args) - 1)
            if args[i] == "-isystem" and BUILTIN_RE.match(args[i + 1])]
    if any(os.path.isfile(os.path.join(args[i + 1], "limits.h")) for i in isys):
        return None
    args = list(args)
    if isys:
        for i in isys:
            args[i + 1] = builtin
        return args
    # where Qt Creator puts it: before the system C include directories
    for i in range(len(args) - 1):
        d = args[i + 1]
        if args[i] == "-isystem" and (d == "/usr/include" or d.startswith("/usr/include/")) \
                and "/c++/" not in d:
            return args[:i] + ["-isystem", builtin] + args[i:]
    if "-nostdinc" not in args:
        return None  # clangd adds its builtin headers by itself
    # before the input file (and a preceding '-x <lang>')
    pos = len(args) - 1
    if pos >= 2 and args[pos - 2] == "-x":
        pos -= 2
    return args[:pos] + ["-isystem", builtin] + args[pos:]


def patch_compile_db(src_dir, builtin):
    """Write a patched copy of src_dir/compile_commands.json.

    Returns the directory of the copy, or None if no patch is needed.
    """
    src = os.path.join(src_dir, "compile_commands.json")
    with open(src, encoding="utf-8") as f:
        db = json.load(f)
    changed = False
    for e in db:
        if "arguments" in e:
            new = patch_arguments(e["arguments"], builtin)
            if new is not None:
                e["arguments"] = new
                changed = True
        elif "command" in e:
            new = patch_arguments(shlex.split(e["command"]), builtin)
            if new is not None:
                e["command"] = " ".join(shlex.quote(a) for a in new)
                changed = True
    if not changed:
        return None
    out_dir = os.path.join(
        CACHE_DIR, hashlib.sha1(os.path.abspath(src_dir).encode()).hexdigest())
    os.makedirs(out_dir, exist_ok=True)
    out = os.path.join(out_dir, "compile_commands.json")
    text = json.dumps(db, indent=1)
    try:
        with open(out, encoding="utf-8") as f:
            if f.read() == text:
                return out_dir
    except OSError:
        pass
    tmp = out + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        f.write(text)
    os.replace(tmp, out)
    return out_dir


_redirected = {}  # original database dir -> patched copy dir


def redirect_db_dir(builtin, src_dir):
    """Patched copy of src_dir's compile database, or None if not needed.

    A thread keeps the copy in sync with the original.
    """
    if builtin is None or not src_dir:
        return None
    src_dir = os.path.abspath(src_dir)
    if src_dir in _redirected:
        return _redirected[src_dir]
    src = os.path.join(src_dir, "compile_commands.json")
    if not os.path.isfile(src):
        return None
    try:
        out_dir = patch_compile_db(src_dir, builtin)
    except Exception as e:
        log("could not patch compile database:", repr(e))
        return None
    _redirected[src_dir] = out_dir
    if out_dir is None:
        log("compile database", src_dir, "already has clang builtin headers")
        return None
    log("compile database", src_dir, "lacks clang builtin headers;",
        "using patched copy in", out_dir, "with", builtin)

    def watcher():
        last = os.stat(src).st_mtime
        while True:
            time.sleep(3)
            try:
                m = os.stat(src).st_mtime
                if m != last:
                    last = m
                    patch_compile_db(src_dir, builtin)
                    log("re-patched updated compile database")
            except Exception as e:
                log("could not re-patch compile database:", repr(e))

    threading.Thread(target=watcher, daemon=True).start()
    return out_dir


def redirect_compile_db(builtin, args):
    """Point a --compile-commands-dir argument to a patched copy."""
    args = list(args)
    for i, a in enumerate(args):
        if a.startswith("--compile-commands-dir="):
            out = redirect_db_dir(builtin, a.split("=", 1)[1])
            if out:
                args[i] = "--compile-commands-dir=" + out
        elif a == "--compile-commands-dir" and i + 1 < len(args):
            out = redirect_db_dir(builtin, args[i + 1])
            if out:
                args[i + 1] = out
    return args


def patch_db_changes(builtin, changes):
    """Patch clangd's 'compilationDatabaseChanges' (file -> command)."""
    if builtin is None or not isinstance(changes, dict):
        return changes
    n = 0
    for v in changes.values():
        cmd = v.get("compilationCommand") if isinstance(v, dict) else None
        if isinstance(cmd, list):
            new = patch_arguments(cmd, builtin)
            if new is not None:
                v["compilationCommand"] = new
                n += 1
    if n:
        log("added clang builtin headers to %d pushed compile commands" % n)
    return changes


# ---------------------------------------------------------------------------
# text helpers
# ---------------------------------------------------------------------------

def uri_to_path(uri):
    if not uri.startswith("file://"):
        return None
    return urllib.request.url2pathname(urllib.parse.urlparse(uri).path)


def path_to_uri(path):
    return "file://" + urllib.request.pathname2url(os.path.abspath(path))


def split_lines(text):
    """Lines without terminators; always at least one element."""
    return text.split("\n")


def utf16_to_index(line, character):
    """Convert an LSP UTF-16 column into a Python string index."""
    units = 0
    for i, ch in enumerate(line):
        if units >= character:
            return i
        units += 2 if ord(ch) > 0xFFFF else 1
    return len(line)


def apply_change(text, change):
    if "range" not in change:
        return change["text"]
    lines = split_lines(text)

    def offset(pos):
        ln = min(pos["line"], len(lines) - 1)
        base = sum(len(l) + 1 for l in lines[:ln])
        return base + utf16_to_index(lines[ln], pos["character"])

    s = offset(change["range"]["start"])
    e = offset(change["range"]["end"])
    return text[:s] + change["text"] + text[e:]


INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
GUARD_RE = re.compile(r'^\s*#\s*(ifndef|pragma\s+once)\b', re.M)
PP_IF_RE = re.compile(r'^\s*#\s*if(n?def)?\b')
PP_ENDIF_RE = re.compile(r'^\s*#\s*endif\b')
SOURCE_EXTS = (".C", ".cpp", ".cxx", ".cc", ".c")
HEADER_EXTS = (".H", ".h", ".hpp", ".hxx", ".hh")


def is_snippet(text):
    # self-contained headers start (after comments) with an include guard
    # or '#pragma once'; snippets have neither
    return GUARD_RE.search(text) is None


def open_depth(lines):
    """Brace depth and open #if count at the end of 'lines'.

    Skips comments, string and character literals and preprocessor lines.
    """
    depth = 0
    pp_depth = 0
    in_block_comment = False
    continued_directive = False
    for line in lines:
        stripped = line.lstrip()
        if not in_block_comment and (continued_directive or stripped.startswith("#")):
            if not continued_directive:
                if PP_IF_RE.match(line):
                    pp_depth += 1
                elif PP_ENDIF_RE.match(line):
                    pp_depth = max(0, pp_depth - 1)
            continued_directive = line.rstrip().endswith("\\")
            continue
        i = 0
        n = len(line)
        while i < n:
            c = line[i]
            if in_block_comment:
                if line.startswith("*/", i):
                    in_block_comment = False
                    i += 2
                    continue
                i += 1
                continue
            if line.startswith("//", i):
                break
            if line.startswith("/*", i):
                in_block_comment = True
                i += 2
                continue
            if c in "\"'":
                i += 1
                while i < n and line[i] != c:
                    i += 2 if line[i] == "\\" else 1
                i += 1
                continue
            if c == "{":
                depth += 1
            elif c == "}":
                depth = max(0, depth - 1)
            i += 1
    return depth, pp_depth


# ---------------------------------------------------------------------------
# context computation
# ---------------------------------------------------------------------------

class Context:
    """Text wrapped around a snippet, plus the origin of each prefix line."""

    def __init__(self, prefix_lines, origins, suffix, sources):
        self.prefix_lines = prefix_lines  # list of str
        self.origins = origins            # per prefix line: (uri, line)
        self.suffix = suffix              # str
        self.sources = sources            # set of paths this depends on

    @property
    def n(self):
        return len(self.prefix_lines)

    def wrap(self, text):
        prefix = "".join(l + "\n" for l in self.prefix_lines)
        return prefix + text + self.suffix

    def key(self):
        return ("\n".join(self.prefix_lines), self.suffix)


class ContextFinder:
    def __init__(self, read_text):
        self.read_text = read_text  # path -> text (open buffer or disk)

    def candidate_includers(self, path):
        d = os.path.dirname(path)
        result = []
        for directory in (d, os.path.dirname(d)):
            try:
                names = sorted(os.listdir(directory))
            except OSError:
                continue
            files = [os.path.join(directory, n) for n in names
                     if n.endswith(SOURCE_EXTS + HEADER_EXTS)]
            # prefer sources over headers
            files.sort(key=lambda f: (not f.endswith(SOURCE_EXTS), f))
            result += [f for f in files if os.path.abspath(f) != path]
        return result

    def find_include(self, includer, target):
        """Line index of the first '#include' in includer resolving to target."""
        text = self.read_text(includer)
        if text is None:
            return None, None
        base = os.path.dirname(includer)
        name = os.path.basename(target)
        for i, line in enumerate(split_lines(text)):
            m = INCLUDE_RE.match(line)
            if m and os.path.basename(m.group(1)) == name:
                if os.path.abspath(os.path.join(base, m.group(1))) == target:
                    return text, i
        return None, None

    def context(self, path, _stack=()):
        path = os.path.abspath(path)
        if path in _stack:
            return None
        for includer in self.candidate_includers(path):
            text, idx = self.find_include(includer, path)
            if text is None:
                continue
            outer = None
            if includer.endswith(HEADER_EXTS) and is_snippet(text):
                outer = self.context(includer, _stack + (path,))
            return self.build(includer, text, idx, outer)
        return None

    def build(self, includer, text, idx, outer):
        uri = path_to_uri(includer)
        base = os.path.dirname(includer)
        lines = split_lines(text)[:idx]
        own = []
        for line in lines:
            # make relative includes independent of the snippet's directory
            m = INCLUDE_RE.match(line)
            if m and not os.path.isabs(m.group(1)):
                cand = os.path.join(base, m.group(1))
                if os.path.isfile(cand):
                    line = line[:m.start(1)] + os.path.abspath(cand) + line[m.end(1):]
            own.append(line)
        if outer is not None:
            prefix = outer.prefix_lines + own
            origins = outer.origins + [(uri, i) for i in range(len(own))]
            sources = outer.sources | {includer}
        else:
            prefix = own
            origins = [(uri, i) for i in range(len(own))]
            sources = {includer}
        braces, pps = open_depth(prefix)
        suffix = "\n" + "}" * braces + "\n" + "#endif\n" * pps
        return Context(prefix, origins, suffix, sources)


# ---------------------------------------------------------------------------
# LSP transport
# ---------------------------------------------------------------------------

def read_raw_message(stream):
    """The body of the next message as bytes, or None at end of stream."""
    length = None
    while True:
        line = stream.readline()
        if not line:
            return None
        line = line.strip()
        if not line:
            break
        k, _, v = line.decode("ascii").partition(":")
        if k.strip().lower() == "content-length":
            length = int(v.strip())
    if length is None:
        return None
    return stream.read(length)


def read_message(stream):
    body = read_raw_message(stream)
    return None if body is None else json.loads(body.decode("utf-8"))


def write_message(stream, lock, msg):
    body = json.dumps(msg, ensure_ascii=False).encode("utf-8")
    write_raw_message(stream, lock, body)


def write_raw_message(stream, lock, body):
    with lock:
        stream.write(b"Content-Length: %d\r\n\r\n" % len(body))
        stream.write(body)
        stream.flush()


# ---------------------------------------------------------------------------
# the proxy
# ---------------------------------------------------------------------------

DROP = object()
VIRTUAL_TAG = ".snippet-ctx"
# members giving an object's own position; if one of them lies outside the
# snippet, the whole object is dropped
POSITION_KEYS = ("range", "selectionRange", "position", "location", "start", "end")
VERSION_FACTOR = 1000
URI_KEYS = ("uri", "targetUri", "file")


class Doc:
    def __init__(self, uri, text, version, language="cpp"):
        self.uri = uri
        self.path = uri_to_path(uri)
        self.text = text
        self.version = version
        self.language = language
        self.ctx = None
        self.bump = 0

    @property
    def server_uri(self):
        """The name under which clangd knows this document.

        A wrapped snippet must not be known to clangd under its real name:
        clangd would then use the wrapped buffer whenever the includer
        includes the snippet (--use-dirty-headers). So it gets a virtual
        name in the same directory, which never exists on disk.
        """
        if self.ctx is None:
            return self.uri
        d, name = os.path.split(self.path)
        stem, ext = os.path.splitext(name)
        return path_to_uri(os.path.join(d, ".%s%s%s" % (stem, VIRTUAL_TAG, ext)))

    @property
    def nlines(self):
        return len(split_lines(self.text))


class Proxy:
    def __init__(self, server, builtin=None):
        self.server = server
        self.builtin = builtin
        self.lock = threading.RLock()
        self.client_out_lock = threading.Lock()
        self.server_in_lock = threading.Lock()
        self.docs = {}       # client uri -> Doc
        self.virtual = {}    # server uri of a wrapped doc -> Doc
        self.requests = {}   # client request id -> (uri, method)
        self.finder = ContextFinder(self.read_text)

    # --- helpers ---------------------------------------------------------

    def read_text(self, path):
        with self.lock:
            for d in self.docs.values():
                if d.path == path:
                    return d.text
        try:
            with open(path, encoding="utf-8", errors="replace") as f:
                return f.read()
        except OSError:
            return None

    def to_server(self, msg):
        write_message(self.server.stdin, self.server_in_lock, msg)

    def to_client(self, msg):
        write_message(sys.stdout.buffer, self.client_out_lock, msg)

    def wrapped(self, uri):
        """Wrapped doc by its client uri."""
        d = self.docs.get(uri)
        return d if d is not None and d.ctx is not None else None

    def set_context(self, d, ctx):
        self.virtual.pop(d.server_uri, None)
        d.ctx = ctx
        if ctx is not None:
            self.virtual[d.server_uri] = d

    def server_version(self, d):
        return d.version * VERSION_FACTOR + d.bump

    def compute_context(self, d):
        if d.path is None or not d.path.endswith(HEADER_EXTS) \
                or not is_snippet(d.text):
            return None
        try:
            ctx = self.finder.context(d.path)
        except Exception as e:  # never break the editor because of us
            log("context computation failed for", d.path, ":", e)
            return None
        if ctx is not None:
            log("context for", d.path, "from",
                ", ".join(sorted(ctx.sources)),
                "(%d lines prepended)" % ctx.n)
        return ctx

    # --- position mapping --------------------------------------------------

    def pos_to_server(self, pos, d):
        return {"line": pos["line"] + d.ctx.n, "character": pos["character"]}

    def classify(self, line, d):
        """'prefix', 'body' or 'suffix' for a server-side line."""
        if line < d.ctx.n:
            return "prefix"
        if line < d.ctx.n + d.nlines:
            return "body"
        return "suffix"

    def range_to_client(self, rng, d):
        """Map a range of a wrapped doc. Returns a range, or DROP."""
        s, e = rng["start"], rng["end"]
        cs, ce = self.classify(s["line"], d), self.classify(e["line"], d)
        if cs != "body" and cs == ce:
            return DROP
        n = d.ctx.n
        if cs == "prefix":
            s = {"line": 0, "character": 0}
        else:
            s = {"line": s["line"] - n, "character": s["character"]}
        if ce == "suffix":
            last = split_lines(d.text)[-1]
            e = {"line": d.nlines - 1, "character": len(last.encode("utf-16-le")) // 2}
        else:
            e = {"line": e["line"] - n, "character": e["character"]}
        return {"start": s, "end": e}

    def location_to_client(self, uri, rng, d):
        """Map (uri, range) of a wrapped doc, possibly into the includer."""
        line = rng["start"]["line"]
        if line < d.ctx.n and rng["end"]["line"] < d.ctx.n:
            ouri, oline = d.ctx.origins[line]
            eline = d.ctx.origins[rng["end"]["line"]]
            if eline[0] != ouri:
                return DROP
            return ouri, {
                "start": {"line": oline, "character": rng["start"]["character"]},
                "end": {"line": eline[1], "character": rng["end"]["character"]},
            }
        r = self.range_to_client(rng, d)
        if r is DROP:
            return DROP
        return d.uri, r

    # generic JSON walker, client -> server
    def map_in(self, obj, uri=None):
        if isinstance(obj, list):
            return [self.map_in(x, uri) for x in obj]
        if not isinstance(obj, dict):
            return obj
        uri = self.uri_of(obj, uri)
        d = self.wrapped(uri) if uri else None
        if d is not None and set(obj.keys()) == {"line", "character"}:
            return self.pos_to_server(obj, d)
        out = {}
        for k, v in obj.items():
            if d is not None and k == "version" and isinstance(v, int):
                out[k] = v * VERSION_FACTOR + d.bump
            elif k in URI_KEYS and isinstance(v, str) and self.wrapped(v):
                out[k] = self.wrapped(v).server_uri
            elif isinstance(v, dict) and k == "changes" and self.is_uri_map(v):
                out[k] = {(self.wrapped(u).server_uri if self.wrapped(u) else u):
                          self.map_in(edits, u) for u, edits in v.items()}
            else:
                out[k] = self.map_in(v, uri)
        return out

    @staticmethod
    def is_uri_map(v):
        return all(isinstance(k, str) and k.startswith("file://") for k in v)

    @staticmethod
    def uri_of(obj, default):
        for k in URI_KEYS:
            v = obj.get(k)
            if isinstance(v, str) and v.startswith("file://"):
                return v
        td = obj.get("textDocument")
        if isinstance(td, dict) and isinstance(td.get("uri"), str):
            return td["uri"]
        return default

    # generic JSON walker, server -> client
    #
    # Wrapped coordinates come with the virtual uri of a wrapped doc; the
    # real uri of a snippet always refers to the file itself (e.g. locations
    # from the includer's translation unit).
    ALL = object()

    def subject_doc(self, uri, subject=None):
        return self.virtual.get(uri) if isinstance(uri, str) else None

    def map_out(self, obj, subject, uri=None):
        if isinstance(obj, list):
            out = []
            for x in obj:
                y = self.map_out(x, subject, uri)
                if y is DROP:
                    # e.g. the document symbol 'main' of the context:
                    # keep the children that are inside the snippet
                    if isinstance(x, dict) and isinstance(x.get("children"), list):
                        out += [c for c in self.map_out(x["children"], subject, uri)
                                if c is not DROP]
                    continue
                out.append(y)
            return out
        if not isinstance(obj, dict):
            return obj

        # Location / LocationLink / hierarchy items may move to another file
        for ukey, rkeys in (("uri", ("range", "selectionRange")),
                            ("targetUri", ("targetRange", "targetSelectionRange"))):
            u = obj.get(ukey)
            d = self.subject_doc(u, subject)
            rkeys = [r for r in rkeys if isinstance(obj.get(r), dict)]
            if d is None or not rkeys:
                continue
            res = [self.location_to_client(u, obj[r], d) for r in rkeys]
            if any(r is DROP for r in res) or len({r[0] for r in res}) != 1:
                return DROP
            new_uri = res[0][0]
            rest = {k: v for k, v in obj.items() if k != ukey and k not in rkeys}
            new = self.map_out(rest, subject, u if new_uri == d.uri else new_uri)
            if new is DROP:
                return DROP
            new[ukey] = new_uri
            for r, (_, rng) in zip(rkeys, res):
                new[r] = rng
            return new

        uri = self.uri_of(obj, uri)
        d = self.subject_doc(uri, subject)

        if d is not None:
            keys = set(obj.keys())
            if keys == {"line", "character"}:
                if self.classify(obj["line"], d) != "body":
                    return DROP
                return {"line": obj["line"] - d.ctx.n, "character": obj["character"]}
            if keys == {"start", "end"}:
                return self.range_to_client(obj, d)
            if "startLine" in obj and "endLine" in obj:  # FoldingRange
                # blocks opened in the context would fold the whole snippet
                if self.classify(obj["startLine"], d) != "body":
                    return DROP
                r = self.range_to_client(
                    {"start": {"line": obj["startLine"], "character": 0},
                     "end": {"line": obj["endLine"], "character": 0}}, d)
                if r is DROP:
                    return DROP
                obj = dict(obj, startLine=r["start"]["line"], endLine=r["end"]["line"])

        # check the position of the object itself before descending into
        # its members: a dropped node's children are mapped by the caller
        # (list splicing), and mapping them twice per level is exponential
        # in the nesting depth (e.g. clangd's AST)
        out = {}
        for k in POSITION_KEYS:
            if k in obj:
                y = self.map_out(obj[k], subject, uri)
                if y is DROP:
                    return DROP
                out[k] = y
        for k, v in obj.items():
            if k in out:
                continue
            if d is not None and k == "version" and isinstance(v, int):
                out[k] = v // VERSION_FACTOR
                continue
            if d is not None and k in ("startLine", "endLine"):
                out[k] = v
                continue
            if k in URI_KEYS and isinstance(v, str) and v in self.virtual:
                out[k] = self.virtual[v].uri
                continue
            if isinstance(v, dict) and k == "changes" and self.is_uri_map(v):
                out[k] = self.map_workspace_changes(v)
                continue
            if k == "documentChanges":
                y = self.map_out(v, self.ALL, uri)
            else:
                y = self.map_out(v, subject, uri)
            if y is DROP:
                continue  # optional members: just leave them out
            out[k] = y
        return out

    def map_workspace_changes(self, changes):
        """WorkspaceEdit.changes: edits may move from a snippet to its includer."""
        result = {}
        for u, edits in changes.items():
            d = self.virtual.get(u)
            for e in edits:
                if d is None:
                    result.setdefault(u, []).append(e)
                    continue
                loc = self.location_to_client(u, e["range"], d)
                if loc is DROP:
                    continue
                nu, rng = loc
                result.setdefault(nu, []).append(dict(e, range=rng))
        return result

    def semantic_tokens_to_client(self, result, d):
        data = result.get("data") if isinstance(result, dict) else None
        if not isinstance(data, list):
            return result
        out = []
        line = char = 0
        pline = pchar = 0
        first = True
        for i in range(0, len(data) - 4, 5):
            dl, dc, length, ttype, tmod = data[i:i + 5]
            line += dl
            char = dc if dl else char + dc
            if self.classify(line, d) != "body":
                continue
            cl = line - d.ctx.n
            if first:
                out += [cl, char, length, ttype, tmod]
                first = False
            else:
                ndl = cl - pline
                out += [ndl, char if ndl else char - pchar, length, ttype, tmod]
            pline, pchar = cl, char
        return dict(result, data=out)

    # --- client -> server ------------------------------------------------

    def handle_client(self, msg):
        method = msg.get("method")
        params = msg.get("params") or {}
        with self.lock:
            if method == "initialize":
                opts = params.get("initializationOptions")
                if isinstance(opts, dict):
                    log("initializationOptions:", sorted(opts.keys()))
                    out = redirect_db_dir(self.builtin, opts.get("compilationDatabasePath"))
                    if out:
                        opts["compilationDatabasePath"] = out
                    patch_db_changes(self.builtin, opts.get("compilationDatabaseChanges"))

            if method == "workspace/didChangeConfiguration":
                settings = params.get("settings")
                if isinstance(settings, dict):
                    patch_db_changes(self.builtin, settings.get("compilationDatabaseChanges"))

            if method == "textDocument/didOpen":
                td = params["textDocument"]
                d = Doc(td["uri"], td["text"], td.get("version", 0),
                        td.get("languageId", "cpp"))
                self.docs[d.uri] = d
                self.set_context(d, self.compute_context(d))
                if d.ctx is not None:
                    msg = dict(msg, params=dict(params, textDocument=dict(
                        td, uri=d.server_uri, text=d.ctx.wrap(d.text),
                        version=self.server_version(d))))
                self.to_server(msg)
                return

            if method == "textDocument/didChange":
                uri = params["textDocument"]["uri"]
                d = self.docs.get(uri)
                if d is not None:
                    for c in params["contentChanges"]:
                        d.text = apply_change(d.text, c)
                    d.version = params["textDocument"].get("version", d.version)
                if d is not None and d.ctx is not None:
                    changes = []
                    for c in params["contentChanges"]:
                        if "range" in c:
                            changes.append(dict(c, range={
                                "start": self.pos_to_server(c["range"]["start"], d),
                                "end": self.pos_to_server(c["range"]["end"], d)}))
                        else:
                            changes = [{"text": d.ctx.wrap(d.text)}]
                    msg = dict(msg, params={
                        "textDocument": {"uri": d.server_uri,
                                         "version": self.server_version(d)},
                        "contentChanges": changes})
                self.to_server(msg)
                return

            if method == "textDocument/didClose":
                d = self.docs.pop(params["textDocument"]["uri"], None)
                if d is not None and d.ctx is not None:
                    self.virtual.pop(d.server_uri, None)
                    msg = dict(msg, params={"textDocument": {"uri": d.server_uri}})
                self.to_server(msg)
                return

            if method == "textDocument/didSave":
                uri = params["textDocument"]["uri"]
                if self.wrapped(uri) is not None:
                    msg = dict(msg, params={"textDocument": {
                        "uri": self.wrapped(uri).server_uri}})
                self.to_server(msg)
                self.refresh_dependents({uri_to_path(uri)})
                return

            if method == "workspace/didChangeWatchedFiles":
                self.to_server(msg)
                self.refresh_dependents(
                    {uri_to_path(c["uri"]) for c in params.get("changes", [])})
                return

            if "id" in msg and method is not None:
                uri = self.uri_of(params, None) if isinstance(params, dict) else None
                d = self.wrapped(uri) if uri else None
                if d is not None and method == "textDocument/semanticTokens/full/delta":
                    # a full result is a valid answer to a delta request
                    method = "textDocument/semanticTokens/full"
                    params = {"textDocument": params["textDocument"]}
                    msg = dict(msg, method=method)
                if d is not None:
                    self.requests[msg["id"]] = (uri, method)
                if "params" in msg:
                    msg = dict(msg, params=self.map_in(msg["params"]))
                self.to_server(msg)
                return

            if method is not None:  # other notifications
                self.to_server(dict(msg, params=self.map_in(params))
                               if "params" in msg else msg)
                return

            # responses to server->client requests
            self.to_server(msg)

    def refresh_dependents(self, paths):
        """Re-wrap open snippets whose context comes from one of 'paths'."""
        for d in list(self.docs.values()):
            if d.ctx is None or not (d.ctx.sources & paths):
                continue
            ctx = self.compute_context(d)
            if ctx is not None and ctx.key() == d.ctx.key():
                continue
            old_uri = d.server_uri
            self.set_context(d, ctx)
            d.bump = min(d.bump + 1, VERSION_FACTOR - 1)
            text = ctx.wrap(d.text) if ctx is not None else d.text
            if d.server_uri == old_uri:
                self.to_server({
                    "jsonrpc": "2.0", "method": "textDocument/didChange",
                    "params": {"textDocument": {"uri": d.server_uri,
                                                "version": self.server_version(d)},
                               "contentChanges": [{"text": text}]}})
            else:  # no longer a snippet with context: back to the real name
                self.to_server({
                    "jsonrpc": "2.0", "method": "textDocument/didClose",
                    "params": {"textDocument": {"uri": old_uri}}})
                self.to_server({
                    "jsonrpc": "2.0", "method": "textDocument/didOpen",
                    "params": {"textDocument": {
                        "uri": d.server_uri, "languageId": d.language,
                        "version": self.server_version(d), "text": text}}})

    # --- server -> client ------------------------------------------------

    def handle_server(self, msg):
        with self.lock:
            if "method" in msg:
                if "params" in msg:
                    # notifications are about their own document; server
                    # requests (workspace/applyEdit) about open buffers
                    subject = self.uri_of(msg["params"], None) \
                        if "id" not in msg else self.ALL
                    p = self.map_out(msg["params"], subject)
                    if p is DROP:
                        return
                    msg = dict(msg, params=p)
                self.to_client(msg)
                return
            # response to a client request
            uri, method = self.requests.pop(msg.get("id"), (None, None))
            d = self.wrapped(uri) if uri else None
            if "result" in msg and msg["result"] is not None:
                if d is not None and method \
                        and method.startswith("textDocument/semanticTokens"):
                    result = self.semantic_tokens_to_client(msg["result"], d)
                else:
                    # results without a uri of their own (hover, ...) are
                    # about the request's document
                    context = d.server_uri if d is not None else uri
                    result = self.map_out(msg["result"], self.ALL, context)
                    if result is DROP:
                        result = None
                msg = dict(msg, result=result)
            self.to_client(msg)

    # --- main loops --------------------------------------------------------

    def client_loop(self):
        stdin = sys.stdin.buffer
        while True:
            msg = read_message(stdin)
            if msg is None:
                break
            t = time.time()
            try:
                self.handle_client(msg)
            except Exception as e:
                log("error handling client message:", repr(e))
                self.to_server(msg)
            if time.time() - t > 1:
                log("slow: %.1fs for client message %s" % (
                    time.time() - t, msg.get("method") or "(response)"))
        try:
            self.server.stdin.close()
        except OSError:
            pass

    def server_loop(self):
        while True:
            body = read_raw_message(self.server.stdout)
            if body is None:
                break
            # fast path: messages that cannot concern a wrapped snippet are
            # passed on untouched
            with self.lock:
                relevant = bool(self.requests) or (
                    self.virtual and VIRTUAL_TAG.encode() in body)
            if not relevant:
                write_raw_message(sys.stdout.buffer, self.client_out_lock, body)
                continue
            t = time.time()
            msg = json.loads(body.decode("utf-8"))
            try:
                self.handle_server(msg)
            except Exception as e:
                log("error handling server message:", repr(e))
                self.to_client(msg)
            if time.time() - t > 1:
                log("slow: %.1fs for server message %s (%d bytes)" % (
                    time.time() - t, msg.get("method") or "(response)", len(body)))


def main():
    clangd, args = find_real_clangd(sys.argv[1:])
    if clangd is None:
        log("no clangd found; put its path into", CONFIG_FILE,
            "or set INSIGHT_REAL_CLANGD")
        return 1

    # non-LSP invocations (--version, --help, ...) go straight through
    if any(a in ("--version", "-version", "--help", "-help", "--help-hidden")
           for a in args):
        os.execv(clangd, [clangd] + args)

    global LOG_FILE
    try:
        os.makedirs(CACHE_DIR, exist_ok=True)
        LOG_FILE = open(os.path.join(CACHE_DIR, "proxy.log"), "a", encoding="utf-8")
        # 'kill -USR1 <pid>' writes the stacks of all threads to the log
        faulthandler.register(signal.SIGUSR1, file=LOG_FILE, all_threads=True)
    except OSError:
        pass

    builtin = find_builtin_include(clangd)
    if builtin is None:
        log("no clang builtin include directory found next to", clangd)
    if any(a.startswith("--check") for a in args):
        os.execv(clangd, [clangd] + redirect_compile_db(builtin, args))

    log("using", clangd, "with arguments", args)
    args = redirect_compile_db(builtin, args)
    server = subprocess.Popen([clangd] + args, stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE)
    proxy = Proxy(server, builtin)
    t = threading.Thread(target=proxy.server_loop, daemon=True)
    t.start()
    proxy.client_loop()
    try:
        return server.wait(timeout=10)
    except subprocess.TimeoutExpired:
        server.kill()
        return 1


if __name__ == "__main__":
    sys.exit(main())
