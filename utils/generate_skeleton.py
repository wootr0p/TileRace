#!/usr/bin/env python3
"""
generate_skeleton.py
--------------------
Generates Skeleton.h — a single, AI-friendly file that merges every .h header
in the project's src/ directory.

What is included
  • Project metadata and full src/ directory tree
  • Full content of every .h file, prefixed with a locator comment
  • Inline function/method BODIES are stripped (only the signature is kept)
    so the AI gets the interface without the noise of implementation details.
  • All comments inside headers are preserved.

What is excluded
  • .cpp implementation files
  • Build artefacts (build/)
  • Third-party headers inside _deps/

Run from the workspace root:
    python utils/generate_skeleton.py
"""

import os
import re
import datetime
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
WORKSPACE = Path(__file__).parent.parent   # workspace root (script lives in utils/)
SRC_ROOT  = WORKSPACE / "src"
OUTPUT    = Path(__file__).parent / "Skeleton.h"  # generated next to the script in utils/

# Subdirectory order for predictable output (common → server → client)
SUBDIR_ORDER = ["common", "server", "client"]

# ---------------------------------------------------------------------------
# Step 1: collect .h files in desired order
# ---------------------------------------------------------------------------
def collect_headers(src_root: Path) -> list[Path]:
    seen    : set[Path] = set()
    ordered : list[Path] = []

    def _walk(subdir: Path):
        if not subdir.exists():
            return
        for path in sorted(subdir.rglob("*.h")):
            if path not in seen:
                seen.add(path)
                ordered.append(path)

    # Preferred order first
    for name in SUBDIR_ORDER:
        _walk(src_root / name)

    # Any remaining subdirectory not listed above
    for sub in sorted(src_root.iterdir()):
        if sub.is_dir() and sub.name not in SUBDIR_ORDER:
            _walk(sub)

    return ordered


# ---------------------------------------------------------------------------
# Step 2: build src/ directory tree string
# ---------------------------------------------------------------------------
def build_tree(root: Path, prefix: str = "") -> list[str]:
    lines  : list[str] = []
    entries = sorted(root.iterdir(), key=lambda p: (p.is_file(), p.name.lower()))
    for i, entry in enumerate(entries):
        connector = "└── " if i == len(entries) - 1 else "├── "
        lines.append(f"{prefix}{connector}{entry.name}")
        if entry.is_dir():
            extension = "    " if i == len(entries) - 1 else "│   "
            lines.extend(build_tree(entry, prefix + extension))
    return lines


# ---------------------------------------------------------------------------
# Step 3: strip inline function bodies while preserving comments
#
# Strategy (line-by-line + brace tracking):
#   • Track brace depth.
#   • When depth == 0 and we see an opening '{' on a line that is NOT a
#     type/namespace/template declaration → we are entering an inline body.
#     Emit the signature line up to (and including) '{', then '/* body stripped */'
#     and '}', then skip lines until the matching '}' is consumed.
#   • Lines inside struct/class/enum/namespace bodies are always kept.
#   • Comment-only lines are always kept regardless of depth.
# ---------------------------------------------------------------------------
def strip_bodies(code: str) -> str:
    lines  = code.splitlines(keepends=True)
    result : list[str] = []

    # Each entry in the stack is either KEEP or INLINE, tracking what kind of
    # brace block we are currently inside.
    #   KEEP   → struct / class / enum / union / namespace / template body:
    #            lines inside are output normally.
    #   INLINE → inline function / method body:
    #            lines inside are suppressed (except standalone comments).
    INLINE = "inline"
    KEEP   = "keep"
    stack  : list[str] = []

    # Regex for declarations that introduce a KEEP block (their body is part of
    # the type definition, not an inline implementation we want to strip).
    keep_re = re.compile(
        r"^\s*(struct|class|enum\b|union|namespace|extern\s+\"C\"|template)\b",
        re.MULTILINE
    )

    i = 0
    while i < len(lines):
        line = lines[i]
        text = line.rstrip("\n").rstrip("\r")

        in_stripped = bool(stack) and stack[-1] == INLINE

        stripped_text = text.strip()
        is_comment_only = (
            stripped_text.startswith("//") or
            stripped_text.startswith("*")  or
            stripped_text.startswith("/*")
        )

        # Emit the line unless we are inside a stripped body
        if not in_stripped:
            result.append(line)
        elif is_comment_only:
            # Always preserve comments even inside stripped bodies
            result.append(line)

        # Count net brace change on this line (ignoring strings/comments)
        clean = _remove_strings_and_comments(text)
        opens  = clean.count("{")
        closes = clean.count("}")
        net    = opens - closes

        if net > 0:
            # Entering net new blocks on this line.
            # Determine kind: KEEP if this line starts a type/namespace declaration,
            # OR if we are already inside a KEEP block (nested type member bodies).
            in_keep_context = bool(stack) and stack[-1] == KEEP
            is_keeper_line  = bool(keep_re.match(text)) or in_keep_context
            kind = KEEP if is_keeper_line else INLINE

            for _ in range(net):
                stack.append(kind)

            # If we just entered an INLINE body (and we just emitted the line),
            # patch the last emitted line: keep the signature up to '{',
            # append the stripped marker, close immediately.
            if kind == INLINE and not in_stripped:
                last = result[-1] if result else ""
                open_idx = last.rfind("{")
                if open_idx != -1:
                    result[-1] = last[:open_idx + 1] + " /* body stripped */ }\n"

        elif net < 0:
            for _ in range(-net):
                if stack:
                    stack.pop()

        i += 1

    return "".join(result)


def _remove_strings_and_comments(line: str) -> str:
    """Remove string literals and line comments for brace counting."""
    result = []
    j      = 0
    n      = len(line)
    while j < n:
        c = line[j]
        # Line comment
        if c == "/" and j + 1 < n and line[j + 1] == "/":
            break
        # Block comment (single line only — multiline not handled here)
        if c == "/" and j + 1 < n and line[j + 1] == "*":
            end = line.find("*/", j + 2)
            j = end + 2 if end != -1 else n
            continue
        # String literal
        if c in ('"', "'"):
            quote = c
            j += 1
            while j < n:
                if line[j] == "\\" :
                    j += 2
                    continue
                if line[j] == quote:
                    j += 1
                    break
                j += 1
            continue
        result.append(c)
        j += 1
    return "".join(result)


# ---------------------------------------------------------------------------
# Step 4: write Skeleton.h
# ---------------------------------------------------------------------------
def main():
    headers = collect_headers(SRC_ROOT)
    now     = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")

    tree_lines = build_tree(SRC_ROOT)
    tree_str   = "\n".join(f" *   {l}" for l in tree_lines)

    header_list = "\n".join(
        f" *   [{idx+1:02d}]  {h.relative_to(WORKSPACE)}"
        for idx, h in enumerate(headers)
    )

    banner = f"""\
/*
 * ============================================================================
 * SKELETON.H  —  AI Context Snapshot for TileRace
 * Generated : {now}
 * ============================================================================
 *
 * PURPOSE
 *   This file is a machine-generated, read-only summary of every public header
 *   in the TileRace project.  It is designed to give an AI assistant immediate,
 *   full-project context without requiring access to the individual source files.
 *
 * HOW TO USE THIS FILE
 *   This file covers the complete public interface of the codebase.
 *   Inline function bodies have been stripped; class / struct / enum bodies are kept.
 *   If you need deeper context — game mechanics, flow, physics constants, network
 *   protocol semantics, implementation status — read PROJECT_CONTEXT.md located in
 *   the utils/ folder (same directory as this file).  That document is the
 *   authoritative human+AI reference for the whole project.
 *
 * IMPORTANT — AI INSTRUCTIONS
 *   After EVERY code change (new feature, refactor, bug fix, constant tweak,
 *   protocol change, or any modification to architecture / mechanics / status),
 *   you MUST do BOTH of the following — always, without exception:
 *     1. Update utils/PROJECT_CONTEXT.md to reflect the current state of the project.
 *     2. Run utils/generate_skeleton.py to regenerate this file:
 *           python utils/generate_skeleton.py
 *   Keep PROJECT_CONTEXT.md and Skeleton.h accurate and in sync with the codebase
 *   at all times.
 *
 * CONTENTS
 *   One section per .h file, in dependency order (common → server → client).
 *   Inline function / method BODIES have been stripped (replaced with
 *   "body stripped" markers) so only the interface is shown.
 *   All comments present in the original headers are preserved.
 *
 * PROJECT OVERVIEW
 *   TileRace — real-time multiplayer 2-D platformer
 *   Language  : C++20
 *   Graphics  : Raylib
 *   Networking: ENet (UDP, authoritative server)
 *   Build     : CMake + Ninja  (presets: run-scc-debug / run-scc-release)
 *   Layout:
 *     src/common/  — pure logic (no Raylib, no ENet): World, Player, Physics, Protocol
 *     src/server/  — authoritative server: ServerSession, LevelManager, ServerLogic
 *     src/client/  — rendering + input + game loop: Renderer, GameSession, MainMenu, …
 *
 * ARCHITECTURE NOTES
 *   • Client-side prediction with server reconciliation (InputFrame + tick seq.)
 *   • Fixed timestep 60 Hz (FIXED_DT = 1/60 s) on both client and server
 *   • Lobby map (_lobby.txt) → levels level_01..N → results screen loop
 *   • ENet wrapped behind NetworkClient (client) / ServerSession (server)
 *   • SOLID refactoring applied: SRP, DIP, DRY throughout
 *
 * src/ DIRECTORY TREE
{tree_str}
 *
 * HEADERS INCLUDED BELOW  ({len(headers)} files)
{header_list}
 * ============================================================================
 */

"""

    sections : list[str] = []
    for hdr in headers:
        rel  = hdr.relative_to(WORKSPACE).as_posix()
        name = hdr.name
        code = hdr.read_text(encoding="utf-8", errors="replace")
        stripped = strip_bodies(code)

        section = (
            f"\n"
            f"// {'=' * 74}\n"
            f"// FILE : {name}\n"
            f"// PATH : {rel}\n"
            f"// {'=' * 74}\n"
            f"\n"
            f"{stripped}\n"
        )
        sections.append(section)

    output_text = banner + "".join(sections)
    OUTPUT.write_text(output_text, encoding="utf-8")
    print(f"[generate_skeleton] Written {len(headers)} headers → {OUTPUT}")
    print(f"[generate_skeleton] Output size: {len(output_text):,} chars  "
          f"({OUTPUT.stat().st_size:,} bytes)")


if __name__ == "__main__":
    main()
