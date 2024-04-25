import subprocess
import os

test_cases = os.listdir(f"./inputs")
test_modes = ["basic", "bulk", "random"]
concurrency_modes = ["parallel", "sequential"]

# For PSC:
# all_num_threads = [1, 2, 4, 8, 16, 32, 64, 128]

# For Xeon Phi:
# all_num_threads = [1, 2, 4, 8, 9, 16, 18, 32, 36, 64, 72]

# For GHC:
all_num_threads = [1, 2, 4, 8]

for test_mode in test_modes:
    for test_case in test_cases:
        if test_mode in test_case:
            for num_threads in all_num_threads:
                concurrency_mode = concurrency_modes[bool(num_threads == 1)]
                command = f"./red-black-{concurrency_mode} -f inputs/{test_case} -n {num_threads}"
                process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
                stdout, stderr = process.communicate()
                with open(f"./outputs/{test_case}_{num_threads}.log", "w") as outfile:
                    outfile.write(stdout.decode("utf-8"))
                    outfile.write(stderr.decode("utf-8"))

                if b"error" in stdout or b"error" in stderr or b"Seg" in stderr:
                    print(f"Error encountered while running test case {test_case} with {num_threads} threads.")
                    print(stdout, stderr)
                    print("\n", flush=True)
                    continue
                else:
                    print(".", end="", flush=True)