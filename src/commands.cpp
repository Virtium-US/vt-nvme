#include "commands.h"

int displayHelp(int argc, char** argv)
{
    std::cout << "\nUsage: vtnvme COMMAND [ARGS]" << std::endl;

    std::cout << "\nSupported commands:" << std::endl;
    for (auto cmdEntry = supportedCommands.begin(); cmdEntry != supportedCommands.end(); cmdEntry++)
    {
        std::cout << "  " << cmdEntry->name << ": " << cmdEntry->help << std::endl;
    }

    return 0;
}

int init(int argc, char** argv)
{
    ADD_CMD(
        "help",
        "Displays this menu", 
        displayHelp
    );

    ADD_CMD(
        "set-op", 
        "Set over-previsioning percentage (ex: vtnvme set-op 75)", 
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