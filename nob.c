#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

int main(int argc, char **argv)
{

    NOB_GO_REBUILD_URSELF(argc, argv);

    char src_file[] = SRC_FOLDER "main.c";
    char out_file[] = BUILD_FOLDER "cg";

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER))
        return true;

    Nob_Cmd cmd = {0};

    nob_cmd_append(&cmd, "cc");
    nob_cmd_append(&cmd, "-Wall", "-Wextra");
    nob_cmd_append(&cmd, "-o", out_file);
    nob_cmd_append(&cmd, src_file);

    if (!nob_cmd_run(&cmd)) return 1;

    return 0;
}