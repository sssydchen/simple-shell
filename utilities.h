int is_pipe_present(int n_tokens, char *tokens[]);
void handle_special_variable(char *tokens[], int n_tokens, char qbuf[]);
int exec_pipes(int commandc, char **argv, int start[], int end[]);
int extract_commands(int argc, char **argv, int start[], int end[]);
int redirect(char **argv, int *argc);