"""Run production library API regressions with simulated device registers."""
from pathlib import Path
import subprocess
import tempfile


def main():
    test_dir = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="pogobot-api-test-") as name:
        directory = Path(name)
        for filename in ("generated/csr.h", "spiflash.h"):
            path = directory / filename
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("#pragma once\n")
        executable = directory / "api_fault_test"
        subprocess.run([
            "cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
            "-fno-omit-frame-pointer",
            "-I", str(directory), "-I", str(test_dir.parent / "pogolib"),
            str(test_dir / "api_fault_test.c"), "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True, timeout=10)


if __name__ == "__main__":
    main()
