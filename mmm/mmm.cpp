#include <stdio.h>
#include <algorithm>
#include <stdlib.h>
#include <set>
#include <vector>
#include <algorithm>
using namespace std;

#define MAX_TASKS 50

int max_cost;
int task_nums;
int module_nums;
int module_size_max;
int module_size_min;
double c[MAX_TASKS][MAX_TASKS];
vector<set<int>> modules;
vector<vector<set<int>>> ans;

typedef bool (*mergable)(const set<int> &, const set<int> &, const int);

#define MAX(x, y) ((x)>(y)?(x):(y))

void get_input(FILE * f)
{
    int i, j;
    double cost;
    fscanf(f, "%d%d%d%d", &task_nums, &module_nums, &module_size_max, &module_size_min);
    while (fscanf(f, "%d%d%lf", &i, &j, &cost) != EOF) {
        c[i][j] = cost;
        c[j][i] = cost;
        max_cost = MAX(max_cost, cost);
    }
}

void init(void)
{
    const int size = modules.size();
    if (size) {
        modules.clear();
    } 

    for (int i = 1; i <= task_nums; ++i) {
        set<int> s;
        s.insert(i);
        modules.push_back(s);
    }
}

void print_tuples(vector<set<int>> &ms, int modules_cnt, int cost)
{
    printf("<%4d, %4d, {", cost, modules_cnt);
    const int size = ms.size();
    for (set<int> s : modules) {
        printf("{");
        for (int t : s) {
            printf("%d,", t);
        }
        printf("}");
    }
    printf("}>\n");
}

void store_ans(void)
{
    vector<set<int>> s;
    for (set<int> m : modules) {
        s.push_back(m);
    }
    ans.push_back(s);
}

double calc_cost(vector<set<int>> &ms)
{
    double cost = 0;
    const int size = ms.size();
    for (int i = 0; i < size - 1; ++i) {
        for (int j = i + 1; j < size; ++j) {
            for (int a : ms[i]) {
                for (int b : ms[j]) {
                    cost += c[a][b];
                }
            }
        }
    }
    return cost;
}

void print_ans(void)
{
    int size = ans.size();
    double min_cost = 1000000;
    int ans_index = 0;
    int cost;
    for (int i = 0; i < size; ++i) {
        if ((cost = calc_cost(ans[i])) < min_cost) {
            min_cost = cost;
            ans_index = i;
        }
    }

    for (set<int> &m : ans[ans_index]) {
        printf("{");
        for (int t : m) {
            printf("%d,", t);
        }
        printf("}");
    }
    printf("\n------------------------------------------\n");
    printf("total cost: %lf\n", min_cost);
}


void mmm(mergable fun, int seed)
{
    srand(seed);
    int modules_cnt = task_nums; 
    int cost_limit = max_cost + 1;

    // print_tuples(modules_cnt, cost_limit);
    --cost_limit;

    do {
        vector<pair<int, int>> rec;

        for (int i = 0; i < modules_cnt - 1; ++i) {
            for (int j = i + 1; j < modules_cnt; ++j) {
                // max size restriction
                if (modules[i].size() +
                    modules[j].size() > module_size_max)
                    continue;
                
                // min size restriction
                if (modules_cnt - 1 == module_nums) {
                    bool is_continue = false;
                    for (int k = 0; k < modules_cnt; ++k) {
                        if (k != j && k != i && modules[k].size() < module_size_min) {
                            is_continue = true;
                        }
                    }
                    if (is_continue)
                        continue;
                }

                if (fun(modules[i], modules[j], cost_limit)) {
                    rec.push_back(pair<int, int>(i, j));
                }
            }

            // unable to merge
            int candidates = rec.size();
            if (!candidates) continue;

            int k = rand() % candidates;
            int a = rec[k].first;
            int b = rec[k].second;
            modules[a].insert(modules[b].begin(), modules[b].end());
            modules[b].clear();
            modules.erase(modules.begin() + b);
            rec.clear();

            --modules_cnt;

            if (modules_cnt == module_nums)
                break;
        }
        // print_tuples(modules_cnt, cost_limit);
        --cost_limit;
        if (cost_limit < -1)
            break;
        else
            store_ans();

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
    double cost_sum = 0;
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
    get_input(f);
    for (int i = 1; i <= 1000; ++i) {
        init();
        mmm(link_funcs[fun_index], i);
    }
    print_ans();
    fclose(f);
    return 0;
}
