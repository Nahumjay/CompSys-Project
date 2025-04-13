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
    if (!file) {
        perror("Failed to open input file");
        exit(1);
    }
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

    clock_t start_time = clock();   // Start timing

    generate_input_file(filename, L);
    int *arr = malloc(L * sizeof(int));
    if (!arr) {
        perror("Memory allocation failed");
        fclose(out);
        exit(1);
    }
    
    read_file_into_array(filename, arr, L);

    int chunk_size = L / PN;
    int pipes[PN][2]; // Create a separate pipe for each child process
    pid_t pids[PN];   // Track child PIDs
    
    // Create pipes before forking
    for (int i = 0; i < PN; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe creation failed");
            free(arr);
            fclose(out);
            exit(1);
        }
    }

    // Fork child processes
    for (int i = 0; i < PN; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("Fork failed");
            free(arr);
            fclose(out);
            exit(1);
        }
        
        if (pids[i] == 0) { // Child process
            // Close all unneeded pipes
            for (int j = 0; j < PN; j++) {
                if (j != i) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }
            close(pipes[i][0]); // Close read end in child
            
            // Calculate segment boundaries
            int start = i * chunk_size;
            int end = (i == PN - 1) ? L : start + chunk_size;
            
            // Process the segment
            struct Metrics result = process_segment(arr, start, end, i + 1);
            
            // Send results to parent
            if (write(pipes[i][1], &result, sizeof(result)) != sizeof(result)) {
                perror("Write failed");
            }
            
            close(pipes[i][1]); // Close pipe
            free(arr);
            exit(i + 1); // Exit with return code
        }
    }
    
    // Parent process
    for (int i = 0; i < PN; i++) {
        close(pipes[i][1]); // Close write ends of all pipes
    }
    
    system("pstree -p"); // Show process tree
    
    int total_hidden_reported = 0;
    int global_max = -99999;
    float total_sum = 0;
    
   
    for (int i = 0; i < PN; i++) {  // Collect results from all children
        struct Metrics result;
        ssize_t r = read(pipes[i][0], &result, sizeof(result));
        
        if (r != sizeof(result)) {
            fprintf(out, "Error reading result from child %d\n", i + 1);
            continue;
        }
        
        fprintf(out, "Hi I'm process %d with return arg %d and my parent is %d.\n",
                result.pid, result.returnArg, result.ppid);
        
        for (int j = 0; j < result.foundKeys && total_hidden_reported < H; j++) {
            fprintf(out, "Hi I am process %d with return arg %d and I found the hidden key in position A[%d].\n",
                    result.pid, result.returnArg, result.hiddenKeys[j]);
            total_hidden_reported++;
        }
        
        fprintf(out, "Max=%d, Avg=%.0f\n", result.max, result.avg);
        
        if (result.max > global_max) global_max = result.max;
        total_sum += result.avg * chunk_size;
        
        
        close(pipes[i][0]); // Close read end of pipe
    }
    
    for (int i = 0; i < PN; i++) {  // Wait for all child processes to finish
        int status;
        waitpid(pids[i], &status, 0);
    }
    
    float global_avg = total_sum / L;
    clock_t end_time = clock();
    double exec_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    fprintf(out, "Max=%d, Avg=%.2f\n", global_max, global_avg);
    fprintf(out, "Execution Time: %.2f seconds\n", exec_time);
    
    fflush(out);
    fclose(out);
    free(arr);
    
    return 0;
}