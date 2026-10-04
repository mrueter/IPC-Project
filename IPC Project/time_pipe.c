// time_shm.c : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[]) {
    // Check if a command was provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    // 1. Create a pipe() before calling fork()
    // fd[0] is for reading, fd[1] is for writing
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe failed");
        return 1;
    }

    // 2. Call fork() to create the child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        close(fd[0]);
        close(fd[1]);
        return 1;
    }

    if (pid == 0) {
        // --- CHILD PROCESS ---

        // 9. Close unused pipe descriptors appropriately
        // The child only writes to the pipe, so close the read end
        close(fd[0]);

        // 3. Call gettimeofday() to record the starting timestamp
        struct timeval start;
        gettimeofday(&start, NULL);

        // 4. Write the starting struct timeval to the pipe
        if (write(fd[1], &start, sizeof(struct timeval)) == -1) {
            perror("child write to pipe failed");
            close(fd[1]);
            exit(1);
        }

        // Close the write end now that we are done sending data
        close(fd[1]);

        // 5. Use execvp() to execute the command provided on the command line
        execvp(argv[1], &argv[1]);

        // If execvp returns, an error occurred
        perror("execvp failed");
        exit(1);
    }
    else {
        // --- PARENT PROCESS ---

        // 9. Close unused pipe descriptors appropriately
        // The parent only reads from the pipe, so close the write end immediately
        close(fd[1]);

        // 6. The parent must wait for the child process to terminate
        int status;
        waitpid(pid, &status, 0);

        // 7. After the child terminates, call gettimeofday() to obtain the ending timestamp
        struct timeval end;
        gettimeofday(&end, NULL);

        // 8. Read the starting timestamp from the pipe
        struct timeval start;
        if (read(fd[0], &start, sizeof(struct timeval)) <= 0) {
            perror("parent read from pipe failed");
            close(fd[0]);
            return 1;
        }

        // Close the read end now that we are done receiving data
        close(fd[0]);

        // Calculate the elapsed time
        double elapsed_seconds = (end.tv_sec - start.tv_sec) +
            (end.tv_usec - start.tv_usec) / 1000000.0;

        // 10. Print the elapsed time in seconds with six digits after the decimal point
        printf("Elapsed time: %.6f seconds\n", elapsed_seconds);
    }

    return 0;
}
