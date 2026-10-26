"""
Automated tests for the PLMun Student Record Management System.

Usage:
    python tests/run_tests.py

The script copies the compiled program and a fresh students.csv into a
temporary folder for each scenario, feeds a scripted menu session through
standard input, checks the output, and saves the transcript under
tests/transcripts/.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TESTS = Path(__file__).resolve().parent
TRANSCRIPTS = TESTS / "transcripts"
SAMPLE = ROOT / "students.csv"
BINARY = ROOT / ("student_management.exe" if os.name == "nt" else "student_management")

if not BINARY.exists():
    print(f"ERROR: {BINARY.name} not found. Run build.bat or compile first.")
    sys.exit(1)

TRANSCRIPTS.mkdir(parents=True, exist_ok=True)


def lines(*items: str) -> str:
    """Join scripted answers. Empty strings are meaningful blank lines."""
    return "\n".join(items) + "\n"


def run_case(name: str, script: str, checks: list[str],
             setup=None, file_checks: list[str] | None = None,
             timeout: int = 30) -> str:
    with tempfile.TemporaryDirectory(prefix=f"plmun_{name}_") as raw:
        tmp = Path(raw)
        shutil.copy2(BINARY, tmp / BINARY.name)
        if setup is None:
            shutil.copy2(SAMPLE, tmp / "students.csv")
        else:
            setup(tmp)

        completed = subprocess.run(
            [str(tmp / BINARY.name)],
            input=script,
            text=True,
            capture_output=True,
            cwd=tmp,
            timeout=timeout,
        )
        output = completed.stdout + completed.stderr
        (TRANSCRIPTS / f"{name}.txt").write_text(output, encoding="utf-8")

        problems = []
        if completed.returncode != 0:
            problems.append(f"exit code {completed.returncode}")
        for needle in checks:
            if needle not in output:
                problems.append(f"missing text: {needle!r}")

        if file_checks:
            csv_path = tmp / "students.csv"
            csv_text = csv_path.read_text(encoding="utf-8") if csv_path.exists() else ""
            for needle in file_checks:
                if needle not in csv_text:
                    problems.append(f"missing in students.csv: {needle!r}")

        if problems:
            print(f"FAIL  {name}")
            for problem in problems:
                print(f"      - {problem}")
            print("      ---- output (first 60 lines) ----")
            for line in output.splitlines()[:60]:
                print("      " + line)
            raise AssertionError(name)

        print(f"PASS  {name}")
        return output


def run_persistence(name: str, first: str, second: str,
                    checks_first: list[str], checks_second: list[str],
                    file_checks: list[str] | None = None) -> None:
    with tempfile.TemporaryDirectory(prefix=f"plmun_{name}_") as raw:
        tmp = Path(raw)
        shutil.copy2(BINARY, tmp / BINARY.name)
        shutil.copy2(SAMPLE, tmp / "students.csv")

        one = subprocess.run([str(tmp / BINARY.name)], input=first, text=True,
                             capture_output=True, cwd=tmp, timeout=30)
        out_one = one.stdout + one.stderr
        (TRANSCRIPTS / f"{name}_1.txt").write_text(out_one, encoding="utf-8")

        two = subprocess.run([str(tmp / BINARY.name)], input=second, text=True,
                             capture_output=True, cwd=tmp, timeout=30)
        out_two = two.stdout + two.stderr
        (TRANSCRIPTS / f"{name}_2.txt").write_text(out_two, encoding="utf-8")

        problems = []
        if one.returncode != 0 or two.returncode != 0:
            problems.append(f"exit codes {one.returncode} and {two.returncode}")
        for needle in checks_first:
            if needle not in out_one:
                problems.append(f"first run missing: {needle!r}")
        for needle in checks_second:
            if needle not in out_two:
                problems.append(f"second run missing: {needle!r}")

        if file_checks:
            csv_text = (tmp / "students.csv").read_text(encoding="utf-8")
            for needle in file_checks:
                if needle not in csv_text:
                    problems.append(f"missing in students.csv: {needle!r}")

        if problems:
            print(f"FAIL  {name}")
            for problem in problems:
                print(f"      - {problem}")
            print("      ---- second output (first 60 lines) ----")
            for line in out_two.splitlines()[:60]:
                print("      " + line)
            raise AssertionError(name)

        print(f"PASS  {name}")


def fresh_folder(tmp: Path) -> None:
    """Leave the working directory without students.csv."""


def corrupt_file(tmp: Path) -> None:
    (tmp / "students.csv").write_text(
        "id,firstName,lastName,course,yearLevel,gpa\n"
        "202400001,Good,Record,BSIT,1,1.50\n"
        "202400002,Bad,Year,BSIT,9,1.50\n"
        "202400003,Bad,Gpa,BSCS,2,0.50\n"
        "202400004,Missing,Fields,BSIT\n"
        "202400005,Good,Two,BSBA,3,2.00\n",
        encoding="utf-8",
    )


def main() -> int:
    print("Running PLMun Student Record Management System tests\n")

    run_case(
        "01_display_reverse",
        lines("2", "y", "", "10", "y"),
        [
            "ALL STUDENT RECORDS",
            "REVERSE ORDER (recursive traversal)",
            "Recursive count      : 15",
            "202400015",
            "Goodbye.",
        ],
    )

    run_case(
        "02_linear_and_name_search",
        lines("3", "1", "202400008", "", "1", "999999999", "",
              "3", "lim", "", "0", "", "10", "y"),
        [
            "Linear search comparisons: 8",
            "No student with ID 999999999 was found.",
            "Linear search comparisons: 15",
            "1 matching record(s)",
            "Lim, Grace",
            "Goodbye.",
        ],
    )

    run_case(
        "03_binary_search_unsorted",
        lines("6", "2", "2", "",
              "3", "2", "202400008", "", "2", "999999999", "",
              "0", "", "10", "y"),
        [
            "Sorted by Last Name using selection sort",
            "pointer array was",
            "Binary search comparisons:",
            "No student with ID 999999999 was found.",
            "Goodbye.",
        ],
    )

    run_case(
        "04_binary_search_after_sort",
        lines("6", "1", "1", "", "3", "2", "202400008", "",
              "0", "", "10", "y"),
        [
            "Sorted by Student ID using bubble sort",
            "The list is now sorted by ID",
            "The list was already sorted by ID",
            "Binary search itself is O(log n)",
        ],
    )

    run_case(
        "05_add_and_duplicate",
        lines("1", "202499999", "Test", "Student", "BSIT", "1", "1.25", "",
              "1", "202400001", "", "10", "y"),
        [
            "Record added successfully",
            "Student, Test",
            "already exists",
        ],
        file_checks=["202499999,Test,Student,BSIT,1,1.25"],
    )

    run_case(
        "06_add_invalid_inputs",
        lines("1", "abc", "12345", "202499998", "Test", "User", "BSIT",
              "1", "6.0", "1.75", "", "10", "y"),
        [
            "Invalid input. Enter a whole number from",
            "Invalid GPA",
            "Record added successfully",
        ],
    )

    run_case(
        "07_update",
        lines("4", "202400007", "Paolo", "Mendoza", "BSIT", "2", "1.60",
              "y", "", "10", "y"),
        [
            "Current record",
            "Proposed record",
            "Record updated and saved",
            "Course   : BSIT",
            "GPA      : 1.60",
        ],
        file_checks=["202400007,Paolo,Mendoza,BSIT,2,1.60"],
    )

    run_case(
        "08_update_missing",
        lines("4", "999999999", "", "10", "y"),
        ["No student with ID 999999999 was found."],
    )

    run_case(
        "09_delete_stack_restore",
        lines("5", "202400013", "y", "",
              "7", "2", "", "3", "", "1", "", "3", "", "0", "",
              "10", "y"),
        [
            "pushed onto the Recently Deleted Stack",
            "Stack size: 1",
            "TOP OF STACK",
            "Aquino, Gabriel",
            "Restored from the stack",
            "the recently deleted stack is empty",
        ],
        file_checks=["202400013,Gabriel,Aquino,BSIT,1,2.75"],
    )

    run_case(
        "10_delete_restore_conflict",
        lines("5", "202400013", "y", "",
              "1", "202400013", "Replacement", "Student", "BSIT", "1", "2.00", "",
              "7", "1", "", "3", "", "0", "",
              "10", "y"),
        [
            "Cannot restore ID 202400013",
            "pushed back onto the stack",
            "Aquino, Gabriel",
        ],
    )

    run_case(
        "11_stack_empty",
        lines("7", "1", "", "2", "", "3", "", "0", "", "10", "y"),
        [
            "stack is empty. There is nothing to restore",
            "The stack is empty.",
            "recently deleted stack is empty",
        ],
    )

    run_case(
        "12_queue_flow",
        lines("8",
              "1", "202400001", "1", "",
              "1", "202400002", "2", "",
              "1", "202400001", "1", "",
              "3", "", "4", "",
              "2", "", "2", "", "2", "",
              "0", "", "10", "y"),
        [
            "position 1",
            "position 2",
            "already waiting in the queue",
            "NEXT STUDENT",
            "FRONT = position 1",
            "NOW SERVING",
            "Remaining: 1",
            "Remaining: 0",
            "queue is empty. There is no student to serve",
        ],
    )

    run_case(
        "13_sort_id_bubble",
        lines("6", "1", "1", "", "10", "y"),
        [
            "Sorted by Student ID using bubble sort",
            "Bubble sort: O(n^2)",
            "The list is now sorted by ID",
        ],
    )

    run_case(
        "14_sort_bubble_best_case",
        lines("6", "1", "1", "", "6", "1", "1", "", "10", "y"),
        [
            "Sorted by Student ID using bubble sort",
            "Comparisons: 14   Swaps: 0",
            "O(n) best when already",
        ],
    )

    run_case(
        "15_sort_lastname_selection",
        lines("6", "2", "2", "", "10", "y"),
        [
            "Sorted by Last Name using selection sort",
            "Comparisons: 105   Swaps:",
            "Selection sort: O(n^2)",
            "Aquino, Gabriel",
            "Villanueva, Mark",
        ],
    )

    run_case(
        "16_sort_gpa_bubble",
        lines("6", "3", "1", "", "10", "y"),
        [
            "Sorted by GPA using bubble sort",
            "Lim, Grace",
            "Santos, Maria",
            "not sorted by ID now",
        ],
    )

    run_case(
        "17_statistics",
        lines("9", "", "10", "y"),
        [
            "Total students (counter)",
            "Average GPA",
            "2.26",
            "Highest GPA (best, lowest value)",
            "1.00",
            "Lim, Grace",
            "Lowest GPA (highest value)",
            "5.00",
            "Ramirez, Sofia",
            "Passing (GPA <= 3.00)   : 13",
            "Failing / at-risk (GPA > 3.00): 2",
            "Dean's list (GPA <= 1.75): 6",
            "BS Information Technology",
        ],
    )

    run_case(
        "18_empty_operations",
        lines("2", "",
              "3", "1", "", "0", "",
              "4", "",
              "5", "",
              "6", "",
              "7", "3", "", "0", "",
              "8", "2", "", "0", "",
              "9", "",
              "10", "y"),
        [
            "No students are stored yet",
            "No records are available to search",
            "No records are available to update",
            "No records are available to delete",
            "At least two records are needed",
            "recently deleted stack is empty",
            "queue is empty. There is no student to serve",
            "No records are available. Add a student first.",
        ],
        setup=fresh_folder,
    )

    run_case(
        "19_corrupt_csv",
        lines("2", "n", "", "10", "y"),
        [
            "Loaded 2 record(s). Skipped 3 invalid line(s).",
            "Records in memory : 2",
            "Record, Good",
            "Two, Good",
        ],
        setup=corrupt_file,
    )

    run_case(
        "20_missing_file",
        lines("2", "", "10", "y"),
        [
            "No existing data file found",
            "Records in memory : 0",
            "No students are stored yet",
        ],
        setup=fresh_folder,
    )

    run_persistence(
        "21_persistence_add",
        lines("1", "202499999", "Persist", "Test", "BSIT", "1", "1.25", "",
              "10", "y"),
        lines("2", "n", "", "10", "y"),
        ["Record added successfully"],
        ["Loaded 16 record(s)", "Records in memory : 16", "202499999",
         "Test, Persist"],
        file_checks=["202499999,Persist,Test,BSIT,1,1.25"],
    )

    run_persistence(
        "22_persistence_delete",
        lines("5", "202400001", "y", "", "10", "y"),
        lines("2", "n", "", "10", "y"),
        ["Record deleted"],
        ["Loaded 14 record(s)", "Records in memory : 14"],
    )

    print("\nAll tests passed.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError:
        sys.exit(1)
