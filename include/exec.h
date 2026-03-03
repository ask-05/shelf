#ifndef EXEC_H
#define EXEC_H

int shelf_launch(char **args);
int shelf_launch_pipeline(char ***cmds, int ncmds);
int shelf_execute(char **args);

#endif
