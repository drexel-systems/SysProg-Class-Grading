
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

//INCLUDES for extra credit
#include <signal.h>
#include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"

void *handle_client(void *arg);
int process_cli_requests_threaded(int svr_socket);


void *handle_client(void *arg) {
    int cli_socket = *(int *)arg;
    free(arg);  // Free memory allocated for socket

    exec_client_requests(cli_socket);
    close(cli_socket);
    return NULL;
}

int process_cli_requests_threaded(int svr_socket) {
    int *cli_socket;
    pthread_t tid;

    while (1) {
        cli_socket = malloc(sizeof(int));
        if (cli_socket == NULL) {
            perror("malloc");
            return ERR_RDSH_SERVER;
        }

        *cli_socket = accept(svr_socket, NULL, NULL);
        if (*cli_socket < 0) {
            perror("accept");
            free(cli_socket);
            return ERR_RDSH_COMMUNICATION;
        }

        printf("Client connected (Threaded).\n");

        if (pthread_create(&tid, NULL, handle_client, cli_socket) != 0) {
            perror("pthread_create");
            free(cli_socket);
            return ERR_RDSH_SERVER;
        }

        pthread_detach(tid);  // Auto-clean up thread
    }

    return OK;
}




/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:  a string in ip address format, indicating the interface
 *              where the server will bind.  In almost all cases it will
 *              be the default "0.0.0.0" which binds to all interfaces.
 *              note the constant RDSH_DEF_SVR_INTFACE in rshlib.h
 * 
 *      port:   The port the server will use.  Note the constant 
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.  If you are using
 *              tux you may need to change this to your own default, or even
 *              better use the command line override -s implemented in dsh_cli.c
 *              For example ./dsh -s 0.0.0.0:5678 where 5678 is the new port  
 * 
 *      is_threded:  Used for extra credit to indicate the server should implement
 *                   per thread connections for clients  
 * 
 *      This function basically runs the server by: 
 *          1. Booting up the server
 *          2. Processing client requests until the client requests the
 *             server to stop by running the `stop-server` command
 *          3. Stopping the server. 
 * 
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.  
 * 
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.  
 */
 int start_server(char *ifaces, int port, int is_threaded){
    int svr_socket;
    int rc;

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0){
        int err_code = svr_socket;  //server socket will carry error code
        return err_code;
    }

    if (is_threaded) {
        printf("Running in multi-threaded mode\n");
        rc = process_cli_requests_threaded(svr_socket); // New function to handle multi-threading
    } else {
        rc = process_cli_requests(svr_socket);
    }

    stop_server(svr_socket);

    return rc;
}


/*
 * stop_server(svr_socket)
 *      svr_socket: The socket that was created in the boot_server()
 *                  function. 
 * 
 *      This function simply returns the value of close() when closing
 *      the socket.  
 */
int stop_server(int svr_socket){
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 *      ifaces & port:  see start_server for description.  They are passed
 *                      as is to this function.   
 * 
 *      This function "boots" the rsh server.  It is responsible for all
 *      socket operations prior to accepting client connections.  Specifically: 
 * 
 *      1. Create the server socket using the socket() function. 
 *      2. Calling bind to "bind" the server to the interface and port
 *      3. Calling listen to get the server ready to listen for connections.
 * 
 *      after creating the socket and prior to calling bind you might want to 
 *      include the following code:
 * 
 *      int enable=1;
 *      setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
 * 
 *      when doing development you often run into issues where you hold onto
 *      the port and then need to wait for linux to detect this issue and free
 *      the port up.  The code above tells linux to force allowing this process
 *      to use the specified port making your life a lot easier.
 * 
 *  Returns:
 * 
 *      server_socket:  Sockets are just file descriptors, if this function is
 *                      successful, it returns the server socket descriptor, 
 *                      which is just an integer.
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code is returned if the socket(),
 *                               bind(), or listen() call fails. 
 * 
 */
 int boot_server(char *ifaces, int port) {
    int svr_socket;
    int ret;
    struct sockaddr_in addr;
    int enable = 1; // Used for setsockopt

    // Step 1: Create a socket (IPv4, Stream/TCP)
    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket == -1) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    // Step 2: Allow address reuse to avoid "Address already in use" errors
    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Step 3: Configure the sockaddr_in structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;               // IPv4
    addr.sin_port = htons(port);             // Convert port to network byte order
    addr.sin_addr.s_addr = inet_addr(ifaces); // Bind to specified interface

    // Step 4: Bind the socket to the address and port
    ret = bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Step 5: Start listening with a backlog of 20 connections
    ret = listen(svr_socket, 20);
    if (ret == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    printf("Server started on %s:%d\n", ifaces, port);
    return svr_socket;
}


/*
 * process_cli_requests(svr_socket)
 *      svr_socket:  The server socket that was obtained from boot_server()
 *   
 *  This function handles managing client connections.  It does this using
 *  the following logic
 * 
 *      1.  Starts a while(1) loop:
 *  
 *          a. Calls accept() to wait for a client connection. Recall that 
 *             the accept() function returns another socket specifically
 *             bound to a client connection. 
 *          b. Calls exec_client_requests() to handle executing commands
 *             sent by the client. It will use the socket returned from
 *             accept().
 *          c. Loops back to the top (step 2) to accept connecting another
 *             client.  
 * 
 *          note that the exec_client_requests() return code should be
 *          negative if the client requested the server to stop by sending
 *          the `stop-server` command.  If this is the case step 2b breaks
 *          out of the while(1) loop. 
 * 
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to 
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket. 
 * 
 *  Returns:
 * 
 *      OK_EXIT:  When the client sends the `stop-server` command this function
 *                should return OK_EXIT. 
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code terminates the loop and is
 *                returned from this function in the case of the accept() 
 *                function failing. 
 * 
 *      OTHERS:   See exec_client_requests() for return codes.  Note that positive
 *                values will keep the loop running to accept additional client
 *                connections, and negative values terminate the server. 
 * 
 */
 int process_cli_requests(int svr_socket){
    int cli_socket;
    int rc = OK;    

    while(1){
        
        cli_socket = accept(svr_socket, NULL, NULL);
        if (cli_socket < 0) {
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }

        printf("Client connected.\n");

        
        rc = exec_client_requests(cli_socket);

        
        close(cli_socket);

        if (rc == OK_EXIT) {
            printf("Server shutting down...\n");
            break;
        }
    }

    stop_server(svr_socket);
    return rc;
}


/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *   
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 * 
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection. 
 *      2.  When the client executes the `stop-server` command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop. 
 * 
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last 
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard. 
 * 
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish 
 *  this. 
 * 
 *  Of final note, this function must allocate a buffer for storage to 
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 * 
 *  Returns:
 * 
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client. 
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 * 
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors. 
 */
 int exec_client_requests(int cli_socket) {
    int io_size;
    command_list_t cmd_list;
    int rc;
    char *io_buff;

    // Allocate buffer for receiving data
    io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        perror("malloc");
        return ERR_RDSH_SERVER;
    }

    while (1) {
        // Step 1: Receive command from client
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);
        io_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ - 1, 0);

        // Step 2: Handle recv() errors
        if (io_size < 0) {
            perror("recv");
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }
        if (io_size == 0) {
            // Client disconnected
            printf("Client disconnected.\n");
            free(io_buff);
            return OK;
        }

        // Step 3: Ensure received command is null-terminated
        io_buff[io_size] = '\0';

        // Step 4: Check for overly long input (Buffer Overflow Protection)
        if (io_size >= RDSH_COMM_BUFF_SZ - 1) {
            send_message_string(cli_socket, "Error: Command too long. Input rejected.\n");
            continue;
        }

        printf("Received command from client: %s\n", io_buff);  // Debug logging

        // Step 5: Parse command into a command list
        rc = build_cmd_list(io_buff, &cmd_list);
        if (rc != OK) {
            send_message_string(cli_socket, "Error: Invalid command format.\n");
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }

        // Step 6: Check for built-in commands
        if (cmd_list.num > 0) {
            Built_In_Cmds bi_cmd = rsh_match_command(cmd_list.commands[0].argv[0]);

            if (bi_cmd == BI_CMD_EXIT) {
                send_message_string(cli_socket, "Exiting client session.\n");
                free_cmd_list(&cmd_list);
                free(io_buff);
                return OK; 
            }
            if (bi_cmd == BI_CMD_STOP_SVR) {
                send_message_string(cli_socket, "Server stopping.\n");
                free_cmd_list(&cmd_list);
                free(io_buff);
                return OK_EXIT;
            }
        }

        // Step 7: Execute the pipeline command
        rc = rsh_execute_pipeline(cli_socket, &cmd_list);
        free_cmd_list(&cmd_list);

        // **🚀 FIX: Handle Invalid Commands Properly**
        if (rc != 0) {  
            send_message_string(cli_socket, "Error: Command not found or failed to execute.\n");
            free(io_buff);
            return ERR_EXEC_CMD;  // Return a non-zero error code
        }

        // Step 8: Send EOF message to signal end of response
        if (send_message_eof(cli_socket) != OK) {
            perror("send_message_eof failed");
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }
    }

    free(io_buff);
    return OK;
}




/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent. 
 * 
 *  Returns:
 * 
 *      OK:  The EOF character was sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character. 
 */
int send_message_eof(int cli_socket){
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len;
    sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);

    if (sent_len != send_len){
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}


/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client. 
 *   
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated. 
 * 
 *  Returns:
 * 
 *      OK:  The message in buff followed by the EOF character was 
 *           sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character. 
 */
 int send_message_string(int cli_socket, char *buff){
    int send_len = strlen(buff) + 1;
    int sent_bytes;

    
    sent_bytes = send(cli_socket, buff, send_len, 0);
    if (sent_bytes != send_len) {
        perror("send_message_string: send() failed");
        return ERR_RDSH_COMMUNICATION;
    }

    // Send the EOF marker
    if (send_message_eof(cli_socket) != OK) {
        return ERR_RDSH_COMMUNICATION;
    }

    return OK;
}



/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell. 
 *   
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable. 
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:  
 * 
 *      
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │    
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │    
 *      │   │              │     │              │     │              │  │  │    
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘  │    
 *          │              │ │   │              │ │   │              │     │    
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘    
 *          └──────────────┘     └──────────────┘     └──────────────┘   
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function       
 * 
 *  Returns:
 * 
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value. 
 */
 int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int pipes[clist->num - 1][2];  // Pipes for inter-process communication
    pid_t pids[clist->num];        // Process IDs of forked processes
    int pids_st[clist->num];       // Exit statuses
    int exit_code;

    // Step 1: Create pipes for inter-process communication
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    // Step 2: Iterate over commands and fork processes
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork");
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) {  
            // Step 3: Handle input redirection
            if (i == 0) {
                dup2(cli_sock, STDIN_FILENO);  
            } else {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            // Step 4: Handle output redirection
            if (i == clist->num - 1) {
                dup2(cli_sock, STDOUT_FILENO);
                dup2(cli_sock, STDERR_FILENO);
            } else {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Step 5: Close all unused pipes
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Step 6: Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);

            perror("execvp");
            send_message_string(cli_sock, "command not found\n");
            exit(ERR_EXEC_CMD);
        }
    }

    // Step 7: Parent process closes all pipes
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Step 8: Wait for all child processes
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &pids_st[i], 0);
    }

    // Step 9: Get the exit code of the last command
    exit_code = WEXITSTATUS(pids_st[clist->num - 1]);

    for (int i = 0; i < clist->num; i++) {
        if (WEXITSTATUS(pids_st[i]) != 0) {
            return ERR_EXEC_CMD;
        }
    }

    return exit_code;
}


/**************   OPTIONAL STUFF  ***************/
/****
 **** NOTE THAT THE FUNCTIONS BELOW ALIGN TO HOW WE CRAFTED THE SOLUTION
 **** TO SEE IF A COMMAND WAS BUILT IN OR NOT.  YOU CAN USE A DIFFERENT
 **** STRATEGY IF YOU WANT.  IF YOU CHOOSE TO DO SO PLEASE REMOVE THESE
 **** FUNCTIONS AND THE PROTOTYPES FROM rshlib.h
 **** 
 */

/*
 * rsh_match_command(const char *input)
 *      cli_socket:  The string command for a built-in command, e.g., dragon,
 *                   cd, exit-server
 *   
 *  This optional function accepts a command string as input and returns
 *  one of the enumerated values from the BuiltInCmds enum as output. For
 *  example:
 * 
 *      Input             Output
 *      exit              BI_CMD_EXIT
 *      dragon            BI_CMD_DRAGON
 * 
 *  This function is entirely optional to implement if you want to handle
 *  processing built-in commands differently in your implementation. 
 * 
 *  Returns:
 * 
 *      BI_CMD_*:   If the command is built-in returns one of the enumeration
 *                  options, for example "cd" returns BI_CMD_CD
 * 
 *      BI_NOT_BI:  If the command is not "built-in" the BI_NOT_BI value is
 *                  returned. 
 */
 Built_In_Cmds rsh_match_command(const char *input)
 {
     if (input == NULL) 
         return BI_NOT_BI;
 
     if (strcmp(input, "exit") == 0)
         return BI_CMD_EXIT;
     if (strcmp(input, "dragon") == 0)
         return BI_CMD_DRAGON;
     if (strcmp(input, "cd") == 0)
         return BI_CMD_CD;
     if (strcmp(input, "stop-server") == 0)
         return BI_CMD_STOP_SVR;
     if (strcmp(input, "rc") == 0)
         return BI_CMD_RC;
 
     return BI_NOT_BI;
 }
 

/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 *      cmd:  The cmd_buff_t of the command, remember, this is the 
 *            parsed version fo the command
 *   
 *  This optional function accepts a parsed cmd and then checks to see if
 *  the cmd is built in or not.  It calls rsh_match_command to see if the 
 *  cmd is built in or not.  Note that rsh_match_command returns BI_NOT_BI
 *  if the command is not built in. If the command is built in this function
 *  uses a switch statement to handle execution if appropriate.   
 * 
 *  Again, using this function is entirely optional if you are using a different
 *  strategy to handle built-in commands.  
 * 
 *  Returns:
 * 
 *      BI_NOT_BI:   Indicates that the cmd provided as input is not built
 *                   in so it should be sent to your fork/exec logic
 *      BI_EXECUTED: Indicates that this function handled the direct execution
 *                   of the command and there is nothing else to do, consider
 *                   it executed.  For example the cmd of "cd" gets the value of
 *                   BI_CMD_CD from rsh_match_command().  It then makes the libc
 *                   call to chdir(cmd->argv[1]); and finally returns BI_EXECUTED
 *      BI_CMD_*     Indicates that a built-in command was matched and the caller
 *                   is responsible for executing it.  For example if this function
 *                   returns BI_CMD_STOP_SVR the caller of this function is
 *                   responsible for stopping the server.  If BI_CMD_EXIT is returned
 *                   the caller is responsible for closing the client connection.
 * 
 *   AGAIN - THIS IS TOTALLY OPTIONAL IF YOU HAVE OR WANT TO HANDLE BUILT-IN
 *   COMMANDS DIFFERENTLY. 
 */
 Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd == NULL || cmd->argv[0] == NULL) {
        return BI_NOT_BI; // Prevent segmentation faults
    }

    Built_In_Cmds ctype = rsh_match_command(cmd->argv[0]);

    switch (ctype) {
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;

        case BI_CMD_STOP_SVR:
            return BI_CMD_STOP_SVR;

        case BI_CMD_RC:
            return BI_CMD_RC;

        case BI_CMD_CD:
            if (cmd->argc < 2) { 
                send_message_string(STDOUT_FILENO, "cd: missing directory argument\n");
                return BI_EXECUTED;
            }
            if (chdir(cmd->argv[1]) != 0) { 
                perror("cd");
                send_message_string(STDOUT_FILENO, "cd: failed to change directory\n");
                return BI_NOT_BI; 
            return BI_EXECUTED;

        default:
            return BI_NOT_BI;
    }
}
