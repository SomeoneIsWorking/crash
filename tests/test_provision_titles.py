"""Both-answer tests for serial-separated Crash title provisioning."""

from __future__ import annotations

import pathlib
import stat
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import provision_title

CRASH1 = provision_title.SPECS["crash1"]
CRASH2 = provision_title.SPECS["crash2"]
SPECS = (CRASH1, CRASH2)


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

    def test_cli_wins_over_environment_and_dotenv_for_each_title(self) -> None:
        for spec in SPECS:
            with self.subTest(title=spec.slug):
                cli = self.disc(f"{spec.slug}-cli.chd")
                env = self.disc(f"{spec.slug}-env.chd")
                dot = self.disc(f"{spec.slug}-dot.chd")
                (self.root / ".env").write_text(
                    f"{spec.env_keys[0]}={dot}\n", encoding="utf-8"
                )
                resolved = provision_title.resolve_disc(
                    spec, str(cli), root=self.root, environ={spec.env_keys[0]: str(env)}
                )
                self.assertEqual(resolved.path, cli.resolve())
                self.assertEqual(resolved.source, "CLI argument")

    def test_invalid_configured_path_refuses_instead_of_falling_through(self) -> None:
        self.disc("dropin.chd")
        for spec in SPECS:
            with (
                self.subTest(title=spec.slug),
                self.assertRaisesRegex(provision_title.Refused, "not a file"),
            ):
                provision_title.resolve_disc(
                    spec,
                    None,
                    root=self.root,
                    environ={spec.env_keys[0]: "missing.chd"},
                )

    def test_dotenv_precedes_single_dropin_and_resolves_relative_to_repo(self) -> None:
        selected = self.disc("configured.chd")
        self.disc("dropin.chd")
        for spec in SPECS:
            with self.subTest(title=spec.slug):
                (self.root / ".env").write_text(
                    f"{spec.env_keys[0]}='configured.chd'\n", encoding="utf-8"
                )
                resolved = provision_title.resolve_disc(
                    spec, None, root=self.root, environ={}
                )
                self.assertEqual(resolved.path, selected.resolve())

    def test_generic_environment_and_dotenv_are_supported(self) -> None:
        selected = self.disc("generic.chd")
        for spec in SPECS:
            with self.subTest(title=spec.slug, source="environment"):
                resolved = provision_title.resolve_disc(
                    spec, None, root=self.root, environ={"PSXPORT_DISC": selected.name}
                )
                self.assertEqual(resolved.path, selected.resolve())
            with self.subTest(title=spec.slug, source="dotenv"):
                (self.root / ".env").write_text(
                    f"PSXPORT_DISC={selected.name}\n", encoding="utf-8"
                )
                resolved = provision_title.resolve_disc(
                    spec, None, root=self.root, environ={}
                )
                self.assertEqual(resolved.path, selected.resolve())

    def test_title_specific_keys_do_not_cross_select(self) -> None:
        selected = self.disc("other-title.chd")
        with self.assertRaisesRegex(provision_title.Refused, "no disc image"):
            provision_title.resolve_disc(
                CRASH2,
                None,
                root=self.root / "empty",
                environ={"PSXPORT_CRASH1_DISC": str(selected)},
            )
        with self.assertRaisesRegex(provision_title.Refused, "no disc image"):
            provision_title.resolve_disc(
                CRASH1,
                None,
                root=self.root / "empty",
                environ={"PSXPORT_CRASH2_DISC": str(selected)},
            )

    def test_single_dropin_is_supported_and_multiple_are_refused(self) -> None:
        dropin_root = self.root / "dropins"
        dropin_root.mkdir()
        selected = dropin_root / "one.chd"
        selected.write_bytes(b"one")
        for spec in SPECS:
            with self.subTest(title=spec.slug):
                resolved = provision_title.resolve_disc(
                    spec, None, root=dropin_root, environ={}
                )
                self.assertEqual(resolved.path, selected.resolve())
        (dropin_root / "two.chd").write_bytes(b"two")
        for spec in SPECS:
            with (
                self.subTest(title=spec.slug),
                self.assertRaisesRegex(provision_title.Refused, "ambiguous"),
            ):
                provision_title.resolve_disc(spec, None, root=dropin_root, environ={})


class ProvisionTests(unittest.TestCase):
    def setUp(self) -> None:
        (ROOT / "scratch").mkdir(exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=ROOT / "scratch")
        self.root = pathlib.Path(self.temporary.name)
        self.disc = self.root / "disc.chd"
        self.disc.write_bytes(b"test CHD placeholder")

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def discdump(self, executable: str) -> pathlib.Path:
        tool = self.root / f"fake-{executable}.py"
        tool.write_text(
            "#!/usr/bin/env python3\n"
            "import pathlib, sys\n"
            "out = pathlib.Path(sys.argv[2])\n"
            "out.mkdir(parents=True, exist_ok=True)\n"
            f"(out / 'SYSTEM.CNF').write_text('BOOT = cdrom:\\\\{executable};1\\r\\n')\n"
            f"(out / '{executable}').write_bytes(b'identity fixture')\n",
            encoding="utf-8",
        )
        tool.chmod(tool.stat().st_mode | stat.S_IXUSR)
        return tool

    def test_matching_boot_and_identity_publish_for_each_title(self) -> None:
        for spec in SPECS:
            with self.subTest(title=spec.slug):
                executable = provision_title.expected_executable(spec)
                output = self.root / f"{spec.slug}-out"
                result = provision_title.provision(
                    spec,
                    self.disc,
                    self.discdump(executable),
                    output_dir=output,
                    identity_check=lambda _: [],
                )
                self.assertEqual(result.read_bytes(), b"identity fixture")
                self.assertTrue((output / "SYSTEM.CNF").is_file())

    def test_identity_disagreement_refuses_without_publishing(self) -> None:
        for spec in SPECS:
            with self.subTest(title=spec.slug):
                executable = provision_title.expected_executable(spec)
                output = self.root / f"{spec.slug}-identity-fail"
                with self.assertRaisesRegex(provision_title.Refused, "1 tracked fact"):
                    provision_title.provision(
                        spec,
                        self.disc,
                        self.discdump(executable),
                        output_dir=output,
                        identity_check=lambda _: ["sha256 mismatch"],
                    )
                self.assertFalse((output / executable).exists())
                self.assertFalse((output / "SYSTEM.CNF").exists())

    def test_other_crash_title_boot_target_refuses_without_publishing(self) -> None:
        crash1_executable = provision_title.expected_executable(CRASH1)
        output = self.root / "wrong-title"
        with self.assertRaisesRegex(
            provision_title.Refused, "SCUS_949.00.*SCUS_941.54"
        ):
            provision_title.provision(
                CRASH2,
                self.disc,
                self.discdump(crash1_executable),
                output_dir=output,
                identity_check=lambda _: [],
            )
        self.assertFalse((output / crash1_executable).exists())
        self.assertFalse((output / "SYSTEM.CNF").exists())


if __name__ == "__main__":
    unittest.main()
