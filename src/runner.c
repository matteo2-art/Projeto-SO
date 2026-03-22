#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_CMD 512

/* ── EXECUTION ENGINE (The child process brain) ──────────────────────────────────────── */

static void execute_command(const char *command) {
    if (!command) return;

    // 1. Create a safe copy of the command string
    char cmd_copy[MAX_CMD];
    strncpy(cmd_copy, command, MAX_CMD - 1);
    cmd_copy[MAX_CMD - 1] = '\0'; // Always guarantee the string ends properly

    char *args[64]; // Array to hold the chopped up words (e.g., ["echo", "hello", NULL])
    int argc = 0;

    // 2. Chop the string into pieces using spaces as the delimiter
    char *token = strtok(cmd_copy, " ");
    while (token != NULL && argc < 63) {
        args[argc++] = token; // Save the word into our array
        token = strtok(NULL, " "); // Grab the next word
    }
    args[argc] = NULL; // execvp requires the last item in the array to be NULL!

    if (argc == 0) return; // If the user just typed spaces, do nothing

    // 3. Clone the process! 
    // From this line forward, TWO identical programs are running simultaneously.
    pid_t pid = fork();

    if (pid < 0) {
        // pid is negative: The OS failed to create a clone (maybe out of memory)
        perror("fork");
        return;
    }

    if (pid == 0) {
        // pid is 0: We are inside the CHILD process.
        // execvp completely replaces this child process with the new program (e.g., "ls" or "echo").
        execvp(args[0], args);
        
        // If execvp succeeds, the child is destroyed and replaced. 
        // Therefore, these next two lines ONLY run if execvp fails (e.g., typo like "echho").
        perror("execvp");
        _exit(1); 
    } else {
        // pid is > 0: We are in the PARENT process. 'pid' is the ID of our child.
        int status;
        
        // Put the parent to sleep until the specific child process finishes executing
        waitpid(pid, &status, 0);
        
        // WIFEXITED checks if the child crashed or finished normally
        if (!WIFEXITED(status)) perror("waitpid: child exited abnormally");
    }
}

/* ── HELPERS ──────────────────────────────────────── */


/* * This function glues the argv array back into a single string.
 * Example: argv[3]="echo", argv[4]="hello"  --> buf="echo hello"
 */
static int join_args(char *buf, int buf_size, char *argv[], int start, int argc) {
    buf[0] = '\0'; // Start with an empty string
    int offset = 0; // Keeps track of where we are currently writing in the buffer

    // Loop through the arguments starting from the command name
    for (int i = start; i < argc; i++) {
        int token_len = strlen(argv[i]);

        // Security check
        // +2 is for the space we add, and the \0 terminator at the end
        if (offset + token_len + 2 > buf_size) {
            write(2, "error: command too long\n", 24); 
            return -1;
        }

        // Copy the word into the buffer exactly where we left off
        strncpy(buf + offset, argv[i], buf_size - offset - 1);
        offset += token_len;

        // If this isn't the last word, add a space after it
        if (i < argc - 1) {
            buf[offset++] = ' ';
        }
    }

    // Seal the final string with a null terminator
    buf[offset] = '\0';
    return 0;
}

/* ── MAIN (The Runner's entry point) ──────────────────────────────────────── */

int main(int argc, char *argv[]) {

    // If the user just types "./runner" with no arguments, show them the manual
    if (argc < 2) {
        write(2, "usage: runner -e <user-id> <command> [args...]\n", 47);
        write(2, "       runner -c\n", 17);
        write(2, "       runner -s\n", 17);
        return 1; // Exit with error code 1
    }

    /* ── option: -e (Execute a command) ── */
    if (strcmp(argv[1], "-e") == 0) { // strcmp returns 0 if the strings are identical

        // We need at least 4 things: ./runner, -e, user_id, and the command itself
        if (argc < 4) {
            write(2, "usage: runner -e <user-id> <command> [args...]\n", 47);
            return 1;
        }

        // Convert the user_id from text (like "1") into an actual integer [cite: 41]
        int user_id = atoi(argv[2]);
        if (user_id <= 0) {
            write(2, "error: user-id must be a positive integer\n", 42);
            return 1;
        }

        // Create an empty string, then use our helper to glue all the command words into it
        char command[MAX_CMD];
        if (join_args(command, MAX_CMD, argv, 3, argc) < 0) return 1;

        /* =========================================================
           TODO (IPC phase): Send request to Controller's pipe here!
           ========================================================= */

        // Spec requirement: Notify the user that the command was submitted [cite: 43]
        // write(1, ...) prints to Standard Output (File Descriptor 1)
        write(1, "[runner] command submitted\n", 27);

        /* =========================================================
           TODO (IPC phase): Pause the runner and wait for the 
           Controller to send the "MSG_GO_AHEAD" through the pipe!
           ========================================================= */
           
        // Once the Controller says YES, we execute:
        write(1, "[runner] executing command...\n", 30);
        execute_command(command);
        write(1, "[runner] command finished\n", 26);

        /* =========================================================
           TODO (IPC phase): Tell the Controller the job is done!
           ========================================================= */

        return 0; // Success!
    }

    /* ── option: -c (Consult running and waiting jobs) ── */
    if (strcmp(argv[1], "-c") == 0) {

        /* TODO (IPC phase): send MSG_QUERY to controller and print the response */
        write(1, "[runner] -c not yet implemented (waiting for IPC)\n", 50);
        return 0;
    }

    /* ── option: -s (Shutdown controller) ── */
    if (strcmp(argv[1], "-s") == 0) {

        /* TODO (IPC phase): send MSG_SHUTDOWN to controller */
        write(1, "[runner] -s not yet implemented (waiting for IPC)\n", 50);
        return 0;
    }

    // If they typed something weird like "./runner -x"
    write(2, "error: unknown option\n", 22);
    write(2, "usage: runner -e <user-id> <command> [args...]\n", 47);
    write(2, "       runner -c\n", 17);
    write(2, "       runner -s\n", 17);
    return 1;
}