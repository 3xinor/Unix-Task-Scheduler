/*
*	Assignment 3 
	PC Scheduling 
*	November 30, 2023
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#include <errno.h>
#include <stdbool.h>

#include "scheduler.h"

/* Mutex's to protect accessing ready queues and cpu running queues */
pthread_mutex_t q0_mutex, q1_mutex, q2_mutex, cpu0q_mutex, cpu1q_mutex, cpu2q_mutex,
	cpu3q_mutex;

/*single ready queues */
task RQ0[MAX_TASKS];
task RQ1[MAX_TASKS];
task RQ2[MAX_TASKS];

/* priority queue array of task trees */
task* p_queue[3] = {RQ0,RQ1,RQ2};

/*cpu thread queues of current assigned tasks */
cpu_queue cpu_tasks[4];

/* load balance queue */
task extracted_tasks[MAX_TASKS * 4];

void *producer_thread_function(void *arg);
void *cpu_thread_function(void *arg);
void *load_balance_thread_function(void *arg);

/* aquire mutex for specific ready queue */
void aquire_mutex(int queue) {
	switch(queue) {
		case 0:
			/* priority queue 0 */
			pthread_mutex_lock(&q0_mutex);
			break;
		case 1:
			/* priority queue 1 */
			pthread_mutex_lock(&q1_mutex);
			break;
		case 2:
			/* priority queue 2 */
			pthread_mutex_lock(&q2_mutex);
			break;
		case 3:
			/* cpu running queue 0 */
			pthread_mutex_lock(&cpu0q_mutex);
			break;
		case 4:
			/* cpu running queue 1 */
			pthread_mutex_lock(&cpu1q_mutex);
			break;
		case 5:
			/* cpu running queue 2 */
			pthread_mutex_lock(&cpu2q_mutex);
			break;
		case 6:
			/* cpu running queue 3 */
			pthread_mutex_lock(&cpu3q_mutex);
			break;
	}
}

/* release mutex for specific queue */
void release_mutex(int queue) {
	switch(queue) {
		case 0:
			/* priority queue 0 */
			pthread_mutex_unlock(&q0_mutex);
			break;
		case 1:
			/* priority queue 1 */
			pthread_mutex_unlock(&q1_mutex);
			break;
		case 2:
			/* priority queue 2 */
			pthread_mutex_unlock(&q2_mutex);
			break;
		case 3:
			/* cpu running queue 0 */
			pthread_mutex_unlock(&cpu0q_mutex);
			break;
		case 4:
			/* cpu running queue 1 */
			pthread_mutex_unlock(&cpu1q_mutex);
			break;
		case 5:
			/* cpu running queue 2 */
			pthread_mutex_unlock(&cpu2q_mutex);
			break;
		case 6:
			/* cpu running queue 3 */
			pthread_mutex_unlock(&cpu3q_mutex);
			break;
	}
}

int add_to_ready_queue(task px) {
	/* find which queue corresponds to task priority*/
	int queue_num;
	
	if (px.dynamic_priority < 100) { 
		queue_num = 0;
	} else if (px.dynamic_priority < 130) {
		queue_num = 1;
	} else {
		queue_num = 2;
	}
	
	aquire_mutex(queue_num);
	
	for(int idx = 0 ; idx < MAX_TASKS; idx++) {
		/* checks if array slice is not initialized */
		if (!p_queue[queue_num][idx].pid) {
			/* adds to first empty array element */
			p_queue[queue_num][idx] = px;
			release_mutex(queue_num);
			return idx;
		}
	}
	
	/* Queue is full failed to add */
	release_mutex(queue_num); 
	errno = ENOBUFS;
	return -1;
}

/* takes the lowest priority task capable of running on given cpu from priority queue */
task extract_from_ready_queue(int cpu_id) {
	int queue;
	int priority_of_best_task = 1000;
	int idx_of_best_task = -1;
	int idx;
	
	/* attempt to find runnable task priority queue 0 */
	
	aquire_mutex(0); 
	/* find highest priority task with affintiy for specified cpu */
	for(idx = 0 ; idx < MAX_TASKS ; idx++) {
		
		/* ensure task has correct cpu affinity */
		if(!p_queue[0][idx].pid){continue;}
		if(!(p_queue[0][idx].affinity == -1 || p_queue[0][idx].affinity == cpu_id)) {continue;}
		
		if(priority_of_best_task > p_queue[0][idx].dynamic_priority) {
			idx_of_best_task = idx;
			priority_of_best_task = p_queue[0][idx].dynamic_priority;
		}
	}
	queue = 0;
	release_mutex(0);
	
	/* attempt to find runnable task priority queue 1 */
	if (idx_of_best_task == -1) {
	
		aquire_mutex(1); 
		/* find highest priority task with affintiy for specified cpu */
		for(idx = 0 ; idx < MAX_TASKS ; idx++) {
			
			/* ensure task has correct cpu affinity */
			if(!p_queue[1][idx].pid){continue;}
			if(!(p_queue[1][idx].affinity == -1 || p_queue[0][idx].affinity == cpu_id)) {continue;}
			
			if(priority_of_best_task > p_queue[1][idx].dynamic_priority) {
				idx_of_best_task = idx;
				priority_of_best_task = p_queue[1][idx].dynamic_priority;
			}
		}
		queue = 1;
		release_mutex(1);
	}
	
	/* attempt to find runnable task priority queue 2 */
	if (idx_of_best_task == -1) {
		aquire_mutex(2); 
		/* find highest priority task with affintiy for specified cpu */
		for(idx = 0 ; idx < MAX_TASKS ; idx++) {
			
			/* ensure task has correct cpu affinity */
			if(!p_queue[2][idx].pid){continue;}
			if(!(p_queue[2][idx].affinity == -1 || p_queue[2][idx].affinity == cpu_id)) {continue;}
			
			if(priority_of_best_task > p_queue[2][idx].dynamic_priority) {
				idx_of_best_task = idx;
				priority_of_best_task = p_queue[2][idx].dynamic_priority;
			}
		}
		queue = 2;
		release_mutex(2);
	}
	
	/* return best task or null */
	
	if (idx_of_best_task == -1) {
		/* no tasks that can run on cpu */
		task empty_task;
		empty_task.pid = 0;
		return empty_task;
	} else {
		/* create a copy of the task, remove it from ready queue and return the task so its "running"*/
		task task_cpy = p_queue[queue][idx_of_best_task];
		p_queue[queue][idx_of_best_task].pid = 0;
		return task_cpy;
	}
}

void init_mutexs() {
	int res;
	res = pthread_mutex_init(&q0_mutex, NULL);
	if (res != 0) {
		perror("Mutex 0 initialization failed");
		exit(EXIT_FAILURE);
	}
	
	res = pthread_mutex_init(&q1_mutex, NULL);
	if (res != 0) {
		perror("Mutex 1 initialization failed");
		exit(EXIT_FAILURE);
	}
	
	res = pthread_mutex_init(&q2_mutex, NULL);
	if (res != 0) {
		perror("Mutex 2 initialization failed");
		exit(EXIT_FAILURE);
	}
	
	res = pthread_mutex_init(&cpu0q_mutex, NULL);
	if (res != 0) {
		perror("CPU Mutex 0 initialization failed");
		exit(EXIT_FAILURE);
	}
	
	res = pthread_mutex_init(&cpu1q_mutex, NULL);
	if (res != 0) {
		perror("CPU Mutex 1 initialization failed");
		exit(EXIT_FAILURE);
	}
	
	res = pthread_mutex_init(&cpu2q_mutex, NULL);
	if (res != 0) {
		perror("CPU Mutex 2 initialization failed");
		exit(EXIT_FAILURE);
	}
	
	res = pthread_mutex_init(&cpu3q_mutex, NULL);
	if (res != 0) {
		perror("CPU Mutex 3 initialization failed");
		exit(EXIT_FAILURE);
	}
	
}

int max(int first, int second) {
	if(first >= second) {
		return first;
	} else {
		return second;
	}
}

int min(int first, int second) {
	if(first <= second) {
		return first;
	} else {
		return second;
	}
}

void cpu_ready_queue_add(int cpu_id, task tx) {
	for (int idx = 0 ; idx < MAX_TASKS ; idx++) {
		if (cpu_tasks[cpu_id].queue[idx].pid == 0) {
			aquire_mutex(cpu_id + 3);
			
			// add task to cpu queue
			cpu_tasks[cpu_id].queue[idx] = tx;
			cpu_tasks[cpu_id].p_count++;
			
			release_mutex(cpu_id + 3);
			return;
		}
	}
	
	// cpu queue full, failed to add
	errno = ENOBUFS;
	return;
}

task remove_from_cpu_queue(int cpu_id, task tx) {
	for (int idx = 0 ; idx < MAX_TASKS ; idx++) {
		if (cpu_tasks[cpu_id].queue[idx].pid == tx.pid) {
			aquire_mutex(cpu_id + 3);
			
			// copy and remove task from cpu queue
			task cpy_task = cpu_tasks[cpu_id].queue[idx];
			cpu_tasks[cpu_id].queue[idx].pid = 0;
			cpu_tasks[cpu_id].p_count--;
			
			release_mutex(cpu_id + 3);
			return cpy_task;
		}
	}
	
	task null_task;
	null_task.pid = 0;
	return null_task;
}

/* adds a task to empty spot in array */
void load_balance_queue_add(task tx) {
	for(int idx = 0 ; idx < MAX_TASKS * 4 ; idx++) {
		if(extracted_tasks[idx].pid == 0) {
			extracted_tasks[idx] = tx;
			return;
		}
	}
	
	/* no empty spots available */
	errno = ENOBUFS;
	return;
}

/* remove highest priority task from load array */
task load_extract_highest_priority() {
	int highest = 0;
	for(int idx = 0 ; idx < MAX_TASKS * 4 ; idx++) {
		if (extracted_tasks[idx].pid == 0) {continue;}
		if (extracted_tasks[idx].dynamic_priority < extracted_tasks[highest].dynamic_priority) {
			highest = idx;
		}
	}
	
	task cpy = extracted_tasks[highest];
	extracted_tasks[highest].pid = 0;
	return cpy;
}


int main(){
	pthread_t load_balance_thread;
	pthread_t producer_thread;
	pthread_t cpu_threads[NUM_CPUS];
	void *thread_result;
	int threads;
	int res;
	srand(time(NULL));
	
	/* initialize mutexs for each ready queue */
	init_mutexs();
	
	/* Create a producer thread to create processes(tasks) and add to priority queue */
	res = pthread_create(&producer_thread, NULL, producer_thread_function, NULL);
	if (res != 0) {
    		perror("Thread creation failed");
   		exit(EXIT_FAILURE);
        }
        
        /* create a load balancing thread which periodically balances tasks every 4 secs */
        res = pthread_create(&load_balance_thread, NULL, load_balance_thread_function, NULL);
	if (res != 0) {
    		perror("Thread creation failed");
   		exit(EXIT_FAILURE);
        }
        
        /* Use 4 CPU threads which run asynchronously with main to handle all tasks in ready queue */
	for(threads = 0 ; threads < NUM_CPUS ; threads++) {
		res = pthread_create(&(cpu_threads[threads]), NULL, cpu_thread_function,
			(void*)&(threads));
		if (res  != 0){
			perror("Thread creatiion failed");
			exit(EXIT_FAILURE);
		}
		sleep(1);
	}
	
	/* join threads which won;t occur because they are all running infinitley */
	for(threads = NUM_CPUS - 1; threads >= 0; threads--) {
		res = pthread_join(cpu_threads[threads], &thread_result);
		if (res == 0) {
		    printf("Picked up a cpu thread\n");
		}
		else {
		    perror("pthread_join failed");
		}
	}
	
	printf("All done!\n");
	exit(EXIT_SUCCESS);
}


void *producer_thread_function(void *arg) {
	printf("Currently in Producer thread\n");
	int i;
	int upper_time = 8000;
	int lower_time = 1000;
	//Produce new tasks to be scheduled
	for(i = 0 ; i < NUM_TASKS ; i++) {
		
		//sleep to ensure processes are added to priority queue at differnent intervals of time
		if (i > 10) {usleep(rand() % 500 + 100);}
		
		task new_task;
		// initialize buffer members
		new_task.pid = (rand() % 1000) + 1000;
		new_task.remain_time = (rand()%(upper_time - lower_time + 1)) + lower_time;
		new_task.time_slice = 0; //done in thread cpu
		new_task.accu_time_slice = 0; //done in thread cpu
		new_task.last_cpu = -1; //done in thread cpu
		new_task.affinity = rand() % 4; 
		new_task.sched = (enum sched_policy)rand() % 3;
		new_task.blocked = false;
		
		/* set cpu affinity to -1 randomly */
		if(rand() % 8 == 7){new_task.affinity = -1;}
		
		switch(new_task.sched) {
			case 0:
				new_task.static_priority = (rand() % 31) + 20;
				new_task.dynamic_priority = new_task.static_priority;
				break;
			case 1:
				new_task.static_priority = (rand() % 41) + 30;
				new_task.dynamic_priority = new_task.static_priority;
				break;
			case 2:
				new_task.static_priority = (rand() % 31) + 100;
				new_task.dynamic_priority = new_task.static_priority;
				break;
		}
		
		/* add new task to ready queue */
		add_to_ready_queue(new_task);
		printf("Added process with id: %d, to ready queue\n", new_task.pid);
	}
	pthread_exit(NULL);
}

void *cpu_thread_function(void *arg) {
	int cpu_id = *(int *)arg;
	int bonus = 10;
	int time_quantum;
	char* policy;
	int sleep_time;
	int avg_sleep;
	
	
	printf("CPU: %d, initialized\n", cpu_id);
	while(1) {
		
		/* get a task from ready queue to run */
		task running_task = extract_from_ready_queue(cpu_id);
		
		if (running_task.pid == 0) {continue;}
		
		/* place task in local cpu ready queue */
		cpu_ready_queue_add(cpu_id, running_task);
	
		
		printf("\n\nCPU %d, now running task: %d\n", cpu_id, running_task.pid);
		printf("Last CPU %d\n", running_task.last_cpu);
		printf("CPU Affinity: %d\n", running_task.affinity);
		
		// check if process was blocked and calculates sleep average
		if(running_task.blocked) {
			running_task.blocked = false;
			gettimeofday(&running_task.unblocked_time, NULL);
			
			// calculate average sleep time in ms 
			sleep_time = (running_task.unblocked_time.tv_sec * 1000000 + running_task.blocked_time.tv_usec) / 100000000000;
		}
		
		switch(running_task.sched) {
			case 0:
				printf("Policy: RR\n");
				// update time slice
				if (running_task.static_priority < 120) {
					running_task.time_slice = (140 - running_task.static_priority) * 20;
				} else {
					running_task.time_slice = (140 - running_task.static_priority) * 5;
				}	
				
				// update remaining time
				running_task.remain_time -= running_task.time_slice;
				if (running_task.remain_time < 0) {
					running_task.time_slice += running_task.remain_time;
					running_task.remain_time = 0;
				}
				printf("Time Slice: %d ms\n", running_task.time_slice);
				printf("Remain Time: %d ms\n", running_task.remain_time);
				
				// update acute time
				running_task.accu_time_slice += running_task.time_slice;
				printf("Accu Time Slice: %d ms\n", running_task.accu_time_slice);
				
				break;
			case 1:
				printf("Policy: FIFO\n");
				// update time slice
				running_task.time_slice = running_task.remain_time;
				
				// update remaining time
				running_task.remain_time = 0;
				printf("Time Slice: %d ms\n", running_task.time_slice);
				printf("Remain Time: %d ms\n", running_task.remain_time);
				
				// update acute time
				running_task.accu_time_slice += running_task.time_slice;
				printf("Accu Time Slice: %d ms\n", running_task.accu_time_slice);
				
				break;
			case 2:
				printf("Policy: NORMAL\n");
				
				//generate cpu execution time for simulating the process being blocked
				int exec_time = rand() % (60 * 20);
				
				// update time slice
				if (running_task.static_priority < 120) {
					running_task.time_slice = (140 - running_task.static_priority) * 20;
				} else {
					running_task.time_slice = (140 - running_task.static_priority) * 5;
				}
				
				// compare random computation time to alloted time slice and put process in blocked state
				if (exec_time < running_task.time_slice) {
					running_task.blocked = true;
					gettimeofday(&running_task.blocked_time, NULL);
				} else {
					exec_time = running_task.time_slice;
					avg_sleep = (sleep_time/running_task.time_slice);
					printf("Average Sleep Time: %d ms\n", avg_sleep);
				}
				
				// update remaining time
				running_task.remain_time -= exec_time;
				
				if (running_task.remain_time < 0) {
					running_task.time_slice += running_task.remain_time;
					running_task.remain_time = 0;
				}
				printf("Time Slice: %d ms\n", running_task.time_slice);
				printf("Execution Time: %d ms\n", exec_time);
				printf("Remain Time: %d ms\n", running_task.remain_time);
				
				// update acute time
				running_task.accu_time_slice += exec_time;
				printf("Accu Time Slice: %d ms\n", running_task.accu_time_slice);
				
				// update dynamic priority value
				running_task.dynamic_priority = max(100, min(running_task.static_priority - 
					bonus + 5, 139));
				printf("Dynamic Priority: %d\n", running_task.dynamic_priority);
				break;
		}
		
		// update last cpu
		running_task.last_cpu = cpu_id;
		
		usleep(running_task.time_slice); // sleep for time_tu_run milliseconds
		
		/* Add back to priority queue if there is remaining time left and remove from cpu running queue */
		if (running_task.remain_time > 0) {
			printf("Adding task back to ready queue\n\n\n");
			add_to_ready_queue(running_task);
			remove_from_cpu_queue(cpu_id, running_task);
		} else if (running_task.remain_time == 0) {
			remove_from_cpu_queue(cpu_id, running_task);
		}
	}
}

/* simplified load balancing algs goal is to spread out the higher priority tasks amongst all cpus so they run in parrallel */

void *load_balance_thread_function(void *arg) {

	task cpy;
	task tx;
	
	while(1) {
		sleep(4);
		
		aquire_mutex(3);
		aquire_mutex(4);
		aquire_mutex(5);
		aquire_mutex(6);
		// find the total number of tasks currently locally queued to run for each cpu
		int total_tasks = cpu_tasks[0].p_count + cpu_tasks[1].p_count + cpu_tasks[2].p_count + 
			cpu_tasks[3].p_count;
		release_mutex(3);
		release_mutex(4);
		release_mutex(5);
		release_mutex(6);
		
		
		if (!total_tasks) {continue;}
		
		printf("\n\n Load Balance Algorithm Running \n");
		
		// aquire all mutexs
		for (int i = 0 ; i < 7 ; i++) {aquire_mutex(i);}
		
		// remove all tasks from local cpu queues
		
		for (int idx ; idx < MAX_TASKS ; idx++) {
			
			if(cpu_tasks[0].queue[idx].pid != 0) {
				cpy = remove_from_cpu_queue(0, cpu_tasks[0].queue[idx]);
				load_balance_queue_add(cpy);	
			}
			
			if(cpu_tasks[1].queue[idx].pid != 0) {
				cpy = remove_from_cpu_queue(1, cpu_tasks[1].queue[idx]);
				load_balance_queue_add(cpy);	
			}
			
			if(cpu_tasks[2].queue[idx].pid != 0) {
				cpy = remove_from_cpu_queue(2, cpu_tasks[2].queue[idx]);
				load_balance_queue_add(cpy);	
			}
			
			if(cpu_tasks[3].queue[idx].pid != 0) {
				cpy = remove_from_cpu_queue(3, cpu_tasks[3].queue[idx]);
				load_balance_queue_add(cpy);	
			}
		}
		
		// release all mutexs
		for (int i = 0 ; i < 7 ; i++) {release_mutex(i);}
		
		// reassign all task cpus and add to ready queue
		for (int i = 0; i < total_tasks ; i++) {
			tx = load_extract_highest_priority();
			tx.affinity = i % 4; // change affintiy
			add_to_ready_queue(tx);
		}
		printf("Load Balance Algorithm Complete\n\n");
	}
}


