#include <libssh/libssh.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    ssh_session my_ssh_session;
    int rc;
    int port = 22;
    char *password;
    ssh_channel channel;
    
    // 创建 SSH 会话
    my_ssh_session = ssh_new();
    if (my_ssh_session == NULL) {
        fprintf(stderr, "Error creating SSH session\n");
        exit(-1);
    }

    // 设置 SSH 选项（修改为你的服务器信息）
    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, "127.0.0.1");
    ssh_options_set(my_ssh_session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, "qwert");

    // 连接到服务器
    rc = ssh_connect(my_ssh_session);
    if (rc != SSH_OK) {
        fprintf(stderr, "Error connecting: %s\n", ssh_get_error(my_ssh_session));
        ssh_free(my_ssh_session);
        exit(-1);
    }

    // 验证密码（实际应用中应该使用更安全的方式获取密码）
    password = "leeweop";
    rc = ssh_userauth_password(my_ssh_session, NULL, password);
    if (rc != SSH_AUTH_SUCCESS) {
        fprintf(stderr, "Authentication failed: %s\n", ssh_get_error(my_ssh_session));
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        exit(-1);
    }

    // 创建通道
    channel = ssh_channel_new(my_ssh_session);
    if (channel == NULL) {
        fprintf(stderr, "Error creating channel\n");
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        exit(-1);
    }

    // 打开通道
    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        fprintf(stderr, "Error opening channel\n");
        ssh_channel_free(channel);
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        exit(-1);
    }

    // 执行命令（示例命令）
    rc = ssh_channel_request_exec(channel, "ls -l");
    if (rc != SSH_OK) {
        fprintf(stderr, "Error executing command\n");
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        exit(-1);
    }

    // 读取输出
    char buffer[256];
    int nbytes;
    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, nbytes, stdout);
    }

    // 清理资源
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);

    return 0;
}
