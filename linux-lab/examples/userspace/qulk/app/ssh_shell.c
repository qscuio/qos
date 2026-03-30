#include <libssh/libssh.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

int authenticate(ssh_session session, const char *password) {
    int rc = ssh_userauth_password(session, NULL, password);
    if (rc == SSH_AUTH_SUCCESS) {
        return 0;
    } else if (rc == SSH_AUTH_DENIED) {
        fprintf(stderr, "Authentication failed: Invalid credentials\n");
    } else {
        fprintf(stderr, "Authentication error: %s\n", ssh_get_error(session));
    }
    return -1;
}

int execute_remote_command(ssh_channel channel, const char *command) {
    int rc = ssh_channel_request_exec(channel, command);
    if (rc != SSH_OK) {
        fprintf(stderr, "Error executing command: %s\n", ssh_get_error(ssh_channel_get_session(channel)));
        return -1;
    }
    return 0;
}

void print_remote_output(ssh_channel channel) {
    char buffer[BUFFER_SIZE];
    int nbytes;
    
    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, nbytes, stdout);
    }
    
    if (nbytes < 0) {
        fprintf(stderr, "Error reading channel: %s\n", ssh_get_error(ssh_channel_get_session(channel)));
    }
}

int interactive_shell(ssh_channel channel) {
    char command[BUFFER_SIZE];
    
    printf("Enter commands to execute remotely (type 'exit' to quit):\n");
    
    while (true) {
        printf("ssh> ");
        fflush(stdout);
        
        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("\nExit\n");
            break;
        }
        
        // 移除换行符
        command[strcspn(command, "\n")] = 0;
        
        if (strlen(command) == 0) {
            continue;
        }
        
        if (strcmp(command, "exit") == 0) {
            break;
        }
        
        if (execute_remote_command(channel, command) == 0) {
            print_remote_output(channel);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    ssh_session session = ssh_new();
    ssh_channel channel = NULL;
    int port = 22;
    const char *host = "127.0.0.1";
    const char *user = "qwert";
    const char *password = "leeweop";
    int rc;

    if (!session) {
        fprintf(stderr, "Error creating SSH session\n");
        return EXIT_FAILURE;
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, host);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_USER, user);

    if (ssh_connect(session) != SSH_OK) {
        fprintf(stderr, "Connection failed: %s\n", ssh_get_error(session));
        ssh_free(session);
        return EXIT_FAILURE;
    }

    if (authenticate(session, password) != 0) {
        ssh_disconnect(session);
        ssh_free(session);
        return EXIT_FAILURE;
    }

    channel = ssh_channel_new(session);
    if (!channel || ssh_channel_open_session(channel) != SSH_OK) {
        fprintf(stderr, "Failed to create channel\n");
        ssh_disconnect(session);
        ssh_free(session);
        return EXIT_FAILURE;
    }

    interactive_shell(channel);

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);

    return EXIT_SUCCESS;
}
