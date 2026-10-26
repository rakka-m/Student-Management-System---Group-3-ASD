"""Generate the diagrams used by the DSA documentation.

Run from this folder:
    python make_diagrams.py
"""

from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch, Polygon
import numpy as np

OUT = Path(__file__).resolve().parent / "diagrams"
OUT.mkdir(parents=True, exist_ok=True)

NAVY = "#1F3864"
BLUE = "#2E75B6"
LIGHT_BLUE = "#DEEAF6"
GREEN = "#E2EFDA"
GREEN_DARK = "#548235"
ORANGE = "#FCE4D6"
ORANGE_DARK = "#C55A11"
GRAY = "#F2F2F2"
GRAY_DARK = "#7F7F7F"
TEXT = "#1F1F1F"
ARROW = "#44546A"


def new_figure(width, height):
    fig, ax = plt.subplots(figsize=(width, height))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 16)
    ax.axis("off")
    return fig, ax


def rounded_box(ax, x, y, w, h, text, face=LIGHT_BLUE, edge=NAVY,
                fontsize=11, weight="normal", text_color=TEXT):
    patch = FancyBboxPatch(
        (x - w / 2, y - h / 2), w, h,
        boxstyle="round,pad=0.02,rounding_size=0.12",
        linewidth=1.5, edgecolor=edge, facecolor=face, zorder=2,
    )
    ax.add_patch(patch)
    ax.text(x, y, text, ha="center", va="center", fontsize=fontsize,
            color=text_color, weight=weight, zorder=3, linespacing=1.25)
    return patch


def terminator(ax, x, y, w, h, text, face=GREEN, edge=GREEN_DARK):
    patch = FancyBboxPatch(
        (x - w / 2, y - h / 2), w, h,
        boxstyle="round,pad=0.02,rounding_size=0.45",
        linewidth=1.5, edgecolor=edge, facecolor=face, zorder=2,
    )
    ax.add_patch(patch)
    ax.text(x, y, text, ha="center", va="center", fontsize=11.5,
            weight="bold", color=TEXT, zorder=3)


def diamond(ax, x, y, w, h, text, face=ORANGE, edge=ORANGE_DARK,
            fontsize=11):
    points = [(x, y + h / 2), (x + w / 2, y), (x, y - h / 2), (x - w / 2, y)]
    ax.add_patch(Polygon(points, closed=True, facecolor=face, edgecolor=edge,
                         linewidth=1.5, zorder=2))
    ax.text(x, y, text, ha="center", va="center", fontsize=fontsize,
            color=TEXT, zorder=3, linespacing=1.2)


def arrow(ax, start, end, label=None, label_offset=(0.15, 0.12),
          color=ARROW, lw=1.5, style="-|>", mutation=16):
    ax.add_patch(FancyArrowPatch(
        start, end, arrowstyle=style, mutation_scale=mutation,
        linewidth=lw, color=color, zorder=1,
        shrinkA=0, shrinkB=0,
    ))
    if label:
        mx = (start[0] + end[0]) / 2 + label_offset[0]
        my = (start[1] + end[1]) / 2 + label_offset[1]
        ax.text(mx, my, label, ha="center", va="center", fontsize=9.5,
                color=ARROW, bbox=dict(boxstyle="round,pad=0.16",
                                       facecolor="white", edgecolor="none"))


def save(fig, name):
    path = OUT / name
    fig.savefig(path, dpi=170, bbox_inches="tight", facecolor="white")
    plt.close(fig)
    print("wrote", path)


# ---------------------------------------------------------------------------
# 1. Main system flowchart
# ---------------------------------------------------------------------------

def main_flowchart():
    fig, ax = new_figure(10.2, 13.2)
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 17)

    ax.text(6, 16.6, "Figure 1. Main Program Flowchart",
            ha="center", va="center", fontsize=15, weight="bold", color=NAVY)

    terminator(ax, 6, 15.5, 2.6, 0.85, "START")
    rounded_box(ax, 6, 13.9, 5.0, 0.95,
                "Load student records from students.csv\n(rebuild the singly linked list)")
    rounded_box(ax, 6, 12.25, 4.4, 0.85,
                "Display the Main Menu and\nread the user's choice", face=GRAY)
    diamond(ax, 6, 10.35, 4.3, 1.7, "Choice\n1 to 10?")

    rounded_box(
        ax, 3.0, 8.1, 5.0, 1.9,
        "Execute the selected operation\n\n"
        "1 Add   2 Display   3 Search   4 Update\n"
        "5 Delete   6 Sort   7 Stack   8 Queue\n"
        "9 Statistics",
        face=LIGHT_BLUE, fontsize=9.8,
    )
    rounded_box(
        ax, 9.4, 8.1, 3.1, 1.45,
        "Choice 10\nEXIT\n\nSave any changes\nto students.csv",
        face=ORANGE, edge=ORANGE_DARK, fontsize=10.5,
    )
    terminator(ax, 9.4, 6.35, 2.3, 0.8, "END")

    # Main spine
    arrow(ax, (6, 15.075), (6, 14.375))
    arrow(ax, (6, 13.425), (6, 12.675))
    arrow(ax, (6, 11.825), (6, 11.15))
    arrow(ax, (6, 9.5), (3.0, 9.05), label="1 - 9", label_offset=(-0.35, 0.18))
    arrow(ax, (6, 9.5), (9.4, 8.825), label="10", label_offset=(0.25, 0.15))
    arrow(ax, (9.4, 7.375), (9.4, 6.75))

    # Loop back to the menu
    ax.plot([0.22, 0.22], [8.1, 12.25], color=ARROW, linewidth=1.5, zorder=1)
    arrow(ax, (0.22, 12.25), (3.75, 12.25))
    ax.text(0.04, 10.2, "repeat until Exit", rotation=90, ha="center",
            va="center", fontsize=9.2, color=ARROW)

    # Notes
    rounded_box(
        ax, 3.0, 5.6, 5.0, 0.95,
        "Every add, update, delete, restore and sort\nauto-saves the linked list to students.csv",
        face=GREEN, edge=GREEN_DARK, fontsize=9.5,
    )
    ax.plot([3.0, 3.0], [7.15, 6.1], color=GREEN_DARK, linewidth=1.2,
            linestyle=":")

    rounded_box(
        ax, 9.4, 4.6, 3.6, 1.2,
        "Records stay in memory\nuntil the program exits;\nCSV provides persistence",
        face=GRAY, edge=GRAY_DARK, fontsize=9.2,
    )

    ax.text(6, 0.55,
            "The program loops through the menu until the user selects Exit.",
            ha="center", va="center", fontsize=10, style="italic",
            color=GRAY_DARK)
    save(fig, "flowchart_main.png")


# ---------------------------------------------------------------------------
# 2. Singly linked list
# ---------------------------------------------------------------------------

def linked_list():
    fig, ax = plt.subplots(figsize=(12.5, 4.6))
    ax.set_xlim(0, 14.2)
    ax.set_ylim(0, 5.4)
    ax.axis("off")

    ax.text(7.1, 5.0, "Figure 2. Singly Linked List Used as Primary Storage",
            ha="center", va="center", fontsize=14.5, weight="bold", color=NAVY)

    # head / tail labels
    ax.text(0.75, 3.45, "head", fontsize=11, weight="bold", color=NAVY,
            ha="center")
    ax.text(0.75, 1.05, "tail", fontsize=11, weight="bold", color=GREEN_DARK,
            ha="center")

    xs = [2.1, 5.0, 7.9, 10.8]
    w, h = 2.45, 1.55
    labels = [
        "202400001\nDela Cruz, Juan\nBSIT  |  2  |  1.50",
        "202400003\nReyes, Jose\nBSIT  |  3  |  2.00",
        "202400005\nVillanueva, Mark\nBSIT  |  4  |  3.00",
        "...",
    ]
    for i, (x, label) in enumerate(zip(xs, labels)):
        # data cell
        ax.add_patch(plt.Rectangle((x - w / 2, 2.15), w * 0.72, h,
                                   facecolor=LIGHT_BLUE, edgecolor=NAVY,
                                   linewidth=1.4, zorder=2))
        # next cell
        ax.add_patch(plt.Rectangle((x - w / 2 + w * 0.72, 2.15), w * 0.28,
                                   h, facecolor=GRAY, edgecolor=NAVY,
                                   linewidth=1.4, zorder=2))
        ax.text(x - w / 2 + w * 0.36, 2.925, label, ha="center", va="center",
                fontsize=8.9, color=TEXT, zorder=3, linespacing=1.3)
        ax.text(x - w / 2 + w * 0.86, 2.925, "next", ha="center", va="center",
                fontsize=8.5, color=GRAY_DARK, zorder=3, rotation=90)

        if i < len(xs) - 1:
            start = (x + w / 2, 2.925)
            end = (xs[i + 1] - w / 2, 2.925)
            arrow(ax, start, end, color=ARROW, lw=1.4, mutation=14)

    # head pointer
    arrow(ax, (1.25, 3.3), (xs[0] - w / 2, 3.05), color=NAVY, lw=1.6)
    # tail pointer, drawn from below the last node so it cannot cross the text
    ax.text(xs[3], 1.18, "tail", fontsize=11, weight="bold",
            color=GREEN_DARK, ha="center", va="center")
    arrow(ax, (xs[3], 1.45), (xs[3], 2.15), color=GREEN_DARK, lw=1.6,
          style="-|>")

    # NULL
    last_x = xs[-1] + w / 2
    ax.plot([last_x, last_x + 0.7], [2.925, 2.925], color=ARROW, linewidth=1.4)
    ax.plot([last_x + 0.7, last_x + 0.95], [2.925, 2.925], color=ARROW,
            linewidth=1.4)
    ax.plot([last_x + 0.7, last_x + 0.7], [2.66, 3.19], color=ARROW,
            linewidth=1.4)
    ax.text(last_x + 1.25, 2.925, "NULL", ha="left", va="center", fontsize=10,
            weight="bold", color=GRAY_DARK)

    ax.text(7.1, 0.62,
            "Each node owns one Student record and one pointer.  add() is O(1) "
            "with the tail pointer;\nsearch, update and delete are O(n) because "
            "the list has no random access.",
            ha="center", va="center", fontsize=9.8, color=TEXT, linespacing=1.4)

    save(fig, "linked_list.png")


# ---------------------------------------------------------------------------
# 3. Stack
# ---------------------------------------------------------------------------

def stack_diagram():
    fig, ax = plt.subplots(figsize=(8.4, 7.2))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 10)
    ax.axis("off")

    ax.text(6, 9.45, "Figure 3. Recently Deleted Stack (LIFO)",
            ha="center", va="center", fontsize=14.5, weight="bold", color=NAVY)

    x, w = 4.0, 4.2
    records = [
        ("Record deleted first", GRAY, GRAY_DARK),
        ("Record deleted second", LIGHT_BLUE, NAVY),
        ("Record deleted third (TOP)", GREEN, GREEN_DARK),
    ]
    y0 = 2.2
    h = 1.35
    for i, (label, face, edge) in enumerate(records):
        y = y0 + i * (h + 0.08)
        ax.add_patch(plt.Rectangle((x, y), w, h, facecolor=face,
                                   edgecolor=edge, linewidth=1.5, zorder=2))
        ax.text(x + w / 2, y + h / 2, label, ha="center", va="center",
                fontsize=10.5, color=TEXT, zorder=3)

    top_y = y0 + 2 * (h + 0.08) + h / 2
    ax.text(10.1, top_y, "top", fontsize=12, weight="bold", color=GREEN_DARK,
            ha="center")
    arrow(ax, (9.7, top_y), (x + w + 0.1, top_y), color=GREEN_DARK, lw=2,
          mutation=18)

    # push / pop arrows directly above the top of the stack
    arrow(ax, (5.3, 8.35), (5.3, top_y + h / 2 + 0.02), color=BLUE, lw=1.8,
          mutation=18)
    ax.text(4.85, 8.15, "push\n(on delete)", ha="right", va="center",
            fontsize=10, color=BLUE, linespacing=1.2)

    arrow(ax, (6.7, top_y + h / 2 + 0.02), (6.7, 8.35), color=ORANGE_DARK,
          lw=1.8, mutation=18)
    ax.text(7.15, 8.15, "pop\n(restore)", ha="left", va="center", fontsize=10,
            color=ORANGE_DARK, linespacing=1.2)

    ax.text(6, 1.2,
            "push and pop both run in O(1).  The top item is always the most\n"
            "recently deleted record, which makes the stack a natural Undo.",
            ha="center", va="center", fontsize=10, color=TEXT, linespacing=1.45)

    save(fig, "stack.png")


# ---------------------------------------------------------------------------
# 4. Queue
# ---------------------------------------------------------------------------

def queue_diagram():
    fig, ax = plt.subplots(figsize=(11.6, 4.9))
    ax.set_xlim(0, 13)
    ax.set_ylim(0, 6.2)
    ax.axis("off")

    ax.text(6.5, 5.75, "Figure 4. Student Service Queue (FIFO)",
            ha="center", va="center", fontsize=14.5, weight="bold", color=NAVY)

    labels = [
        "Ticket 1\nJuan\nEnrollment",
        "Ticket 2\nMaria\nTranscript",
        "Ticket 3\nJose\nPayment",
    ]
    xs = [3.0, 6.0, 9.0]
    w, h = 2.15, 1.65

    for i, (x, label) in enumerate(zip(xs, labels)):
        ax.add_patch(plt.Rectangle((x - w / 2, 2.6), w, h,
                                   facecolor=LIGHT_BLUE, edgecolor=NAVY,
                                   linewidth=1.5, zorder=2))
        ax.text(x, 3.425, label, ha="center", va="center", fontsize=10,
                color=TEXT, zorder=3, linespacing=1.35)
        if i < len(xs) - 1:
            arrow(ax, (x + w / 2, 3.425), (xs[i + 1] - w / 2, 3.425),
                  color=ARROW, lw=1.4, mutation=14)

    # front and rear pointers
    ax.text(3.0, 5.0, "front", ha="center", va="center", fontsize=12,
            weight="bold", color=GREEN_DARK)
    arrow(ax, (3.0, 4.75), (3.0, 4.35), color=GREEN_DARK, lw=2, mutation=18)

    ax.text(9.0, 5.0, "rear", ha="center", va="center", fontsize=12,
            weight="bold", color=ORANGE_DARK)
    arrow(ax, (9.0, 4.75), (9.0, 4.35), color=ORANGE_DARK, lw=2, mutation=18)

    # enqueue / dequeue
    arrow(ax, (11.2, 3.425), (10.15, 3.425), color=ORANGE_DARK, lw=2,
          mutation=18)
    ax.text(11.35, 3.9, "enqueue\nat rear", ha="left", va="center", fontsize=10,
            color=ORANGE_DARK, linespacing=1.2)

    arrow(ax, (2.0, 3.425), (0.75, 3.425), color=GREEN_DARK, lw=2,
          mutation=18)
    ax.text(0.65, 3.95, "dequeue\nfrom front", ha="left", va="center",
            fontsize=10, color=GREEN_DARK, linespacing=1.2)

    ax.text(6.5, 1.35,
            "The first student to arrive is the first to be served.  enqueue and\n"
            "dequeue are O(1) because the queue keeps both front and rear pointers.",
            ha="center", va="center", fontsize=10, color=TEXT, linespacing=1.45)

    save(fig, "queue.png")


# ---------------------------------------------------------------------------
# 5. Growth-rate chart
# ---------------------------------------------------------------------------

def complexity_chart():
    n = np.array([8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096])

    fig, axes = plt.subplots(1, 2, figsize=(12.4, 4.7))
    fig.suptitle("Figure 5. Growth of Search and Sort Work",
                 fontsize=14.5, weight="bold", color=NAVY)

    ax = axes[0]
    ax.plot(n, n, marker="o", linewidth=2, color=ORANGE_DARK,
            label="Linear search: n")
    ax.plot(n, np.log2(n), marker="s", linewidth=2, color=BLUE,
            label="Binary search: log2(n)")
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xlabel("Number of records (n)")
    ax.set_ylabel("Comparisons (log scale)")
    ax.set_title("Searching", fontsize=11, color=NAVY)
    ax.grid(True, which="both", linestyle=":", alpha=0.45)
    ax.legend(fontsize=9)
    ax.tick_params(labelsize=8.5)

    ax = axes[1]
    bubble_worst = n * (n - 1) / 2
    selection = n * (n - 1) / 2
    bubble_best = n - 1
    ax.plot(n, bubble_worst, marker="o", linewidth=2, color=ORANGE_DARK,
            label="Bubble / selection: n(n-1)/2")
    ax.plot(n, bubble_best, marker="s", linewidth=2, color=GREEN_DARK,
            label="Bubble best case: n-1")
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xlabel("Number of records (n)")
    ax.set_ylabel("Comparisons (log scale)")
    ax.set_title("Sorting", fontsize=11, color=NAVY)
    ax.grid(True, which="both", linestyle=":", alpha=0.45)
    ax.legend(fontsize=9)
    ax.tick_params(labelsize=8.5)

    for ax in axes:
        for spine in ["top", "right"]:
            ax.spines[spine].set_visible(False)

    fig.tight_layout(rect=(0, 0, 1, 0.93))
    save(fig, "complexity_growth.png")


if __name__ == "__main__":
    main_flowchart()
    linked_list()
    stack_diagram()
    queue_diagram()
    complexity_chart()
