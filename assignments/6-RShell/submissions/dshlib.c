#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"
int free_cmd_list(command_list_t *cmd_lst);
int build_cmd_list(char *cmd_line, command_list_t *clist);

// Function to allocate memory for command buffer
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX * sizeof(char));
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }

    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_output = false;
    return OK;
}

// Function to free allocated command buffer
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }

    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    if (cmd_buff->input_file) {
        free(cmd_buff->input_file);
        cmd_buff->input_file = NULL;
    }
    if (cmd_buff->output_file) {
        free(cmd_buff->output_file);
        cmd_buff->output_file = NULL;
    }
    cmd_buff->append_output = false;
    return OK;
}

// Clear the command buffer
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    if (cmd_buff->input_file) {
        free(cmd_buff->input_file);
        cmd_buff->input_file = NULL;
    }
    if (cmd_buff->output_file) {
        free(cmd_buff->output_file);
        cmd_buff->output_file = NULL;
    }
    cmd_buff->append_output = false;
    return OK;
}

// Function to build the command buffer
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    while (isspace(*cmd_line)) {
        cmd_line++;
    }

    if (*cmd_line == '\0') {
        return WARN_NO_CMDS;
    }

    strcpy(cmd_buff->_cmd_buffer, cmd_line);
    cmd_buff->argc = 0;

    char *token = cmd_buff->_cmd_buffer;
    bool in_quotes = false;
    char *start = token;

    while (*token != '\0' && cmd_buff->argc < CMD_ARGV_MAX - 1) {
        if (*token == '"') {
            if (!in_quotes) {
                start = token + 1;
                in_quotes = true;
            } else {
                *token = '\0';
                cmd_buff->argv[cmd_buff->argc++] = start;
                in_quotes = false;
                start = token + 1;
            }
        } else if (!in_quotes && isspace(*token)) {
            if (start != token) {
                *token = '\0';
                cmd_buff->argv[cmd_buff->argc++] = start;
            }
            start = token + 1;
        } else if (!in_quotes && *token == '<') {
            *token = '\0';
            cmd_buff->argv[cmd_buff->argc++] = start;
            start = token + 1;
            while (isspace(*start)) start++;
            char *file_start = start;
            while (!isspace(*start) && *start != '\0') start++;
            *start = '\0';
            cmd_buff->input_file = strdup(file_start);
            start++;
        } else if (!in_quotes && *token == '>') {
            *token = '\0';
            cmd_buff->argv[cmd_buff->argc++] = start;
            start = token + 1;
            if (*start == '>') {
                cmd_buff->append_output = true;
                start++;
            }
            while (isspace(*start)) start++;
            char *file_start = start;
            while (!isspace(*start) && *start != '\0') start++;
            *start = '\0';
            cmd_buff->output_file = strdup(file_start);
            start++;
        }
        token++;
    }

    if (start < token && *start != '\0' && cmd_buff->argc < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[cmd_buff->argc++] = start;
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

// Function to match built-in commands like "exit" and "cd"
Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    }
    return BI_NOT_BI;
}

// Execute built-in commands like "exit" and "cd"
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd->argc == 0)
        return BI_NOT_BI;
    
    Built_In_Cmds type = match_command(cmd->argv[0]);

    switch (type) {
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;

        case BI_CMD_CD:
            if (cmd->argc > 1) {
                if (chdir(cmd->argv[1]) != 0) {
                    perror("cd");
                }
            }
            return BI_EXECUTED;

        default:
            return BI_NOT_BI;
    }
}

// Function to execute non-built-in commands
int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        fprintf(stderr, "Failed to create process for command '%s'\n", cmd->argv[0]);
        return ERR_EXEC_CMD;
    }

    if (pid == 0) {
        // Check if the input and output files are the same
        if (cmd->input_file && cmd->output_file && strcmp(cmd->input_file, cmd->output_file) == 0) {
            fprintf(stderr, "Cannot redirect input and output to the same file: %s\n", cmd->input_file);
            exit(1);
        }

        // Handle input redirection
        if (cmd->input_file) {
            int fd_in = open(cmd->input_file, O_RDONLY);
            if (fd_in < 0) {
                perror("open input file");
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        // Handle output redirection
        if (cmd->output_file) {
            int fd_out;
            if (cmd->append_output) {
                fd_out = open(cmd->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            } else {
                fd_out = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            if (fd_out < 0) {
                perror("open output file");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        // Execute the command
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return OK;
    }
}

// Function to execute a pipeline of commands
int exec_pipeline(command_list_t *clist) {
    if (clist->num == 0) {
        return WARN_NO_CMDS;
    }

    if (clist->num == 1) {
        Built_In_Cmds bi_rc = exec_built_in_cmd(&(clist->commands[0]));
        if (bi_rc == BI_CMD_EXIT) {
            return OK_EXIT;
        } else if (bi_rc == BI_EXECUTED) {
            return OK;
        }
        return exec_cmd(&(clist->commands[0]));
    }

    int pipes[CMD_MAX - 1][2]; 
    pid_t pids[CMD_MAX];       

    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < clist->num; i++) {
        cmd_buff_t *cmd = &(clist->commands[i]);

        // Check for built-in commands
        Built_In_Cmds bi_rc = exec_built_in_cmd(cmd);

        if (i == 0) {
            if (bi_rc == BI_CMD_EXIT) {
                for (int j = 0; j < clist->num - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                return OK_EXIT;
            } else if (bi_rc == BI_EXECUTED) {
                printf("error: Built-in commands cannot be used in pipelines\n");

                for (int j = 0; j < clist->num - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                return ERR_EXEC_CMD;
            }
        } else if (bi_rc != BI_NOT_BI) {
            printf("error: Built-in commands cannot be used in pipelines\n");

            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_EXEC_CMD;
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");

            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_EXEC_CMD;
        }

        if (pid == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(cmd->argv[0], cmd->argv);
            perror("execvp");
            exit(1);
        } else {
            pids[i] = pid;
        }
    }

    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < clist->num; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
    return OK;
}


int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    char *cmd_copy = strdup(cmd_line);
    if (cmd_copy == NULL)
    {
        return ERR_MEMORY;
    }

    clist->num = 0;

    char *cmd_token;
    char *saveptr;

    cmd_token = strtok_r(cmd_copy, PIPE_STRING, &saveptr);

    while (cmd_token != NULL && clist->num < CMD_MAX)
    {
        int rc = alloc_cmd_buff(&(clist->commands[clist->num]));
        if (rc != OK)
        {
            free(cmd_copy);
            free_cmd_list(clist);
            return rc;
        }

        rc = build_cmd_buff(cmd_token, &(clist->commands[clist->num]));
        if (rc == WARN_NO_CMDS)
        {
            printf("%s", CMD_WARN_NO_CMD);
            free_cmd_buff(&(clist->commands[clist->num]));

            // Continue parsing the next command
            cmd_token = strtok_r(NULL, PIPE_STRING, &saveptr);
            continue;
        }

        if (rc == OK)
        {
            clist->num++;
        }

        cmd_token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }

    if (cmd_token != NULL && clist->num >= CMD_MAX)
    {
        printf("error: pipe limit reached, piping limited to %d commands\n", CMD_MAX);
        free(cmd_copy);
        free_cmd_list(clist);
        return ERR_TOO_MANY_COMMANDS;
    }

    free(cmd_copy);

    if (clist->num == 0)
    {
        return WARN_NO_CMDS;
    }

    return OK;
}

int free_cmd_list(command_list_t *cmd_lst) {
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&(cmd_lst->commands[i]));
    }

    cmd_lst->num = 0;
    return OK;
}

// Main command execution loop
int exec_local_cmd_loop() {
    char *cmd_buff;
    int rc = 0;
    cmd_buff_t cmd;
    command_list_t cmd_list; 

    // Allocate memory for command buffer
    cmd_buff = malloc(SH_CMD_MAX * sizeof(char));
    if (cmd_buff == NULL) {
        return ERR_MEMORY;
    }

    rc = alloc_cmd_buff(&cmd);
    if (rc != OK) {
        free(cmd_buff);
        return rc;
    }

    while(1) {
        // Print the prompt, but only the prompt itself
        printf("%s", SH_PROMPT);

        // Get the command input
        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        // Remove any trailing newline character from the input
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Make sure no extra prompt characters are included
        if (cmd_buff[0] == '\0') {
            continue;  // Skip empty inputs
        }

        // Check if the command includes a pipe
        if (strchr(cmd_buff, PIPE_CHAR) != NULL) {
            rc = build_cmd_list(cmd_buff, &cmd_list);

            if (rc == WARN_NO_CMDS) {
                printf("%s", CMD_WARN_NO_CMD);
                continue;
            }
            
            if (rc == ERR_TOO_MANY_COMMANDS) {
                printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
                continue;
            }

            rc = exec_pipeline(&cmd_list);
        
            if (rc == OK_EXIT) {
                break;
            }
            
            free_cmd_list(&cmd_list);
        } else {
            rc = build_cmd_buff(cmd_buff, &cmd);

            if (rc == WARN_NO_CMDS) {
                printf("%s", CMD_WARN_NO_CMD);
                continue;
            }

            Built_In_Cmds bi_rc = exec_built_in_cmd(&cmd);
            if (bi_rc == BI_CMD_EXIT) {
                rc = OK_EXIT;
                break;
            } else if (bi_rc == BI_EXECUTED) {
                continue;
            }

            rc = exec_cmd(&cmd);
            if (rc != OK) {
                printf("Failed executing command\n");
            }
        }
    }

    free(cmd_buff);
    free_cmd_buff(&cmd);

    return rc;
}

