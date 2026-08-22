#!/usr/bin/env python3
"""Run the resident recompile evidence gate for Crash Bandicoot (`SCUS-94900`)."""

from resident_recomp import TitleSpec, main


if __name__ == "__main__":
    raise SystemExit(main(TitleSpec("crash1", "SCUS-94900", "SCUS_949.00")))
