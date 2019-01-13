# CPU2
A lightweight kernel in C++ that takes executables as command line arguments and round robins through them for a fixed length of time in a time-sharing fashion. This project allowed me to practice my knowledge of process control blocks, interrupt service vectors, sending signals, SIG traps, and piping information between parent and child processes.

All system calls are asserted, plus static and dynamic analysis was done with scan-build and valgrind, respectively. No warnings are generated, and no memory leakage.
