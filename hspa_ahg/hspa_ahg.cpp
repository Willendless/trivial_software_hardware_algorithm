#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <queue>
#include <algorithm>
#include <climits>
#include <assert.h>
using namespace std;

#define MAX_TASKS 50
#define MAX_CPUS 50
#define MAX_EXECUTORS 5
#define BUF_SIZE 2048
#define MAX(a1,a2) ((a1>=a2)?(a1):(a2))

enum TaskState {
    TASK_UNRELEASED,
    TASK_FINISHED,
    TASK_RUNNING,
    TASK_WAITING,
    TASK_READY
};

struct Task {
    int priority;       // task priority
    int release_time;   // task release time
    int start_tick;     // task start tick
    int exec_time;      // task execution time
    TaskState state;    // task current state
    // bookkeep info
    int running_executor;
    int running_cpu;
};

struct Cpu {
    int cur_task;
    int finish_time;
    int running_time;
    queue<int> task_record;
};

enum EventKind {
    TASK_RELEASE,
    CPU_FINISHED,
    COMMUNICATION_FINISHED
};

struct ScheduleEvent {
    EventKind kind;
    union {
        struct {
            int task_id;
        } task_release_data;

        struct {
            int executor_id;
            int cpu_id;
            int finished_task_id;
        } cpu_finished_data;

        struct {
            int from_task_id;
            int to_task_id;
        } communication_finished_data;
    } payload;
    int time;
};

struct Executor {
    enum Kind {
        HARD,
        SOFT
    } type;
    int ntasks;
    int ncpu;
    int tasks[MAX_TASKS];
    Cpu cpus[MAX_CPUS];
};

int nalltasks;                          // number of all tasks
int nexecutors;                         // number of executors
Executor executors[MAX_EXECUTORS];      // executors
int dep[MAX_TASKS][MAX_TASKS];          // inter-task communication costs
Task tasks[MAX_TASKS];                  // global task list
char comm_buf[BUF_SIZE];                // mermaid comm buf
int comm_bufp;                          // comm buf pointer
char md_comm_buf[BUF_SIZE];             // markdown comm buf
int md_comm_bufp;

// schedule event queue
priority_queue<ScheduleEvent,
    vector<ScheduleEvent>, less<ScheduleEvent>> eventq;

/**
 * Notice:
 * 1. time ascending
 * 2. CPU_FINISHED task should be next, so that communication_finished/released tasks are able to run on them
 * 2. for COMMUNICATION_FINISHED, TASK_RELEASE tasks, priority descending
 */
bool operator < (const ScheduleEvent &lhs, const ScheduleEvent &rhs) {
    if (lhs.time != rhs.time) return lhs.time > rhs.time;
    if (lhs.kind != rhs.kind) {
        if (rhs.kind == CPU_FINISHED) return true;
        if (lhs.kind == CPU_FINISHED) return false;
        int tid, lhs_priority, rhs_priority;
        switch (lhs.kind) {
            case COMMUNICATION_FINISHED: 
                tid = lhs.payload.communication_finished_data.to_task_id;
                lhs_priority = tasks[tid].priority;
                break;
            case TASK_RELEASE:
                tid = rhs.payload.task_release_data.task_id;
                rhs_priority = tasks[tid].priority;
                break;
            default: assert(0 && "code should not reach here");
        }
        return lhs_priority < rhs_priority;
    } else {
        if (lhs.kind == CPU_FINISHED) {
            int l_eid, l_cid, r_eid, r_cid;
            l_eid = lhs.payload.cpu_finished_data.executor_id;
            l_cid = lhs.payload.cpu_finished_data.cpu_id;
            r_eid = rhs.payload.cpu_finished_data.executor_id;
            r_cid = rhs.payload.cpu_finished_data.cpu_id;
            return l_eid == r_eid ? l_cid > r_cid : l_eid > r_eid;
        }

        int ltid, rtid, lhs_priority, rhs_priority;
        switch (lhs.kind) {
            case COMMUNICATION_FINISHED:
                ltid = lhs.payload.communication_finished_data.to_task_id;
                rtid = rhs.payload.communication_finished_data.to_task_id;
            case TASK_RELEASE:
                ltid = lhs.payload.task_release_data.task_id;
                rtid = rhs.payload.task_release_data.task_id;
        }
        lhs_priority = tasks[ltid].priority;
        rhs_priority = tasks[rtid].priority;
        return lhs_priority == rhs_priority ? ltid > rtid : lhs_priority < rhs_priority;
    }
}

/**
 * Check if a task is ready, which means:
 *  1. It's release time has passed.
 *  2. All predecessors of it has finished.
 */
bool is_scheduable(int task) {
    if (tasks[task].state == TASK_READY) return true;
    for (int i = 1; i <= nalltasks; ++i) {
        if (dep[i][task])
            return false;
    }
    return true;
}

Executor read_executor_setting(FILE *f, int sid) {
    Executor e;
    fscanf(f, "%d%d", &e.ntasks, &e.ncpu);
    for (int i = 1; i <= e.ntasks; i++) {
        Task *t;
        fscanf(f, "%d", &e.tasks[i]);
        t = &tasks[e.tasks[i]];
        fscanf(f, "%d%d",
                &t->exec_time,
                &t->release_time);
    }
    return e;
}

void read_input(FILE *f) {
    int a, b, w;

    fscanf(f, "%d%d", &nalltasks, &nexecutors);
    for (int i = 1; i <= nexecutors; i++) {
        executors[i] = read_executor_setting(f, i);
    }

    // read dependencies matrix
    while (fscanf(f, "%d%d%d", &a, &b, &w) != EOF) {
        dep[a][b] = w;
    }
}

/**
 * Init task state based on its release time and dependency relations.
 */
void init_tasks(void) {
    for (int i = 1; i <= nalltasks; ++i) {
        Task *const t = &tasks[i];
        if (t->release_time)
            t->state = TASK_UNRELEASED;
        else if (is_scheduable(i))
            t->state = TASK_READY;
        else
            t->state = TASK_WAITING;
    }
}

/**
 * Calculate node i's priority:
 *  times[i] + O(childs) + MAX(comm[child] + priorities[child])
 */
void calc_priority(int t, bool *traverse) {
    assert(t <= nalltasks && "task id should be less than nalltasks");
    int childs_cnt = 0;
    int max_child_cost = 0;
    for (int i = 1; i <= nalltasks; ++i) {
        if (dep[t][i]) {
            if (!traverse[i]) calc_priority(i, traverse);
            max_child_cost = MAX(max_child_cost,
                                 dep[t][i] + tasks[i].priority);
            ++childs_cnt;
        }
    }
    tasks[t].priority = tasks[t].exec_time + childs_cnt + max_child_cost;
    traverse[t] = true;
}

/**
 * Calculate all tasks' priority
 */
void calc_priorities() {
    bool traverse[nalltasks];
    memset(traverse, 0, sizeof(traverse));

    for (int i = 1; i <= nalltasks; ++i) {
        if (!traverse[i])
            calc_priority(i, traverse);
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
 * Sorting each executor's all tasks based on their priorities and release time.
 */
void sort_tasks() {
    for (int i = 1; i <= nexecutors; ++i) {
        Executor *e = &executors[i];
        qsort(e->tasks + 1, e->ntasks, sizeof(int), task_cmp);
    }
}

void print_rankings() {
    printf("%10ctask number: %d%10cexecutor number: %d\n", ' ', nalltasks, ' ', nexecutors);

    printf("%10s %15s %15s %15s\n", "TASKS", "PRIORITY", "RELEASE_TIME", "EXECUTION_TIME");
    for (int i = 1; i <= nexecutors; ++i) {
        printf("--------------------------------------------------------------------------------\n");
        for (int j = 1; j <= executors[i].ntasks; ++j) {
            const int task = executors[i].tasks[j];
            printf("%10d %15d %15d %15d\n", task, tasks[task].priority, tasks[task].release_time, tasks[task].exec_time);
        }
    }
    printf("\n");
    fflush(stdout);
}

// init task release / communication finished events
// COMMUNICATION_FINISHED: if task i has no successor or the weight of all out edges are 0
// TASK_RELEASE: otherwise
void init_eventqueue(void) {
    // dummy boot event
    ScheduleEvent se;
    se.kind = CPU_FINISHED;
    se.time = 0;
    se.payload.cpu_finished_data.executor_id = 1;
    se.payload.cpu_finished_data.cpu_id = 1;
    se.payload.cpu_finished_data.finished_task_id = 0;
    eventq.push(se);

    for (int i = 1; i<= nalltasks; ++i) {
        if (tasks[i].state == TASK_UNRELEASED) {
            bool next = false;
            ScheduleEvent se;
            for (int j = 1; j <= nalltasks; ++j) {
                if (dep[j][i]) {
                    se.kind = TASK_RELEASE;
                    se.time = tasks[i].release_time;
                    se.payload.task_release_data.task_id = i;
                    eventq.push(se);
                    next = true;
                }
            }
            if (next) continue;
            se.kind = COMMUNICATION_FINISHED;
            se.time = tasks[i].release_time,
            se.payload.task_release_data.task_id = i;
            eventq.push(se);
        }
    }
}

// Bookkeep task and cpu info and then trigger communication finished event
void handle_cpu_finished_event(ScheduleEvent &se, int tick) {
    const int executor_id = se.payload.cpu_finished_data.executor_id;
    const int cpu_id = se.payload.cpu_finished_data.cpu_id;
    const int finished_task_id = se.payload.cpu_finished_data.finished_task_id;

    if (finished_task_id <= 0 || finished_task_id > nalltasks)
        return;

    Cpu *const cpu = &executors[executor_id].cpus[cpu_id];

    tasks[finished_task_id].state = TASK_FINISHED;

    cpu->cur_task = 0;
    cpu->running_time += tasks[finished_task_id].exec_time;

    if (finished_task_id)
        printf("[%04d] CPU %d.%d finished executing Task %d\n", tick, executor_id, cpu_id, finished_task_id);
    
    for (int i = 1; i <= nalltasks; ++i) {
        int comm_cost = dep[finished_task_id][i];
        if (comm_cost) {
            ScheduleEvent se;
            se.kind = COMMUNICATION_FINISHED;
            se.time = tick + comm_cost;
            se.payload.communication_finished_data.from_task_id = finished_task_id;
            se.payload.communication_finished_data.to_task_id = i;
            eventq.push(se);
            printf("[%04d] TASK %d begin to communicate with Task %d\n", 
                    tick, finished_task_id, i);
        }
    }
}

/**
 * Bookeep released task and try preemptive a cpu with least priority task.
 */
void handle_task_release_event(ScheduleEvent &se, int tick) {
    const int task_id = se.payload.task_release_data.task_id;

    if (is_scheduable(task_id))
        tasks[task_id].state = TASK_READY;
    else
        tasks[task_id].state = TASK_WAITING;
    
    printf("[%04d] Task %d released\n", tick, task_id);
}


/**
 * Bookkeep communication cost matrix and task.
 */
void handle_communication_finish_event(ScheduleEvent &se, int tick) {
    const int from_tid = se.payload.communication_finished_data.from_task_id;
    const int to_tid = se.payload.communication_finished_data.to_task_id;

    dep[from_tid][to_tid] = 0;

    // mermaid bookkeeping
    char s[100];
    int stime = tasks[from_tid].start_tick + tasks[from_tid].exec_time;
    sprintf(s, "\t%d->%d :done, des1, %2d, %2d\n",
            from_tid, to_tid, stime, tick);
    sprintf(comm_buf + comm_bufp, "%s", s);
    comm_bufp += strlen(s);
    sprintf(s, "B(%d->%d,%d,%d)<br>", from_tid, to_tid, stime, tick);
    sprintf(md_comm_buf + md_comm_bufp, "%s", s);
    md_comm_bufp += strlen(s);

    if (is_scheduable(to_tid))
        tasks[to_tid].state = TASK_READY;
}

/**
 * Assign unfinished tasks to IDLE Cpu.
 */
void schedule(int tick) {
    for (int i = 1; i <= nexecutors; ++i) {
        int rank_id = 1;
        Executor *const executor = &executors[i];
        for (int j = 1; j <= executor->ncpu; ++j) {
            if (!executor->cpus[j].cur_task) {
                Cpu *const cpu = &executors[i].cpus[j];
                for (; rank_id <= executors[i].ntasks; ++rank_id) {
                    int task_id = executors[i].tasks[rank_id];
                    if (tasks[task_id].state == TASK_READY) {
                        cpu->cur_task = task_id;
                        cpu->finish_time = tick + tasks[task_id].exec_time;
                        cpu->task_record.push(task_id);

                        ScheduleEvent se;
                        se.kind = CPU_FINISHED,
                        se.time = tick + tasks[task_id].exec_time,
                        se.payload.cpu_finished_data.executor_id = i;
                        se.payload.cpu_finished_data.cpu_id = j;
                        se.payload.cpu_finished_data.finished_task_id = task_id;
                        eventq.push(se);

                        tasks[task_id].start_tick = tick;
                        tasks[task_id].state = TASK_RUNNING;
                        tasks[task_id].running_executor = i;
                        tasks[task_id].running_cpu = j;
                        printf("[%04d] CPU %d.%d begins to run Task %d\n", tick, i, j, task_id);
                        rank_id++;
                        break;
                    }
                }
            }
        }
    }
}

/**
 * Schedule and execute tasks.
 * Within a single tick, handle events first and then schedule tasks.
 */
int scheduler() {
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
                handle_task_release_event(e, cur_tick);
                break;
            case COMMUNICATION_FINISHED:
                handle_communication_finish_event(e, cur_tick);
                break;
            default:
                assert(0 && "Unknown event type\n");
            }
        } while (eventq.size() > 0 && eventq.top().time == cur_tick);
        schedule(cur_tick);
    }
    return total_ticks;
}

void print_metrics(int total_ticks) {
    printf("--------------------------------------------------------------------------------\n");
    printf("%10s %15s %15s %15s\n", "CPU", "RUNNING_TIME", "TOTAL_TIME", "UTILIZATION");
    for (int i = 1; i <= nexecutors; ++i) {
        for (int j = 1; j <= executors[i].ncpu; ++j) {
            Cpu *const cpu = &executors[i].cpus[j];
            const int running_ticks = cpu->running_time;
            printf("%8d.%d %15d %15d %15f     ", i, j, running_ticks, total_ticks, running_ticks * 1.0 / total_ticks);
            const int runned_tasks = cpu->task_record.size();
            for (int j = 0; j < runned_tasks; ++j) {
                printf(" %d%c", cpu->task_record.front(), j == runned_tasks - 1 ? '\0' : ',');
                cpu->task_record.pop();
            }
            printf("\n");
        }
    }
}

void gen_mermaid_output(void) {
    FILE *f = fopen("./mermaid_out.mmd", "w");
    FILE *mf = fopen("./markdown_table.md", "w");
    fprintf(f, "gantt\ntitle 任务调度图\n");
    fprintf(f, "\tdateFormat s\n");
    fprintf(f, "\taxisFormat %%S\n");

    fprintf(mf, "|处理器|c1|c2|H|硬件面积|BUS|\n");
    fprintf(mf, "|---|---|---|---|---|---|\n");
    fprintf(mf, "|分配方案|-|-|-|-|-|\n");
    fprintf(mf, "|最大执行时间|-|-|-|-|-|\n");
    fprintf(mf, "|统计数据|-|-|-|-|-|\n");
    fprintf(mf, "|使用效率|-|-|-|-|-|\n");
    fprintf(mf, "\n");

    // all kinds of cpu
    for (int i = 1; i <= nexecutors; ++i) {
        const Executor *const e = &executors[i];
        for (int j = 1; j <= e->ncpu; ++j) {
            fprintf(f, "\tsection CPU %d.%d\n", i, j);
            fprintf(mf, "CPU %d.%d\n", i, j);
            for (int k = 1; k <= nalltasks; ++k) {
                if (tasks[k].running_executor == i &&
                    tasks[k].running_cpu == j) {
                    int stime = tasks[k].start_tick;
                    int ftime = stime + tasks[k].exec_time;
                    fprintf(f, "\ttask%d :done, des1, %02d, %02d\n", k, stime, ftime);
                    fprintf(mf, "T%d(%d->%d)<br>", k, stime, ftime);
                }
            }
            fprintf(mf, "\n");
        }
    }

    fprintf(f, "\tsection BUS\n");
    fprintf(mf, "BUS\n");
    // bus communication
    fprintf(f, "%s", comm_buf);
    fprintf(mf, "%s", md_comm_buf);

    fclose(f);
    fclose(mf);
}

int main(int argc, char **argv) {
    FILE* f;
    int total_ticks;

    if (argc == 1) {
        printf("usage: ./a.out [file name]\n");
        return 1;
    }
    if ((f = fopen(argv[1], "r")) == NULL) {
        printf("Failed to open file %s\n", argv[1]);
        return 1;
    }
    read_input(f);
    fclose(f);

    init_tasks();
    calc_priorities();
    sort_tasks();
    print_rankings();
    total_ticks = scheduler();
    print_metrics(total_ticks);
    gen_mermaid_output();
    return 0;
}
