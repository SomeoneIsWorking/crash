"""Dependency provenance refusal cases through Crash's shipping checker."""

from __future__ import annotations

import contextlib
import io
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import psxport_sync


class ProvenanceTests(unittest.TestCase):
    def check(self, built: tuple[str, str] | None, head: str, dirty: bool) -> int:
        with (
            patch.object(psxport_sync, "read_pin", return_value=("https://example.invalid/runtime", "a" * 40)),
            patch.object(psxport_sync, "read_resolved", return_value=built),
            patch.object(psxport_sync, "head_of", return_value=head),
            patch.object(psxport_sync, "dirty", return_value=dirty),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            return psxport_sync.do_check(SimpleNamespace(build_dir="build/verify"))

    def test_clean_matching_checkout(self) -> None:
        self.assertEqual(self.check(("framework", "a" * 40), "a" * 40, False), 0)

    def test_missing_configuration_refuses(self) -> None:
        self.assertEqual(self.check(None, "a" * 40, False), 2)

    def test_dirty_checkout_refuses(self) -> None:
        self.assertEqual(self.check(("framework", "a" * 40), "a" * 40, True), 1)

    def test_checkout_changed_since_configuration_refuses(self) -> None:
        self.assertEqual(self.check(("framework", "a" * 40), "b" * 40, False), 1)

    def test_build_disagrees_with_pin_refuses(self) -> None:
        self.assertEqual(self.check(("framework", "b" * 40), "b" * 40, False), 1)


if __name__ == "__main__":
    unittest.main()
