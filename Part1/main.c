#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

#define MAX_HIDDEN_KEYS 80
#define MAX_ARRAY_SIZE 1000000

struct Metrics {
    int max;
    float avg;
    int hiddenKeys[MAX_HIDDEN_KEYS];
    int foundKeys;
    pid_t pid;
    pid_t ppid;
    int returnArg;
};

void generate_input_file(const char *filename, int L) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("File open failed");
        exit(1);
    }

    srand(time(NULL));
    int hidden_key_positions[MAX_HIDDEN_KEYS];
    for (int i = 0; i < MAX_HIDDEN_KEYS; i++) {
        hidden_key_positions[i] = rand() % L;
    }

    for (int i = 0; i < L; i++) {
        int is_hidden = 0;
        for (int j = 0; j < MAX_HIDDEN_KEYS; j++) {
            if (i == hidden_key_positions[j]) {
                fprintf(file, "%d\n", -(j + 1));
                is_hidden = 1;
                break;
            }
        }
        if (!is_hidden) {
            fprintf(file, "%d\n", rand() % 100 + 1);
        }
    }
    fclose(file);
}

void read_file_into_array(const char *filename, int *arr, int L) {
    FILE *file = fopen(filename, "r");
    for (int i = 0; i < L; i++) {
        fscanf(file, "%d", &arr[i]);
    }
    fclose(file);
}

struct Metrics process_segment(int *arr, int start, int end, int returnArg) {
    struct Metrics result = {.max = -99999, .avg = 0.0, .foundKeys = 0};
    int sum = 0;
    result.pid = getpid();
    result.ppid = getppid();
    result.returnArg = returnArg;

    for (int i = start; i < end; i++) {
        if (arr[i] > result.max) result.max = arr[i];
        sum += arr[i];
        if (arr[i] < 0 && result.foundKeys < MAX_HIDDEN_KEYS) {
            result.hiddenKeys[result.foundKeys++] = i;
        }
    }
    result.avg = (float)sum / (end - start);
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <L> <H> <PN>\n", argv[0]);
        exit(1);
    }

    int L = atoi(argv[1]);
    int H = atoi(argv[2]);
    int PN = atoi(argv[3]);

    const char *filename = "input.txt";
    const char *outputfile = "results.txt";
    FILE *out = fopen(outputfile, "w");
    if (!out) {
        perror("Failed to open results.txt");
        exit(1);
    }

    // Start timing
    clock_t start_time = clock();

    generate_input_file(filename, L);

    int *arr = malloc(L * sizeof(int));
    read_file_into_array(filename, arr, L);

    int chunk_size = L / PN;
    int pipes[2];
    pipe(pipes);

    int total_hidden_reported = 0;
    int global_max = -99999;
    float total_sum = 0;

    pid_t root_pid = getpid();

    for (int i = 0; i < PN; i++) {
        if (i != 0) break; // ensure only one parent continues the loop

        pid_t pid = fork();

        if (pid == 0) {
            for (int j = 1; j < PN; j++) {
                if (i + j >= PN) break;
                pid_t child = fork();
                if (child == 0) {
                    i += j;
                } else {
                    break;
                }
            }

            int start = i * chunk_size;
            int end = (i == PN - 1) ? L : start + chunk_size;
            struct Metrics result = process_segment(arr, start, end, i + 1);

            write(pipes[1], &result, sizeof(result));
            sleep(10);
            exit(i + 1);
        }
    }

    // Only root collects and prints
    if (getpid() == root_pid) {
        system("pstree -p");

        for (int i = 0; i < PN; i++) {
            struct Metrics result;
            read(pipes[0], &result, sizeof(result));

            fprintf(out, "Hi I’m process %d with return arg %d and my parent is %d.\n",
                    result.pid, result.returnArg, result.ppid);

            for (int j = 0; j < result.foundKeys && total_hidden_reported < H; j++) {
                fprintf(out, "Hi I am process %d with return arg %d and I found the hidden key in position A[%d].\n",
                        result.pid, result.returnArg, result.hiddenKeys[j]);
                total_hidden_reported++;
            }

            fprintf(out, "Max=%d, Avg =%.0f\n", result.max, result.avg);

            if (result.max > global_max) global_max = result.max;
            total_sum += result.avg * chunk_size;

            int status;
            wait(&status);
        }

        float global_avg = total_sum / L;
        clock_t end_time = clock();
        double exec_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

        fprintf(out, "Max=%d, Avg=%.2f\n", global_max, global_avg);
        fprintf(out, "Execution Time: %.2f seconds\n", exec_time);

        fclose(out);
        free(arr);
    }

    return 0;
}
