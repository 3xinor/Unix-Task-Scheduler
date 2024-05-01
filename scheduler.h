#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#define CPU1 0
#define CPU2 1
#define CPU3 2
#define CPU4 3
#define NUM_CPUS 4
#define NUM_TASKS 400

#define MAX_TASKS 256

enum sched_policy {
	RR,
	FIFO,
	NORMAL,
};
typedef struct {
	int pid;
	int static_priority;
	int dynamic_priority;
	int remain_time;
	int time_slice; 
	int accu_time_slice;
	int last_cpu;
	enum sched_policy sched;
	int affinity;
	bool blocked;
	struct timeval blocked_time;
	struct timeval unblocked_time;
	int sleep_avg;
} task;

typedef struct {
	task queue[MAX_TASKS];
	int p_count;
} cpu_queue;

#endif /*SCHEDULER_H*/
