#ifndef BUILTINS_H
#define BUILTINS_H

extern char *shelf_builtin[];
extern int (*shelf_builtin_funcs[]) (char **);
extern int num_builtin_funcs;

int shelf_cd(char **args);
int shelf_help(char **args);
int shelf_exit(char **args);

#endif
