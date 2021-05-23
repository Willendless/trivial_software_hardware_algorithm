#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <queue>
#include <algorithm>
#include <climits>
#include <assert.h>
using namespace std;

#define MAX_TASKS 50
#define MAX_CPUS 50
#define MAX(a1,a2) (a1>=a2)?(a1):(a2)

enum TaskState {
    TASK_FINISHED,
    TASK_RUNNING,
    TASK_READY, // task still need to check if all predecessors finished
    TASK_UNRELEASED,
};

struct Task {
    int exec_time;
    int release_time;
    int priority;
    int start_tick;
    TaskState state;
};

struct Cpu {
    int cur_task;
    int finish_time;
    int running_time;
    queue<int> task_record;
};

int n;
int ncpu;
Cpu cpus[MAX_CPUS];
Task tasks[MAX_TASKS];
int g[MAX_TASKS][MAX_TASKS];
int rankings[MAX_TASKS];
int traverse[MAX_TASKS];

enum EventKind {
    CPU_FINISHED,
    TASK_RELEASE,
};

struct ScheduleEvent {
    EventKind kind;
    int time;
    union {
        struct {
            int cpu_id;
            int finished_task_id;
        } cpu_data;
        int task_id;
    } data;
};

priority_queue<ScheduleEvent,
    vector<ScheduleEvent>, less<ScheduleEvent>> eventq;

/**
 * Notice:
 * 1. CPU_FINISHED task should be first so that released task will not preemptive them.
 * 2. Released Task with higher priority should follow so that released tasks within same tick
 *     will not preemptive each other.
 */
bool operator < (const ScheduleEvent &lhs, const ScheduleEvent &rhs) {
    if (lhs.time != rhs.time) return lhs.time > rhs.time;
    if (lhs.kind != rhs.kind) return lhs.kind == TASK_RELEASE;
    if (lhs.kind == CPU_FINISHED) return lhs.data.cpu_data.cpu_id > rhs.data.cpu_data.cpu_id;
    return tasks[lhs.data.task_id].priority < tasks[rhs.data.task_id].priority;
}

void read_input(FILE *f, bool *preemptive) {
    int is_preemptive, n1, n2;
    fscanf(f, "%d", &is_preemptive);
    if (is_preemptive) *preemptive = true;
    else *preemptive = false;
    fscanf(f, "%d %d", &n, &ncpu);
    for (int i = 1; i <= n; ++i) {
        fscanf(f, "%d %d", &tasks[i].exec_time, &tasks[i].release_time);
        if (tasks[i].release_time)
            tasks[i].state = TASK_UNRELEASED;
        else
            tasks[i].state = TASK_READY;
        rankings[i] = i;
    }
    while (fscanf(f, "%d %d", &n1, &n2) != EOF) g[n1][n2] = 1;
}

/**
 * Calculate node i's priority:
 *  times[i] + O(childs) + MAX(priorities[child])
 */
void calc_priority(int node) {
    int max_child_priority = 0;
    int childs = 0;
    for (int i = 1; i <= n; ++i) {
        if (g[node][i]) {
            if (!traverse[i]) calc_priority(i);
            max_child_priority = MAX(max_child_priority, tasks[i].priority);
            childs++;
        }
    }
    tasks[node].priority = tasks[node].exec_time + childs + max_child_priority;
    traverse[node] = 1;
}

/**
 * Calculate all tasks' priority
 */
void calc_priorities() {
    for (int i = 1; i <= n; ++i) {
        if (!traverse[i]) {
            calc_priority(i);
        }
    }
}

/**
 * Determine task's execution ranks based on:
 *  1. priority: descending
 *  2. release time: ascending
 *  3. execution time: descending
 *  4. index: ascending
 */
int task_cmp(const void *t1, const void *t2) {
    const int n1 = *(const int *)t1;
    const int n2 = *(const int *)t2;
    const int p1 = tasks[n1].priority;
    const int p2 = tasks[n2].priority;
    if (p1 != p2) return (-1) * (p1 - p2);
    const int rtime1 = tasks[n1].release_time;
    const int rtime2 = tasks[n2].release_time;
    if (rtime1 != rtime2) return rtime1 - rtime2;
    const int etime1 = tasks[n1].exec_time;
    const int etime2 = tasks[n2].exec_time;
    if (etime1 != etime2) return (-1) * (etime1 - etime2);
    return n1 - n2;
}

/**
 * Sorting all tasks based on their priorities and release time.
 */
void sort_tasks() {
    qsort(rankings + 1, n, sizeof(int), task_cmp);
}

void print_rankings() {
    printf("%10ctask number: %d%10ccpu number: %d\n", ' ', n, ' ', ncpu);
    printf("--------------------------------------------------------------------------------\n");
    printf("%10s %15s %15s %15s\n", "TASKS", "PRIORITY", "RELEASE_TIME", "EXECUTION_TIME");
    for (int i = 1; i <= n; ++i) {
        const int task = rankings[i];
        printf("%10d %15d %15d %15d\n", task, tasks[task].priority, tasks[task].release_time, tasks[task].exec_time);
    }
    printf("--------------------------------------------------------------------------------\n");
    for (int i = 1; i <= n; ++i) printf("%d %-2s", rankings[i], i == n? "" : ">");
    printf("\n");
    printf("--------------------------------------------------------------------------------\n");
}

void init_eventqueue(void) {
    // init tick 0 CPU_FINISHED events
    for (int i = 1; i <= ncpu; ++i) {
        ScheduleEvent se;
        se.kind = CPU_FINISHED;
        se.data.cpu_data.cpu_id = i;
        se.data.cpu_data.finished_task_id = 0;
        se.time = 0;
        eventq.push(se);
    }
    // init task release events
    for (int i = 1; i<= n; ++i) {
        if (tasks[i].state == TASK_UNRELEASED) {
            printf("add task %d release event to eq\n", i);
            ScheduleEvent se;
            se.kind = TASK_RELEASE,
            se.data.task_id = i,
            se.time = tasks[i].release_time,
            eventq.push(se);
        }
    }
}

void handle_cpu_finished_event(ScheduleEvent &se, int tick) {
    const int cpu_id = se.data.cpu_data.cpu_id;
    const int finished_task_id = se.data.cpu_data.finished_task_id;
    if (finished_task_id != cpus[cpu_id].cur_task) return;
    tasks[finished_task_id].state = TASK_FINISHED;
    cpus[cpu_id].cur_task = 0;
    cpus[cpu_id].running_time += tasks[finished_task_id].exec_time;
    if (finished_task_id)
        printf("[%04d] CPU %d finished executing Task %d\n", tick, cpu_id, finished_task_id);
}

int get_cpu_with_least_priority() {
    int least_priority = INT_MAX;
    int res = 0;
    for (int i = 1; i <= ncpu; ++i) {
        const int cur_task = cpus[i].cur_task;
        const int cur_priority = tasks[cur_task].priority;
        if (cur_task && cur_priority < least_priority) {
            least_priority = cur_priority;
            res = i;
        }
    }
    return res;
}

/**
 * Check if a task is ready, which means:
 *  1. It's release time has passed.
 *  2. All predecessors of it has finished.
 */
bool is_scheduable(int task) {
    if (tasks[task].state != TASK_READY) return false;
    for (int i = 1; i <= n; ++i) {
        if (g[i][task] && tasks[i].state != TASK_FINISHED)
            return false;
    }
    return true;
}

/**
 * Bookeep released task and try preemptive a cpu with least priority task.
 */
void handle_task_release_event(ScheduleEvent &se, int tick, bool preemptive) {
    const int task_id = se.data.task_id;
    const int task_priority = tasks[task_id].priority;
    tasks[task_id].release_time = 0;
    tasks[task_id].state = TASK_READY;
    
    printf("[%04d] Task %d releases\n", tick, task_id);
    if (!preemptive || !is_scheduable(task_id)) return;

    // preemptive phase
    const int least_priority_cpu = get_cpu_with_least_priority();
    if (least_priority_cpu <= 0) return;

    const int least_priority_task = cpus[least_priority_cpu].cur_task;
    const int least_priority = tasks[least_priority_task].priority;
    if (task_priority <= least_priority) return;

    // bookkeep preemptived task
    tasks[least_priority_task].exec_time -= tick - tasks[least_priority_task].start_tick;
    tasks[least_priority_task].state = TASK_READY;
    // bookkeep preemptived cpu
    cpus[least_priority_cpu].cur_task = task_id;
    cpus[least_priority_cpu].task_record.push(task_id);
    cpus[least_priority_cpu].finish_time = tick + tasks[task_id].exec_time;
    cpus[least_priority_cpu].running_time += tick - tasks[least_priority_task].start_tick;
    // push new schedule event
    ScheduleEvent new_se;
    new_se.kind = CPU_FINISHED;
    new_se.data.cpu_data.cpu_id = least_priority_cpu;
    new_se.data.cpu_data.finished_task_id = task_id;
    new_se.time = tick + tasks[task_id].exec_time;
    eventq.push(new_se);
    // bookkeep released task
    tasks[task_id].state = TASK_RUNNING;
    tasks[task_id].start_tick = tick;
    printf("[%04d] Task %d preemptives CPU %d with Task %d whose left time is %d\n",
        tick, task_id, least_priority_cpu, least_priority_task, tasks[least_priority_task].exec_time);
}

/**
 * Assign unfinished tasks to IDLE Cpu.
 */
void schedule(int tick) {
    int rank_id = 1;
    for (int i = 1; i <= ncpu; ++i) {
        if (!cpus[i].cur_task) {
            for (; rank_id <= n; ++rank_id) {
                const int task_id = rankings[rank_id];
                if (is_scheduable(task_id)) {
                    cpus[i].cur_task = task_id;
                    cpus[i].finish_time = tick + tasks[task_id].exec_time;
                    cpus[i].task_record.push(task_id);

                    ScheduleEvent se;
                    se.kind = CPU_FINISHED,
                    se.data.cpu_data.cpu_id = i,
                    se.data.cpu_data.finished_task_id = task_id,
                    se.time = tick + tasks[task_id].exec_time,
                    eventq.push(se);

                    tasks[task_id].start_tick = tick;
                    tasks[task_id].state = TASK_RUNNING;
                    printf("[%04d] CPU %d begins to run Task %d\n", tick, i, task_id);
                    rank_id++;
                    break;
                }
            }
        }
    }
}

/**
 * Schedule and execute tasks.
 */
int scheduler(bool preemptive) {
    int total_ticks = 0;
    init_eventqueue();
    while (eventq.size()) {
        const int cur_tick = total_ticks = eventq.top().time;
        do {
            assert(eventq.size() > 0);
            ScheduleEvent e = eventq.top();
            eventq.pop();
            switch (e.kind) {
            case CPU_FINISHED:
                handle_cpu_finished_event(e, cur_tick);
                break;
            case TASK_RELEASE:
                handle_task_release_event(e, cur_tick, preemptive);
                break;
            default:
                printf("Unknown event type\n");
                exit(-1);
                break;
            }
        } while (eventq.size() > 0 && eventq.top().time == cur_tick);
        schedule(cur_tick);
    }
    return total_ticks;
}

void print_metrics(int total_ticks) {
    printf("--------------------------------------------------------------------------------\n");
    printf("%10s %15s %15s %15s\n", "CPU", "RUNNING_TIME", "TOTAL_TIME", "UTILIZATION");
    for (int i = 1; i <= ncpu; ++i) {
        const int running_ticks = cpus[i].running_time;
        printf("%10d %15d %15d %15f     ", i, running_ticks, total_ticks, running_ticks * 1.0 / total_ticks);
        const int runned_tasks = cpus[i].task_record.size();
        for (int j = 0; j < runned_tasks; ++j) {
            printf(" %d%c", cpus[i].task_record.front(), j == runned_tasks - 1 ? '\0' : ',');
            cpus[i].task_record.pop();
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    FILE* f;
    bool preemptive = false;
    int total_ticks;
    if (argc == 1) {
        printf("usage: TaPsA [file name]\n");
        return 1;
    }
    if ((f = fopen(argv[1], "r")) == NULL) {
        printf("Failed to open file %s\n", argv[1]);
        return 1;
    }
    read_input(f, &preemptive);
    calc_priorities();
    sort_tasks();
    print_rankings();
    total_ticks = scheduler(preemptive);
    print_metrics(total_ticks);
    fclose(f);
    return 0;
}
