#include "commands.h"

int displayHelp(int argc, char** argv)
{
    std::cout << "\nUsage: vtnvme COMMAND [ARGS]" << std::endl;

    std::cout << "\nSupported commands:" << std::endl;
    for (auto cmdEntry = supportedCommands.begin(); cmdEntry != supportedCommands.end(); cmdEntry++)
    {
        std::cout << "  " << cmdEntry->name << ": " << cmdEntry->usage << " - " << cmdEntry->help << std::endl;
    }

    return 0;
}

int list(int argc, char** argv)
{
    std::cout << "NVMe devices found:" << std::endl;
    return 0;
}

int init(int argc, char** argv)
{
    ADD_CMD(
        "help",
        "Displays this menu",
        "vtnvme help",
        displayHelp
    );

    ADD_CMD(
        "list",
        "Prints list of connected NVMe devices",
        "vtnvme list",
        list
    );

    ADD_CMD(
        "set-op", 
        "Sets over-provisioning percentage (ex: vtnvme set-op /dev/nvme0n1 40)", 
        "vtnvme set-op DEVICE OP%",
        setOP
    );

    for (auto cmdEntry = supportedCommands.begin(); cmdEntry != supportedCommands.end(); cmdEntry++)
    {
        if (strcmp(argv[1], cmdEntry->name.c_str()) == 0)
        {
            return cmdEntry->func(argc, argv);
        }
    }

    return COMMAND_NOT_DEFINED;
}