import os
import subprocess as sp

TEST_DIR = "tests"
CHECK_IMPL_DEFINED = False

BW     = "\x1b[0m"
RED    = "\x1b[31;1m"
GREEN  = "\x1b[32;1m"
BLUE   = "\x1b[36;1m"

test_files = os.listdir(TEST_DIR)
y_files = sorted(file for file in test_files if file.startswith("y_"))
n_files = sorted(file for file in test_files if file.startswith("n_"))
i_files = sorted(file for file in test_files if file.startswith("i_"))
max_len = max(map(len, y_files + n_files + i_files))

failures = 0

def run_validator_tests(files, unexpected=None):
	global failures
	for file in files:
		path = f"{TEST_DIR}/{file}"
		proc = sp.run(["out/validator", path],
			capture_output=True, timeout=1)
		failed = False
		colored = False
		if proc.returncode not in (0, 2):
			print(RED + path)
			print("→ Parser crashed")
			failed = True
			colored = True
		else:
			if unexpected is None:
				print(BLUE + path)
				colored = True
				if proc.returncode == 0:
					print(f"→ Success")
			elif proc.returncode == unexpected:
				print(path)
				if proc.returncode == 0:
					print("→ Parsing should have failed")
				failed = True
		if failed:
			failures += 1
		if len(proc.stderr) > 0 and (failed or unexpected is None):
			print("→ " + proc.stderr.decode("utf-8", "replace"), end="")
		if colored:
			print(BW, end="")

run_validator_tests(y_files, 2)
run_validator_tests(n_files, 0)
if CHECK_IMPL_DEFINED:
	print()
	run_validator_tests(i_files)

if CHECK_IMPL_DEFINED or failures != 0:
	print()

if failures == 0:
	print(GREEN+f"All validator tests passed!"+BW)
else:
	print(RED+f"{failures} validator failures!"+BW)
	exit(1)
