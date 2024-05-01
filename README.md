Completely Fair Scheduler (CFS)

Project Description
This application simulates a simple CFS using multi-threading to represent 4 CPUs and task structs in a 3-
level priority queue. The task struct members act as a pseudo PCB keeping track of the process context
to schedule it by moving it from the ready queue to CPU. All processes have a CPU affinity and
scheduling policy (FIFO, RR, NORMAL). FIFO and RR are for real-time processes and NORMAL is for
processes that may go from the running → blocked state at any point. Each process's simulated
execution on a CPU thread is handled based on its policy and if it's NORMAL a random execution time
may override the allocated time slice to represent the process being blocked. There is also a separate
thread that periodically checks the running queues of each CPU thread and performs load balancing by
changing process CPU affinities so that running processes are spread out amongst all CPUs. The simple
load balancing algorithm also ensures higher priority tasks are spread out amongst all CPUs.
How to Install and Run the Project
To install the project simply download the provided zip file and unpack its contents. To run the project
the user must be running the application within a UNIX-based operating system such as a MacOS or
Linux.

How to Use the Project
To use the program once downloaded and unpacked simply:
1. Open the command terminal to the location of project download
2. Type “make” into the terminal to execute the makefile
3. Type “./CFS” to execute application
NOTE: producer thread used to insantiate random tasks with appropriate parameters. The number of
tasks produced is managed by the #NUM_TASKS macro. This macro can be reliably set up to 256, the
maximum number of tasks each level of priority queue can take in. However, NUM_TASKS is currently
set to 400 and can go as high as 768 with an increased risk of data loss.

Credits
Created by Dave Exinor 101184298
Assignment 3 SYSC 4001
Prof. Chung Lung
Carleton University
