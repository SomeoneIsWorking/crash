#!/usr/bin/env python3
"""Run the resident recompile evidence gate for Crash Bandicoot 2 (`SCUS-94154`)."""

from resident_recomp import TitleSpec, main


if __name__ == "__main__":
    raise SystemExit(main(TitleSpec("crash2", "SCUS-94154", "SCUS_941.54")))
