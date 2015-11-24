NOTES ON USAGE
==============

Assignment 2 --- Cache Simulator
--------------------------------

1. To compile without running the tests
```bash
make PIN_ROOT=/path/to/root/pin/dir
```
2. To compile for debugging purposes
```bash
make PIN_ROOT=/path/to/root/pin/dir DEBUG=1
```
3. To compile and run the tests automatically
```bash
make PIN_ROOT=<root pin directory> matrix_multiply.test
```

If you want to run the tool on a single test case,
first compile (preferably with DEBUG=1) and then 
run the following command (assuming you are in the
directory with the source files):-
```bash
/path/to/root/pin/dir/pin.sh -t obj-intel64/cache_sim_tool.so -f /path/to/config/file -- obj-intel64/matrix_multiply <MATRIX_SIZE>
```

For full details on how to debug a pintool, go through the
Pin tutorial at https://software.intel.com/sites/landingpage/pintool/docs/65163/Pin/html/index.html#DEBUGGING

