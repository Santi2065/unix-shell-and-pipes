"""Regenerates the diagrams shown in the README.

    pip install numpy matplotlib
    python docs/figures/make_figures.py

Both figures draw the process and pipe topology exactly as src/ej1/ring.c and src/ej2/shell.c
build it; the outcome counts in Table 1 come from the loop in "Reproducing the results".
"""
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager
from matplotlib.patches import Circle, FancyBboxPatch

HERE = Path(__file__).resolve().parent
for f in Path("/usr/share/fonts/lm").glob("lm*10-*.otf"):  # Latin Modern, if installed
    font_manager.fontManager.addfont(str(f))
plt.style.use(HERE / "paper.mplstyle")
C = plt.rcParams["axes.prop_cycle"].by_key()["color"]
INK, GRAY = "#1a1a1a", "#8c8c8c"
BLUE, LGRAY, RED = "#dce6f2", "#ececec", "#f9e3df"


def save(fig, name):
    fig.savefig(HERE / name, metadata={"Date": None})
    plt.close(fig)


def canvas(w, h, xmax, ymax, x0=0, y0=0):
    fig, ax = plt.subplots(figsize=(w, h))
    ax.set_xlim(x0, xmax)
    ax.set_ylim(y0, ymax)
    ax.set_aspect("equal")
    ax.axis("off")
    return fig, ax


def box(ax, x, y, w, h, text="", fc="white", ec=INK, fs=7.5, ls="-", lw=0.6, **kw):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0,rounding_size=0.06",
                                fc=fc, ec=ec, lw=lw, ls=ls))
    if text:
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs, **kw)


def arrow(ax, p, q, color=INK, ls="-", lw=0.7, rad=0.0, ms=7):
    ax.annotate("", xy=q, xytext=p, arrowprops=dict(arrowstyle="-|>", lw=lw, color=color, ls=ls, shrinkA=0,
                                                    shrinkB=0, mutation_scale=ms,
                                                    connectionstyle=f"arc3,rad={rad}"))


# ---- Figure 2: the ring built by ring.c for n = 5, s = 3
n, s = 5, 3
R, r = 2.6, 0.42
ang = lambda i: np.pi / 2 - 2 * np.pi * i / n          # child index i, clockwise from the top
P = {i: np.array([R * np.cos(ang(i)), R * np.sin(ang(i))]) for i in range(n)}
fig, ax = canvas(7.2, 3.5, 7.6, 3.5, x0=-3.4, y0=-3.0)
inject, collect = s - 1, (s - 2) % n                     # pipes[s-1] written by the parent, pipes[s-2] read by it
for i in range(n):                                        # pipes[i]: child i -> child i+1
    a, b = P[i], P[(i + 1) % n]
    d = (b - a) / np.linalg.norm(b - a)
    hot = i == collect
    arrow(ax, a + d * (r + 0.05), b - d * (r + 0.05), color=C[1] if hot else C[0], lw=1.1 if hot else 0.8, rad=-0.18)
    mid = (a + b) / 2 * 1.13
    ax.text(*mid, f"pipes[{i}]", ha="center", va="center", fontsize=7.2, color=C[1] if hot else C[0])
for i in range(n):
    start = i == s - 1
    ax.add_patch(Circle(P[i], r, fc=RED if start else BLUE, ec=C[1] if start else INK, lw=0.8))
    ax.text(*P[i], f"P{i + 1}", ha="center", va="center", fontsize=8.5)
box(ax, -0.62, -0.32, 1.24, 0.64, "parent", fc=LGRAY, fs=8.5)
mid_pipe = lambda i: (P[i] + P[(i + 1) % n]) / 2 * 0.86
arrow(ax, (0.0, -0.32), mid_pipe(inject) + np.array([0, 0.06]), color=INK)
ax.text(0.1, -1.15, "write $c$", fontsize=7.2)
arrow(ax, mid_pipe(collect) - np.array([0.08, 0]), (0.62, -0.05), color=C[1])
ax.text(0.8, -0.72, "read result", fontsize=7.2, color=C[1])
outcomes = [("Parent reads first", ["prints $c + n - 1$ and exits 0;", f"P{s} reads end-of-file"]),
            (f"P{s} reads first", [f"P{s} writes into pipes[{inject}], which has", "no reader left (SIGPIPE);",
                                   "the parent reads end-of-file and", "exits 1 (\"read padre\")"])]
ax.text(3.6, 3.05, f"$n = {n}$, $s = {s}$: the parent and P{s}\nboth read pipes[{collect}]", fontsize=7.8,
        color=C[1], va="center")
for k, (title, lines) in enumerate(outcomes):
    top = 2.25 - k * 2.25
    h = 0.5 + 0.36 * len(lines) + 0.1
    box(ax, 3.6, top - h, 3.9, h, fc="white", ec=GRAY)
    ax.text(3.75, top - 0.27, title, fontsize=7.8, weight="bold", va="center")
    for j, ln in enumerate(lines):
        ax.text(3.75, top - 0.65 - 0.36 * j, ln, fontsize=7.2, va="center")
fig.tight_layout(pad=0.2)
save(fig, "fig2-ring.svg")

# ---- Figure 1: how shell.c runs `ls | grep .zip | wc -l`
fig, ax = canvas(7.2, 3.3, 14.4, 6.6)
stages = ["input line", "split_pipeline\n(on '|' outside quotes)", "trim + parse_command\n(argv, quotes kept together)",
          "fork + dup2 + execvp"]
for k, st in enumerate(stages):
    box(ax, 0.2 + k * 3.6, 5.6, 3.1, 0.85, st, fs=7.3, fc="#f7f7f7")
    if k:
        arrow(ax, (k * 3.6 - 0.1, 6.025), (0.2 + k * 3.6, 6.025))
box(ax, 4.4, 4.2, 5.6, 0.8, "shell (parent): pipe() × 2, fork() × 3, wait() × 3", fs=7.3, fc=LGRAY)
cmds = ["ls", "grep .zip", "wc -l"]
xs = [0.6, 5.6, 10.6]
for k, (x, cmd) in enumerate(zip(xs, cmds)):
    box(ax, x, 1.0, 3.2, 1.85, fc=BLUE)
    ax.text(x + 1.6, 2.55, f"child {k}", fontsize=7.5, ha="center", style="italic")
    ax.text(x + 1.6, 2.1, f"execvp(\"{cmd.split()[0]}\", …)", fontsize=7, ha="center", family="monospace")
    sin = "terminal" if k == 0 else f"pipes[{k - 1}][0]"
    sout = "terminal" if k == 2 else f"pipes[{k}][1]"
    ax.text(x + 1.6, 1.65, f"stdin  = {sin}", fontsize=6.6, ha="center", family="monospace")
    ax.text(x + 1.6, 1.3, f"stdout = {sout}", fontsize=6.6, ha="center", family="monospace")
    arrow(ax, (7.2, 4.2), (x + 1.6, 2.85), color=GRAY, lw=0.6)
for k in range(2):
    x = xs[k] + 3.2
    box(ax, x + 0.25, 1.65, 1.3, 0.55, f"pipes[{k}]", fs=7, fc="white", ec=C[0])
    arrow(ax, (x, 1.92), (x + 0.25, 1.92), color=C[0])
    arrow(ax, (x + 1.55, 1.92), (xs[k + 1], 1.92), color=C[0])
ax.text(0.2, 0.4, "each child closes every pipe descriptor after dup2; the parent closes pipes[k-1] after forking "
        "child k and waits for all children", fontsize=6.8, color="#4d4d4d")
fig.tight_layout(pad=0.2)
save(fig, "fig1-pipeline.svg")
