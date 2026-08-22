#!/usr/bin/env python3
"""Run the resident recompile evidence gate for Crash Bandicoot: Warped (`SCUS-94244`)."""

from resident_recomp import TitleSpec, main


if __name__ == "__main__":
    raise SystemExit(main(TitleSpec("crash3", "SCUS-94244", "SCUS_942.44")))
