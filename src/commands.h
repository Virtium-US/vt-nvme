#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <string>
#include <cstring>
#include <vector>
#include <iostream>
#include <StorageKitTypes.h>
#include <StorageKitStorageDeviceUtils.h>

#include "maverick.h"

#define COMMAND_NOT_DEFINED -1
#define TOO_FEW_ARGUMENTS 1
#define INVALID_ARGUMENTS 2
#define COMMAND_FAILED 3

typedef struct {
    std::string name;
    std::string help;
    std::string usage;
    int (*func)(int, char**);
} vtNVMeCmd;

static std::vector<vtNVMeCmd> supportedCommands;

#define ADD_CMD(n, h, u, f) supportedCommands.push_back({n, h, u, f});

int init(int argc, char** argv);

// standard command set
/**
 * Displays the help menu
 */ 
int displayHelp(int argc, char** argv);

/**
 * Prints list of connected NVMe devices
 */ 
int list(int argc, char** argv);

#endif
