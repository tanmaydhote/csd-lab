# csd-lab

## CS4110 - Computer System Design Lab Assignments (Fall 2014)##

**NOTE:-** Keep this README updated whenever a change is made

### Assignment 2 --- Cache Simulator

This is a list of resources that may be useful:

1. source/tools/ManualExamples/pinatrace.cpp under the pin download root directory
2. http://www.cs.du.edu/~dconnors/courses/comp3361/notes/PinTutorial.pdf

#### Issues:

1. As of now the latency information is useless. Do we have to use a
timing model and if so, how?
2. _Assumption_:- All cache levels use the same replacement policy.
Do we need to mix and match?
3. The treatment of a memory read or a memory write is the same as of
now. Should they differ and if yes, in what way?
4. Compare results with others and see if the model is correct.
5. Code needs to be commented.

### Assignment 3 --- Out-of-order execution###

Found a set of slides that explains register renaming and Tomasulo's algorithm quite well along with detailed examples [here](https://www.student.cs.uwaterloo.ca/~cs450/w14/public/register%20renaming.pdf)

### Assignment 4 --- Cache Coherence###

MESI has been implemented. All operations have been done at block level. Blocks in memory are mapped to blocks in cache.
MOESI has also been implemented.
Both of them seem to be working fine. Look at the code once, should be simple enough to understand. Tell if you have any issues/clarifications.
http://en.wikipedia.org/wiki/MESI_protocol#mediaviewer/File:MESI_protocol_activity_diagram.png