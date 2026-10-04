"""Write a shields.io endpoint badge from a gcovr JSON summary."""

import json
import os
import sys
from pathlib import Path


def badge_color(percent: float) -> str:
    if percent >= 80:
        return "brightgreen"
    if percent >= 60:
        return "yellow"
    return "red"


def main() -> None:
    summary = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    covered = summary["line_covered"]
    total = summary["line_total"]
    # An empty report means the filter matched nothing, a badge of 0% would hide that
    if total <= 0 or not summary["files"]:
        raise ValueError("gcovr reported no lines for the library")

    percent = 100 * covered / total
    badge = {
        "schemaVersion": 1,
        "label": "coverage",
        "message": f"{percent:.1f}%",
        "color": badge_color(percent),
    }
    Path(sys.argv[2]).write_text(json.dumps(badge) + "\n", encoding="utf-8")

    result = f"Line coverage of bibstd: {covered}/{total} ({percent:.1f}%)"
    print(result)
    step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if step_summary:
        with open(step_summary, "a", encoding="utf-8") as output:
            output.write(f"### Coverage\n\n{result}\n")


if __name__ == "__main__":
    main()
