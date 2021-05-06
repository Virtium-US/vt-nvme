#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <string>
#include <cstring>
#include <vector>
#include <iostream>

#include "maverick.h"

#define COMMAND_NOT_DEFINED -1
#define INVALID_ARGUMENTS 2

typedef struct {
    std::string name;
    std::string help;
    int (*func)(int, char**);
} vtNVMeCmd;

static std::vector<vtNVMeCmd> supportedCommands;

#define ADD_CMD(n, h, f) supportedCommands.push_back({n, h, f});

int init(int argc, char** argv);

// standard command set
int displayHelp(int argc, char** argv);

#endif
