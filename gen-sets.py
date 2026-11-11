#!/usr/bin/env python3

#awk 'sub(/^PARAMETERSET>>> /,""){f=1} /<<<PARAMETERSET/{f=0} f' $*
import sys, re, os, shutil, glob

# Supported syntax variants (all backward compatible):
#
#   PARAMETERSET>>>                          struct name defaults to "Parameters",
#                                            enclosing class inferred from context
#
#   PARAMETERSET>>> StructName               struct name given explicitly,
#                                            enclosing class inferred from context
#
#   PARAMETERSET>>> ClassName StructName     fully explicit (original syntax)
#
re_start   = re.compile(r"^PARAMETERSET>>>(?: +(\w+))?(?: +(\w+))? *$")
re_end     = re.compile(r"^<<<PARAMETERSET")
re_include = re.compile(r'^ *#include *"(.*)" *$')

# Matches the opening of a class or struct definition (not a forward declaration,
# not a friend declaration, not a template parameter).
re_class = re.compile(r'^\s*(?:class|struct)\s+(\w+)')


def find_enclosing_class(lines, filename):
    """Scan C++ lines in reverse to find the innermost enclosing class/struct name."""
    for line in reversed(lines):
        s = line.strip()
        if not s:
            continue
        # Skip friend declarations
        if re.search(r'\bfriend\b', s):
            continue
        # Skip forward declarations and single-line definitions (end with ;)
        if s.endswith(';'):
            continue
        # Skip template parameter lists
        if re.search(r'\btemplate\b', s):
            continue
        m = re_class.match(s)
        if m:
            return m.group(1)
    raise Exception(
        filename + ": PARAMETERSET>>> without explicit class name, "
        "but no enclosing class/struct found")


# ---------------------------------------------------------------------------
# Scope analysis for out-of-line code generation
#
# The PDL-generated struct is included into the user code by an #include of
# the generated header ("<stem>.h" or the computed-include macro). To define
# its member functions outside the class, its fully qualified scope is needed.
# A lightweight scanner tracks namespaces and class definitions (brace
# matching on comment- and string-stripped code) up to that #include line.
# ---------------------------------------------------------------------------

re_token = re.compile(r'[A-Za-z_]\w*|::|[{}();<>,:=]|\S')


class ScopeScanner:
    """Tracks the stack of namespace/class scopes line by line."""

    def __init__(self):
        self.stack   = []     # entries: (kind, name), kind in namespace/class/template/other
        self.pending = []     # tokens since last ; { }
        self.in_comment = False
        self.in_pp_continuation = False

    def _strip(self, line):
        """remove comments and string/char literals; keeps block comment state"""
        out = []
        i = 0
        n = len(line)
        while i < n:
            if self.in_comment:
                j = line.find('*/', i)
                if j < 0:
                    return ''.join(out)
                self.in_comment = False
                i = j + 2
                continue
            c = line[i]
            if line.startswith('//', i):
                break
            if line.startswith('/*', i):
                self.in_comment = True
                i += 2
                continue
            if c == '"' or c == "'":
                # raw strings are not expected in headers with PDL
                j = i + 1
                while j < n and line[j] != c:
                    if line[j] == '\\':
                        j += 1
                    j += 1
                out.append(' ')
                i = j + 1
                continue
            out.append(c)
            i += 1
        return ''.join(out)

    def is_preprocessor(self, line):
        """True for preprocessor lines (incl. continuation), which are not scanned."""
        if self.in_pp_continuation:
            self.in_pp_continuation = line.rstrip().endswith('\\')
            return True
        if not self.in_comment and line.lstrip().startswith('#'):
            self.in_pp_continuation = line.rstrip().endswith('\\')
            return True
        return False

    def _classify(self, toks):
        # namespace
        if 'namespace' in toks:
            i = toks.index('namespace')
            names = [t for t in toks[i+1:] if re.match(r'[A-Za-z_]\w*$', t)]
            if not names or (len(toks) > i+1 and toks[i+1] == '{'):
                return [('other', None)]   # anonymous namespace
            return [('namespace', nm) for nm in names]
        # class/struct head at angle bracket depth 0
        depth = 0
        for i, t in enumerate(toks):
            if t == '<':
                depth += 1
            elif t == '>':
                depth -= 1
            elif t == '(' and depth == 0:
                return [('other', None)]   # function, lambda, initializer
            elif t in ('class', 'struct', 'union') and depth == 0:
                if i > 0 and toks[i-1] == 'enum':
                    return [('other', None)]
                name = None
                for u in toks[i+1:]:
                    if u in (':', '{', 'final'):
                        break
                    if re.match(r'[A-Za-z_]\w*$', u):
                        name = u
                if name is None:
                    return [('other', None)]   # unnamed struct
                is_template = 'template' in toks[:i]
                return [('template' if is_template else 'class', name)]
        return [('other', None)]

    def feed(self, line):
        code = self._strip(line)
        for t in re_token.findall(code):
            if t == '{':
                entries = self._classify(self.pending)
                self.stack.append(entries)
                self.pending = []
            elif t == '}':
                if self.stack:
                    self.stack.pop()
                self.pending = []
            elif t == ';':
                self.pending = []
            else:
                self.pending.append(t)

    def qualified_scope(self):
        """fully qualified scope (e.g. "insight::MyClass::") or None,
        if the definitions cannot be placed outside (templates, function scopes, anonymous namespaces)"""
        scope = ''
        for entries in self.stack:
            for kind, name in entries:
                if kind not in ('namespace', 'class'):
                    return None
                scope += name + '::'
        return scope


def copy(l, out, expanded):
    m = re_include.match(l)
    if m is not None:
        fn = m.group(1)
        if fn in expanded:
            raise Exception("Circular dependency detected: File " + fn + " has already been expanded!")
        expanded.add(fn)
        with open(fn, 'r') as sf:
            for sl in sf:
                copy(sl, out, expanded)
        expanded.remove(fn)
    else:
        out.write(l)


# options (anywhere on the command line):
#   --pdl-cpp=<file>  generate the member functions of the parameter structs
#                     out of line and write a source file <file>, which
#                     contains their definitions. Without this option,
#                     all code is generated inline into the headers.
pdl_cpp = None
args = []
for a in sys.argv[1:]:
    if a.startswith('--pdl-cpp='):
        pdl_cpp = a[len('--pdl-cpp='):]
    else:
        args.append(a)

filename = args[0]
pdlexe   = args[1]
addlocs  = args[2:]
basename = os.path.splitext(os.path.basename(filename))[0]
upper_base = re.sub(r'[^A-Za-z0-9]', '_', basename).upper()

print("Scanning", filename, "for PDL definitions")
generated    = []
expanded_glob = set()

# Read manifest from previous run to detect stale files
manifest_file = basename + "_pdl.manifest"
try:
    with open(manifest_file) as mf:
        previous_stems = set(line for line in mf.read().splitlines() if line)
except FileNotFoundError:
    previous_stems = set()

lines_seen = []   # C++ lines outside PDL blocks, used for class inference
inside = False
out    = None

scanner       = ScopeScanner()
include_scope = {}   # include token -> qualified scope (or None) at its #include
re_pp_include = re.compile(r'^\s*#\s*include\s*(?:"([^"]*)"|<([^>]*)>|(\w+))')

with open(filename, "r") as f:
    for l in f:
        if not inside:
            if scanner.is_preprocessor(l):
                m = re_pp_include.match(l)
                if m is not None:
                    tok = m.group(1) or m.group(2) or m.group(3)
                    include_scope.setdefault(tok, scanner.qualified_scope())
            elif re_start.search(l) is None:
                scanner.feed(l)
        if inside:
            if re_end.match(l):
                if out is not None:
                    out.close()
                    out = None
                inside = False
            else:
                copy(l, out, expanded_glob)
        else:
            m = re_start.search(l)
            if m is not None:
                tok1, tok2 = m.group(1), m.group(2)
                if tok2 is not None:
                    # Original two-token syntax: class and struct both explicit
                    prefix     = tok1
                    structname = tok2
                elif tok1 is not None:
                    # One-token syntax: struct name given, class inferred
                    structname = tok1
                    prefix     = find_enclosing_class(lines_seen, filename)
                else:
                    # No-token syntax: struct defaults to "Parameters", class inferred
                    structname = "Parameters"
                    prefix     = find_enclosing_class(lines_seen, filename)
                outfname = basename + "__" + prefix + "__" + structname
                print("Generating", outfname)
                generated.append(outfname)
                out    = open(outfname + ".pdl", "w")
                inside = True
            lines_seen.append(l)


def out_of_line_scope(stem):
    """the qualified scope, if the definitions can be generated out of line"""
    if pdl_cpp is None:
        return None
    parts = stem.split("__")
    candidates = [stem + ".h"]
    if len(parts) == 3:
        candidates.append(upper_base + "_PDL_" + parts[1])
    found = [c for c in candidates if c in include_scope]
    if not found:
        print("Generated header of", stem, "is not included in", filename, ": generating inline code")
        return None
    scope = include_scope[found[0]]
    if scope is None:
        print("Scope of", stem, "is a template or local: generating inline code")
    return scope


out_of_line = []
for stem in generated:
    scope = out_of_line_scope(stem)
    opts = ''
    if scope is not None:
        opts = '"--out-of-line-scope=%s" ' % scope
        out_of_line.append(stem)
    if os.system('"%s" %s"%s"' % (pdlexe, opts, stem + ".pdl")) != 0:
        sys.exit(-1)
    for file in glob.glob(stem + "__*.h"):
        for al in addlocs:
            shutil.copy(file, al)


# Generate the source file with the out-of-line definitions
if pdl_cpp is not None:
    content = "// Generated by gen-sets.py from %s -- do not edit\n" % filename
    if out_of_line:
        content += '#include "%s"\n' % os.path.abspath(filename)
        for stem in out_of_line:
            content += '#include "%s_impl.h"\n' % stem
    # write only on changes to avoid needless recompilation
    try:
        with open(pdl_cpp) as cf:
            old = cf.read()
    except FileNotFoundError:
        old = None
    if old != content:
        with open(pdl_cpp, "w") as cf:
            cf.write(content)


# Generate basename_pdl.h:
#   - #include lines for all *_headers.h prerequisites (replaces per-class file-scope includes)
#   - #define macros for computed class-body includes (e.g. #include MYFILE_PDL_MyClass)
pdl_hdr_name = basename + "_pdl.h"
with open(pdl_hdr_name, "w") as ph:
    ph.write("// Generated by gen-sets.py — do not edit\n")
    ph.write("#pragma once\n")
    if generated:
        ph.write("\n// Prerequisite includes for all parameter sets defined in %s\n" % (basename + ".h"))
        for stem in generated:
            ph.write('#include "%s_headers.h"\n' % stem)
        ph.write("\n// Computed-include macros for class-body use, e.g.:\n")
        ph.write("//   #include %s_PDL_ClassName\n" % upper_base)
        for stem in generated:
            parts = stem.split("__")
            if len(parts) == 3:
                macro = upper_base + "_PDL_" + parts[1]
                ph.write('#define %-48s "%s.h"\n' % (macro, stem))
for al in addlocs:
    shutil.copy(pdl_hdr_name, al)


# Remove stale files from previous runs whose stems are no longer generated
current_stems = set(generated)
for stale_stem in previous_stems - current_stems:
    stale_pdl = stale_stem + ".pdl"
    if os.path.exists(stale_pdl):
        print("Removing stale PDL:", stale_pdl)
        os.remove(stale_pdl)
    for stale_h in glob.glob(stale_stem + "__*.h"):
        print("Removing stale header:", stale_h)
        os.remove(stale_h)
        for al in addlocs:
            mirror = os.path.join(al, os.path.basename(stale_h))
            if os.path.exists(mirror):
                print("Removing stale header from addloc:", mirror)
                os.remove(mirror)

# Write updated manifest
with open(manifest_file, "w") as mf:
    mf.write("\n".join(generated))
