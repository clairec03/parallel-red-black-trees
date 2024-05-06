# Parallel Red-black Trees
![speedup scaling](./program_speedup.png)
**Instructions:**

To compile, run the following:

`cd src && make parallel`

To run the compiled binary, `cd` into `src`, then enter:

`./red-black-parallel -n <number_of_threads> -f inputs/<test_case_file>.txt`

To obtain the performance metrics, run

`python3 run-test.py`

which will run test cases in `src/inputs` and save the program output (including computation time & speedup) to `src/outputs`.

To generate more test cases than the provided examples, run

`python3 test-gen.py`
