#include <stdio.h>
#include <vector>
#include <list>
#include <map>
#include <set>
using namespace std;

#define MAX_NODES 50

int nodes_num;
vector<int> partition1;
vector<int> partition2;
double w[MAX_NODES][MAX_NODES];

void get_input(FILE *f)
{
    fscanf(f, "%d", &nodes_num);
    for (int i = 1; i <= nodes_num; ++i) {
        for (int j = 1; j <= nodes_num; ++j) {
            fscanf(f, "%lf", &w[i][j]);
        }
    }
}

double calc_cost(void)
{
    double res = 0;
    for (int a : partition1) {
        for (int b : partition2) {
            res += w[a][b];
        }
    }
    return res;
}

void print_output(void)
{
    int cnt = nodes_num >> 1;
    printf("partition 1:");
    for (int a : partition1) {
        printf(" %d", a);
    }
    printf("\n");
    printf("partition 2:");
    for (int b : partition2) {
        printf(" %d", b);
    }
    printf("\n");
    // print total cost
    printf("total cost: %lf\n", calc_cost());
}

void init_partition(unsigned int seed)
{
    srand(seed);
    int cnt = nodes_num >> 1;
    set<int> random_nums;
    for (int i = 1; i <= cnt; ++i) {
        int r = rand() % 8 + 1;
        if (!random_nums.count(r)) {
            random_nums.insert(r);
        } else {
            --i;
        }
    }
    for (int i = 1; i <= nodes_num; ++i)
        if (!random_nums.count(i))
            partition1.push_back(i);
    for (int e : random_nums)
        partition2.push_back(e);

    printf("init_partition: \n");
    for (int a : partition1)
        printf("%d ", a);
    printf("\n");
    for (int b : partition2)
        printf("%d ", b);
    printf("\n");
}

/*
 * D = Sum(external link) - Sum(internal link)
 */
double calc_difference(vector<int> &p1, int i, vector<int> &p2)
{
    double diff = 0;
    int a = p1[i];
    for (int b : p2) {
        diff += w[a][b];
    }
    for (int b : p1) {
        diff -= w[a][b];
    }
    return diff;
}

void kl(void)
{
    bool run = true;
    int n;

    while (run) {
        vector<double> g;
        vector<pair<int, int>> rec;

        n = nodes_num >> 1;
        for (int i = 1; i <= n; ++i) {
            double max_g = INT32_MIN;
            int resi = 0;
            int resj = 0;
            int size = partition1.size();
            vector<double> d1;
            vector<double> d2;

            // calc difference
            for (int j = 0; j < size; ++j) {
                d1.push_back(calc_difference(partition1, j, partition2));
                d2.push_back(calc_difference(partition2, j, partition1));
            }

            // calc g
            for (int j = 0; j < size; ++j) {
                for (int k = 0; k < size; ++k) {
                    double g = d1[j] + d2[k] - 2 * w[partition1[j]][partition2[k]];
                    if (g > max_g) {
                        max_g = g;
                        resi = j;
                        resj = k;
                    }
                }
            }

            g.push_back(max_g);
            rec.push_back(pair<int,int>(partition1[resi], partition2[resj]));
            partition1.erase(partition1.begin() + resi);
            partition2.erase(partition2.begin() + resj);
        }

        // find maximum prefix sum of g
        int res = 0;
        double g_max = g[0];
        for (int i = 1; i < n; ++i) {
            g[i] += g[i-1];
            if (g[i] > g_max) {
                res = i;
                g_max = g[i];
            }
        }

        // no more gain, just return
        if (g_max <= 0) {
            res = -1;
            run = false;
        }

        // exchange
        for (int i = 0; i < n; ++i) {
            int a = rec[i].first;
            int b = rec[i].second;
            if (i <= res) {
                partition1.push_back(b);
                partition2.push_back(a);
            } else {
                partition1.push_back(a);
                partition2.push_back(b);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    FILE *f;

    if (argc != 2) {
        printf("./kl [input file]\n");
        return -1;
    }

    if ((f = fopen(argv[1], "r")) == NULL) {
        printf("Unable to read file %s\n", argv[1]);
        return -1;
    }

    get_input(f);

    for (int i = 1; i <= 100; ++i) {
        partition1.clear();
        partition2.clear();
        init_partition(i);
        kl();
        print_output();
        printf("\n");
    }

    fclose(f);

    return 0;
}
