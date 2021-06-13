#include <stdio.h>
#include <string.h>

#define MAX_TASKS 50
#define MAX_PROPERTIES 50
#define PROPERITY_LEN 100

int task_nums;
int tasks[MAX_TASKS];
int property_nums;
char cost[MAX_PROPERTIES][2][MAX_TASKS][PROPERITY_LEN];
char properties[MAX_PROPERTIES][PROPERITY_LEN];

int main(int argc, char *argv[])
{
    FILE *f;
    if (argc != 2) {
        printf("Usage: ./a.out [input file]");
        return -1;
    }

    if ((f = fopen(argv[1], "r")) == NULL) {
        printf("Failed to open input file");
        return -1;
    }

    char *property_name;
    fscanf(f, "%d%d", &task_nums, &property_nums);

    for (int i = 1; i <= property_nums; ++i) {
        // read property name
        fscanf(f, "%s", properties[i]);
        // read software cost
        for (int j = 1; j <= task_nums; ++j)
            fscanf(f, "%s", cost[i][0][j]);
        // read hardware cost
        for (int j = 1; j <= task_nums; ++j)
            fscanf(f, "%s", cost[i][1][j]);
    }
    fclose(f);

    int overall_cnt = 0;
    int module_task_cnt;
    int module_cnt = 1;
    char ofname[100];
    char s[100 + 1];
    do {
        int tasks[MAX_TASKS];
        printf("输入模块任务数：");
        scanf("%d", &module_task_cnt);
        overall_cnt += module_task_cnt;
        printf("输入任务索引：");
        for (int i = 1; i <= module_task_cnt; ++i)
            scanf("%d", &tasks[i]);
        
        sprintf(ofname, "lingo_out%d.txt", module_cnt);


        FILE *outf = fopen(ofname, "w");
        fprintf(outf, "model:\n");

        for (int i = 1; i <= property_nums; ++i) {
            fprintf(outf, "%s = ", properties[i]);
            // software
            for (int j = 1; j <= module_task_cnt; ++j) {
                fprintf(outf, "x%d0 * %s + ", tasks[j], cost[i][0][tasks[j]]);
            }
            // hardware
            for (int j = 1; j <= module_task_cnt; ++j) {
                fprintf(outf, "x%d1 * %s %s", tasks[j], cost[i][1][tasks[j]],
                        j == module_task_cnt? "" : "+");
            }
            fprintf(outf, ";\n");
        }
        for (int i = 1; i <= module_task_cnt; ++i) {
            fprintf(outf, "x%d0 + x%d1 = 1;\n", tasks[i], tasks[i]);
        }
        fprintf(outf, "\n");

        for (int i = 1; i <= module_task_cnt; ++i) {
            fprintf(outf, "@bin(x%d0); @bin(x%d1);\n", tasks[i], tasks[i]);
        }
        
        fprintf(outf, "end\n");
        fclose(outf);

        module_cnt++;
    } while (overall_cnt != task_nums);
    return 0;
}
