#include <iostream>

#include "commands.h"

int main(int argc, char** argv) 
{
    if (argc < 2) 
    {
        std::cout << "Error! Please see \"vtnvme help\" for more info." << std::endl;
        return 1;
    }

    int ret = init(argc, argv);
    if (ret == -1)
    {
        std::cout << "Error! Command \"" << argv[1] << "\" is not supported!" << std:: endl;
        std::cout << "Please see \"vtnvme help\" for more info." << std::endl;
        return 1;
    }
    else if (ret != 0)
    {
        std::cout << "\nSomething went wrong - Error code: " << ret << std::endl; 
    }

    return ret;
}
