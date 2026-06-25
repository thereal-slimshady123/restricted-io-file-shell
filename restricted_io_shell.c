/**
 * @file restricted_io_shell.c
 * @brief A custom Unix shell-like utility demonstrating restricted I/O redirection.
 *
 * This program implements a command-line interface (CLI) for file and log management
 * under strict constraints: NO direct file read() or write() system calls are allowed.
 * Instead, standard input (stdin, fd 0) and standard output (stdout, fd 1) are 
 * dynamically duplicated and redirected to target file descriptors using dup() and dup2().
 * File reads and writes are then performed using standard formatting I/O (scanf and printf).
 *
 * Key Concepts Demonstrated:
 *  1. Low-level Descriptor Duplication (dup & dup2)
 *  2. Process Sandbox and Isolation (getpid, mkdir, chdir)
 *  3. Dynamic Memory Allocation and Circular Buffers (malloc/free for LAST n lines)
 *  4. Stdin Buffer Management and Error Clearing (clearerr, buffer flushing)
 *  5. Unbuffered I/O streams for reliable pipe redirection (setvbuf)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * @brief Custom helper to format an integer to string.
 * Used to construct directory names without relying on complex string formatters.
 * 
 * @param text Output buffer
 * @param num Integer to convert
 */
void mysprintf(char* text, int num)
{
    int count = 0;
    if (num == 0) {
        text[count++] = '0';
    }
    while (num != 0) {
        int digit = num % 10;
        text[count++] = (char)(digit + '0');
        num /= 10;
    }
    text[count] = '\0';
    // Reverse string
    for (int i = 0; i < count / 2; i++) {
        char temp = text[i];
        text[i] = text[count - i - 1];
        text[count - i - 1] = temp;
    }
}

int main()
{
    // 0. Disable Buffering
    // Disable buffering on stdin and stdout to ensure descriptor redirection (dup2)
    // works seamlessly even when inputs/outputs are piped or redirected.
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    // 1. Process Sandbox Creation
    // Get unique process identifier (PID) to create a process-isolated sandbox
    int pid = (int)getpid();
    char folder_name[100] = "folder_";
    mysprintf(folder_name + 7, pid);
    
    // Create and move execution context into the sandbox folder
    if (mkdir(folder_name, 0777) != 0) {
        // It's acceptable if the directory already exists from a previous run
    }
    if (chdir(folder_name) != 0) {
        perror("Failed to change directory to sandbox");
        return 1;
    }

    // 2. Open Persistent Descriptors
    // Open content.txt and logs.txt with append write access
    int content_fd = open("content.txt", O_CREAT | O_WRONLY | O_APPEND, 0666);
    int log_fd     = open("logs.txt",     O_CREAT | O_WRONLY | O_APPEND, 0666);
    if (content_fd < 0 || log_fd < 0) {
        perror("Failed to initialize target files");
        return 1;
    }

    int flag = 1;
    while (flag) {
        printf("Enter command (INPUT, PRINT, FIRST n, LAST n, LOG n, STOP): ");

        char command[100];
        if (scanf("%99s", command) != 1) {
            // Handle EOF or read error gracefully
            break;
        }

        /* ----------------------------------------------------
         * INPUT Command: Append line of text to content.txt
         * ---------------------------------------------------- */
        if (strcmp(command, "INPUT") == 0) {
            printf("INPUT -> ");
            
            // Consume residual newline character from stdin buffer
            char temp;
            if (scanf("%c", &temp) != 1) {
                // Ignore or handle stream interruption
            }
            
            char input[1000];  
            if (scanf("%999[^\n]", input) != 1) {
                input[0] = '\0';
            }
            
            // Save original stdout descriptor (fd 1)
            int save_out = dup(1);
            
            // Redirect stdout to content.txt descriptor
            dup2(content_fd, 1);
            printf("%s\n", input);
            
            // Redirect stdout to logs.txt descriptor to log the operation
            dup2(log_fd, 1);
            printf("INPUT\n");
            
            // Restore stdout to console
            dup2(save_out, 1);
            close(save_out);
        }

        /* ----------------------------------------------------
         * PRINT Command: Print entire contents of content.txt
         * ---------------------------------------------------- */
        else if (strcmp(command, "PRINT") == 0) 
        {
            // Save original stdin descriptor (fd 0)
            int save_in = dup(0);
            int read_fd = open("content.txt", O_RDONLY);
            if (read_fd < 0) {
                perror("open content.txt for PRINT");
                dup2(save_in, 0);
                close(save_in);
                continue;
            }
            
            // Redirect stdin to read from content.txt
            dup2(read_fd, 0);
            close(read_fd);

            char c;
            // Read character by character from redirected stdin and print to stdout
            while (scanf("%c", &c) == 1) 
            {
                printf("%c", c);
            }
            
            // Restore stdin to console and clear EOF/error flags
            dup2(save_in, 0);
            close(save_in);
            clearerr(stdin);

            // Log operation
            int save_out = dup(1);
            dup2(log_fd, 1);
            printf("PRINT\n");
            dup2(save_out, 1);
            close(save_out);
        }
        
        /* ----------------------------------------------------
         * STOP Command: Log operation and terminate shell
         * ---------------------------------------------------- */
        else if (strcmp(command, "STOP") == 0) {
            int save_out = dup(1);
            dup2(log_fd, 1);
            printf("STOP\n");
            dup2(save_out, 1);
            close(save_out);
            flag = 0;
        }
        
        /* ----------------------------------------------------
         * FIRST n Command: Read first n lines of content.txt
         * ---------------------------------------------------- */
        else if(strcmp(command, "FIRST") == 0)
        {
            int n;
            if (scanf("%d", &n) != 1) {
                continue;
            }
            char temp_char;
            if (scanf("%c", &temp_char) != 1) {
                // Ignore
            }
            
            int save_in = dup(0);
            int read_fd = open("content.txt", O_RDONLY);
            if (read_fd < 0) {
                perror("open content.txt for FIRST");
                dup2(save_in, 0);
                close(save_in);
                continue;
            }
            dup2(read_fd, 0);
            close(read_fd);
            
            int line_count = 0;
            char c;
            // Read characters, tracking newline counts to output only n lines
            while (line_count < n && scanf("%c", &c) == 1) {
                printf("%c", c);
                if (c == '\n') {
                    line_count++;
                }
            }
            
            // Drain any leftover content in stdin stream
            char temp_buffer;
            while (scanf("%c", &temp_buffer) == 1) 
            {
                // do nothing
            }
            
            dup2(save_in, 0);
            close(save_in);
            clearerr(stdin);
            
            // Log operation
            int save_out = dup(1);
            dup2(log_fd, 1);
            printf("FIRST %d\n", n);
            dup2(save_out, 1);
            close(save_out);
        }
        
        /* ----------------------------------------------------
         * LAST n Command: Read last n lines of content.txt
         * Uses a sliding window circular buffer for memory efficiency.
         * ---------------------------------------------------- */
        else if(strcmp(command, "LAST") == 0)
        {
            int n;
            if (scanf("%d", &n) != 1) {
                continue;
            }
            char temp_char;
            if (scanf("%c", &temp_char) != 1) {
                // Ignore
            }
            
            int save_in = dup(0);
            int read_fd = open("content.txt", O_RDONLY);
            if (read_fd < 0) {
                perror("open content.txt for LAST");
                dup2(save_in, 0);
                close(save_in);
                continue;
            }
            dup2(read_fd, 0);
            close(read_fd);
            
            // Dynamic allocation of circular buffer containing n strings
            char **lines = malloc(n * sizeof(char*));
            for (int i = 0; i < n; i++) {
                lines[i] = malloc(1025 * sizeof(char));
            }
            
            int current_index = 0;
            int total_lines = 0;
            char line[1025];
            
            // Read lines and overwrite older lines in the circular buffer
            while (scanf("%999[^\n]", line) == 1) {
                strcpy(lines[current_index], line);
                current_index = (current_index + 1) % n;
                total_lines++;
                
                char bufc;
                if (scanf("%c", &bufc) != 1 || bufc != '\n') 
                {
                    break; 
                }
            }
            
            dup2(save_in, 0);
            close(save_in);
            clearerr(stdin);
            
            // Output lines from oldest to newest in circular buffer
            int lines_to_print = (total_lines < n) ? total_lines : n;
            int start_index = (total_lines <= n) ? 0 : current_index;
            
            for (int i = 0; i < lines_to_print; i++) {
                int index = (start_index + i) % n;
                printf("%s\n", lines[index]);
            }
            
            // Free dynamic memory
            for (int i = 0; i < n; i++) {
                free(lines[i]);
            }
            free(lines);
            
            // Log operation
            int save_out = dup(1);
            dup2(log_fd, 1);
            printf("LAST %d\n", n);
            dup2(save_out, 1);
            close(save_out);
        }
        
        /* ----------------------------------------------------
         * LOG n Command: Prints last n log actions from logs.txt
         * Note: Self-exclusionary (this command itself is not logged).
         * ---------------------------------------------------- */
        else if(strcmp(command, "LOG") == 0)
        {
            int n;
            if (scanf("%d", &n) != 1) {
                continue;
            }
            char temp_char;
            if (scanf("%c", &temp_char) != 1) {
                // Ignore
            }
            
            int save_in = dup(0);
            int read_fd = open("logs.txt", O_RDONLY);
            if (read_fd < 0) {
                perror("open logs.txt for LOG");
                dup2(save_in, 0);
                close(save_in);
                continue;
            }
            dup2(read_fd, 0);
            close(read_fd);
            
            // Circular buffer representation for logs
            char **log_lines = malloc(n * sizeof(char*));
            for (int i = 0; i < n; i++) {
                log_lines[i] = malloc(1000 * sizeof(char));
            }
            
            int current_index = 0;
            int total_lines = 0;
            char line[1024];
            while (scanf("%999[^\n]", line) == 1) {
                strcpy(log_lines[current_index], line);
                current_index = (current_index + 1) % n;
                total_lines++;
                
                char bufc;
                if (scanf("%c", &bufc) != 1 || bufc != '\n') 
                {
                    break; 
                }
            }
            
            dup2(save_in, 0);
            close(save_in);
            clearerr(stdin);
            
            int lines_to_print = (total_lines < n) ? total_lines : n;
            int start_index = (total_lines <= n) ? 0 : current_index;
            
            for (int i = 0; i < lines_to_print; i++) {
                int index = (start_index + i) % n;
                printf("%s\n", log_lines[index]);
            }
            
            for (int i = 0; i < n; i++) {
                free(log_lines[i]);
            }
            free(log_lines);
        }
        else {
            printf("Unknown command: %s\n", command);
        }
    }

    close(content_fd);
    close(log_fd);
    return 0;
}
