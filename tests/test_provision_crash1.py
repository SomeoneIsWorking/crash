#!/usr/bin/env python3
"""Both-answer tests for Crash 1 disc resolution and provisioning."""

from __future__ import annotations

import pathlib
import stat
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import provision_crash1


class ResolverTests(unittest.TestCase):
    def setUp(self) -> None:
        (ROOT / "scratch").mkdir(exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=ROOT / "scratch")
        self.root = pathlib.Path(self.temporary.name)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def disc(self, name: str) -> pathlib.Path:
        path = self.root / name
        path.write_bytes(b"test CHD placeholder")
        return path

    def test_cli_wins_over_environment_and_dotenv(self) -> None:
        cli = self.disc("cli.chd")
        env = self.disc("env.chd")
        dot = self.disc("dot.chd")
        (self.root / ".env").write_text(f"PSXPORT_CRASH1_DISC={dot}\n", encoding="utf-8")
        resolved = provision_crash1.resolve_disc(
            str(cli), root=self.root, environ={"PSXPORT_CRASH1_DISC": str(env)}
        )
        self.assertEqual(resolved.path, cli.resolve())
        self.assertEqual(resolved.source, "CLI argument")

    def test_invalid_configured_path_refuses_instead_of_falling_through(self) -> None:
        self.disc("dropin.chd")
        with self.assertRaisesRegex(provision_crash1.Refused, "not a file"):
            provision_crash1.resolve_disc(
                None,
                root=self.root,
                environ={"PSXPORT_CRASH1_DISC": "missing.chd"},
            )

    def test_dotenv_precedes_single_dropin_and_resolves_relative_to_repo(self) -> None:
        selected = self.disc("configured.chd")
        self.disc("dropin.chd")
        (self.root / ".env").write_text(
            "PSXPORT_CRASH1_DISC='configured.chd'\n", encoding="utf-8"
        )
        resolved = provision_crash1.resolve_disc(None, root=self.root, environ={})
        self.assertEqual(resolved.path, selected.resolve())

    def test_generic_environment_is_supported(self) -> None:
        selected = self.disc("generic-env.chd")
        resolved = provision_crash1.resolve_disc(
            None, root=self.root, environ={"PSXPORT_DISC": selected.name}
        )
        self.assertEqual(resolved.path, selected.resolve())

    def test_generic_dotenv_is_supported(self) -> None:
        selected = self.disc("generic-dotenv.chd")
        (self.root / ".env").write_text(f"PSXPORT_DISC={selected.name}\n", encoding="utf-8")
        resolved = provision_crash1.resolve_disc(None, root=self.root, environ={})
        self.assertEqual(resolved.path, selected.resolve())

    def test_single_dropin_is_supported(self) -> None:
        selected = self.disc("dropin.chd")
        resolved = provision_crash1.resolve_disc(None, root=self.root, environ={})
        self.assertEqual(resolved.path, selected.resolve())

    def test_multiple_dropins_refuse_ambiguity(self) -> None:
        self.disc("one.chd")
        self.disc("two.chd")
        with self.assertRaisesRegex(provision_crash1.Refused, "ambiguous"):
            provision_crash1.resolve_disc(None, root=self.root, environ={})


class ProvisionTests(unittest.TestCase):
    def setUp(self) -> None:
        (ROOT / "scratch").mkdir(exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=ROOT / "scratch")
        self.root = pathlib.Path(self.temporary.name)
        self.disc = self.root / "disc.chd"
        self.disc.write_bytes(b"test CHD placeholder")
        self.discdump = self.root / "fake_discdump.py"
        self.discdump.write_text(
            "#!/usr/bin/env python3\n"
            "import pathlib, sys\n"
            "out = pathlib.Path(sys.argv[2])\n"
            "out.mkdir(parents=True, exist_ok=True)\n"
            "(out / 'SYSTEM.CNF').write_text('BOOT = cdrom:\\\\SCUS_949.00;1\\r\\n')\n"
            "(out / 'SCUS_949.00').write_bytes(b'identity fixture')\n",
            encoding="utf-8",
        )
        self.discdump.chmod(self.discdump.stat().st_mode | stat.S_IXUSR)
        self.output = self.root / "out"

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def test_matching_boot_and_identity_publish(self) -> None:
        result = provision_crash1.provision(
            self.disc,
            self.discdump,
            output_dir=self.output,
            identity_check=lambda _: [],
        )
        self.assertEqual(result.read_bytes(), b"identity fixture")
        self.assertTrue((self.output / "SYSTEM.CNF").is_file())

    def test_identity_disagreement_refuses_without_publishing(self) -> None:
        with self.assertRaisesRegex(provision_crash1.Refused, "1 tracked fact"):
            provision_crash1.provision(
                self.disc,
                self.discdump,
                output_dir=self.output,
                identity_check=lambda _: ["sha256 mismatch"],
            )
        self.assertFalse((self.output / "SCUS_949.00").exists())
        self.assertFalse((self.output / "SYSTEM.CNF").exists())


if __name__ == "__main__":
    unittest.main()
