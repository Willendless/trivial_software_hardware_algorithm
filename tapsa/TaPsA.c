#include <stdio.h>
#include <stdlib.h>

#define MAX_TASKS 50
#define MAX(a1,a2) (a1>=a2)?(a1):(a2)

int n;
int g[MAX_TASKS][MAX_TASKS];
int exec_times[MAX_TASKS];
int release_times[MAX_TASKS];
int priorities[MAX_TASKS];
int rankings[MAX_TASKS];
int traverse[MAX_TASKS];

void read_input(FILE *f) {
    int n1, n2;
    fscanf(f, "%d", &n);
    for (int i = 1; i <= n; ++i) {
        fscanf(f, "%d %d", &exec_times[i], &release_times[i]);
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
            max_child_priority = MAX(max_child_priority, priorities[i]);
            childs++;
        }
    }
    priorities[node] = exec_times[node] + childs + max_child_priority;
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
    const int p1 = priorities[n1];
    const int p2 = priorities[n2];
    if (p1 != p2) return (-1) * (p1 - p2);
    const int rtime1 = release_times[n1];
    const int rtime2 = release_times[n2];
    if (rtime1 != rtime2) return rtime1 - rtime2;
    const int etime1 = exec_times[n1];
    const int etime2 = exec_times[n2];
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
    printf("%10s %10s %15s %15s\n", "TASKS", "PRIORITY", "RELEASE_TIME", "EXECUTION_TIME");
    for (int i = 1; i <= n; ++i) {
        const int task = rankings[i];
        printf("%10d %10d %15d %15d\n", task, priorities[task], release_times[task], exec_times[task]);
    }
    printf("------------------------------------------------------------\n");
    for (int i = 1; i <= n; ++i) printf("%d %-2s", rankings[i], i == n? "" : ">");
    printf("\n");
}

int main(int argc, char **argv) {
    FILE* f;
    if (argc == 1) {
        printf("usage: TaPsA [file name]\n");
        return 1;
    }
    if ((f = fopen(argv[1], "r")) == NULL) {
        printf("Failed to open file %s\n", argv[1]);
        return 1;
    }
    read_input(f);
    calc_priorities();
    sort_tasks();
    print_rankings();
    fclose(f);
    return 0;
}
