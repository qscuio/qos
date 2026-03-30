#ifndef __HAVE_UNIXCTL_H__
#define __HAVE_UNIXCTL_H__

/* Server for Unix domain socket control connection. */
struct unixctl_server;
int unixctl_server_create(const char *path, struct unixctl_server **);
void unixctl_server_run(struct unixctl_server *);
void unixctl_server_wait(struct unixctl_server *);
void unixctl_server_destroy(struct unixctl_server *);

const char *unixctl_server_get_path(const struct unixctl_server *);

/* Client for Unix domain socket control connection. */
struct jsonrpc;
int unixctl_client_create(const char *path, struct jsonrpc **client);
int unixctl_client_transact(struct jsonrpc *client,
                            const char *command,
                            int argc, char *argv[],
                            char **result, char **error);

/* Command registration. */
struct unixctl_conn;
typedef void unixctl_cb_func(struct unixctl_conn *,
                             int argc, const char *argv[], void *aux);
void unixctl_command_register(const char *name, const char *usage,
                              int min_args, int max_args,
                              unixctl_cb_func *cb, void *aux);
void unixctl_command_reply_error(struct unixctl_conn *, const char *error);
void unixctl_command_reply(struct unixctl_conn *, const char *body);

#endif /* __HAVE_UNIXCTL_H__ */
