// time_shm.c : This file contains the 'main' function. Program execution begins and ends there.
// Group: Group 1
// Names: Baser Abrahim, Yuxuan(Jack) He, Michael Rueter, Bryant Hernandez, HuuNgoc Nguyen
// Course/Section: CPSC351 - Section 18102
// Assignment: Programming Assignment 2 - IPC

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[]) {
    // Check if a command was provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    // 1. Create a shared memory region
    const char* shm_name = "/shm_elapsed_time";
    const size_t shm_size = sizeof(struct timeval);

    // Open the shared memory object
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return 1;
    }

    // Configure the size of the shared memory object
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("ftruncate failed");
        shm_unlink(shm_name);
        return 1;
    }

    // Map the shared memory region into the process's address space
    struct timeval* shared_time = (struct timeval*)mmap(
        NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0
    );

    if (shared_time == MAP_FAILED) {
        perror("mmap failed");
        shm_unlink(shm_name);
        return 1;
    }

    // 2. Use fork() to create the child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        munmap(shared_time, shm_size);
        shm_unlink(shm_name);
        return 1;
    }

    if (pid == 0) {
        // --- CHILD PROCESS ---

        // 3. Call gettimeofday() to record the starting timestamp
        struct timeval start;
        gettimeofday(&start, NULL);

        // 4. Store the starting struct timeval in the shared-memory region
        *shared_time = start;

        // 5. Use execvp() to execute the command provided on the command line
        execvp(argv[1], &argv[1]);

        // If execvp returns, an error occurred
        perror("execvp failed");
        exit(1);
    }
    else {
        // --- PARENT PROCESS ---

        // 6. The parent must wait for the child process to terminate
        int status;
        waitpid(pid, &status, 0);

        // 7. After the child terminates, obtain the ending timestamp
        struct timeval end;
        gettimeofday(&end, NULL);

        // 8. Read starting timestamp from shared memory and calculate elapsed time
        struct timeval start = *shared_time;

        double elapsed_seconds = (end.tv_sec - start.tv_sec) +
            (end.tv_usec - start.tv_usec) / 1000000.0;

        // 9. Print the elapsed time in seconds with six digits after the decimal point
        // "%.6f" forces 6 decimal spots
        printf("Elapsed time: %.6f seconds\n", elapsed_seconds);

        // 10. Clean up the shared-memory resource before the program exits
        munmap(shared_time, shm_size);
        close(shm_fd);
        shm_unlink(shm_name);
    }

    return 0;
}

