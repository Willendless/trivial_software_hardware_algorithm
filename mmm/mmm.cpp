#include <stdio.h>
#include <algorithm>
#include <stdlib.h>
#include <set>
using namespace std;

#define MAX_TASKS 50

int max_cost;
int task_nums;
int module_nums;
int module_size_limit;
double c[MAX_TASKS][MAX_TASKS];
set<int> modules[MAX_TASKS];

typedef bool (*mergable)(const set<int> &, const set<int> &, const int);

#define MAX(x, y) ((x)>(y)?(x):(y))

void get_input(FILE * f)
{
    int i, j;
    double cost;
    fscanf(f, "%d%d%d", &task_nums, &module_nums, &module_size_limit);
    while (fscanf(f, "%d%d%lf", &i, &j, &cost) != EOF) {
        c[i][j] = cost;
        c[j][i] = cost;
        max_cost = MAX(max_cost, cost);
    }
}

void init(FILE *f)
{
    get_input(f);
    for (int i = 1; i <= task_nums; ++i) {
        modules[i] = set<int>();
        modules[i].insert(i);
    }
}

void print_tuples(int modules_cnt, int cost)
{
    printf("<%4d, %4d, {", cost, modules_cnt);
    for (int i = 1; i <= task_nums; ++i) {
        if (!modules[i].size())
            continue;

        printf("{");
        for (int t : modules[i]) {
            printf("%d,", t);
        }
        printf("}");
    }
    printf("}>\n");
}

void mmm(mergable fun)
{
    int modules_cnt = task_nums; 
    int cost_limit = max_cost + 1;

    print_tuples(modules_cnt, cost_limit);
    --cost_limit;

    do {
AGAIN:
        for (int i = 1; i < task_nums; ++i) {
            if (!modules[i].size())
                continue;

            for (int j = i + 1; j <= task_nums; ++j) {
                if (!modules[j].size())
                    continue;

                // each module has size limit
                if (modules[i].size() +
                    modules[j].size() > module_size_limit)
                    continue;

                if (fun(modules[i], modules[j], cost_limit)) {
                    for (int e : modules[j]) {
                        modules[i].insert(e);
                    }
                    modules[j].clear();
                    --modules_cnt;
                    goto AGAIN;
                }
            }
        }
        print_tuples(modules_cnt, cost_limit);
        --cost_limit;
    } while (modules_cnt != module_nums);
}

bool single_link(const set<int> &a, const set<int> &b, const int cost_limit)
{
    for (int n : a) {
        for (int i = 1; i <= task_nums; ++i) {
            if (b.count(i) && c[n][i] >= cost_limit) {
                return true;
            }
        }
    }
    return false;
}

bool compare_link(const set<int> &a, const set<int> &b, const int cost_limit)
{
    for (int n : a) {
        for (int i = 1; i <= task_nums; ++i) {
            if (b.count(i) && c[n][i] < cost_limit) {
                return false;
            }
        }
    }
    return true;
}

bool average_link(const set<int> &a, const set<int> &b, const int cost_limit)
{
    int cnt = 0;
    int64_t cost_sum = 0;
    for (int n : a) {
        for (int i = 1; i <= task_nums; ++i) {
            if (b.count(i)) {
                cost_sum += c[n][i];
                ++cnt;
            }
        }
    }
    return (cost_sum * 1.0) / (cnt * 1.0) >= cost_limit;
}

mergable link_funcs[] = {single_link, compare_link, average_link};

double calc_cost(void)
{
    double cost = 0;
    for (int i = 1; i < task_nums; ++i) {
        if (!modules[i].size())
            continue;
        for (int j = i + 1; j <= task_nums; ++j) {
            if (!modules[j].size())
                continue;
            for (int a : modules[i]) {
                for (int b : modules[j]) {
                    cost += c[a][b];
                }
            }
        }
    }
    return cost;
}

int main(int argc, char *argv[])
{
    FILE *f;
    int fun_index = 1;
    if (argc != 3) {
        printf("usage: MMM [file name] [link func index]\n");
        printf("  0: single link\n");
        printf("  1: compare link\n");
        printf("  2: average link\n");
        return -1;
    }
    if ((f = fopen(argv[1], "r")) == NULL) {
        printf("Failed to open file %s", argv[1]);
        return -1;
    }
    fun_index = atoi(argv[2]);
    fun_index = fun_index >= 3 ? 0 : fun_index;
    init(f);
    mmm(link_funcs[fun_index]);
    printf("------------------------------------------\n");
    printf("total cost: %lf\n", calc_cost());
    fclose(f);
    return 0;
}
