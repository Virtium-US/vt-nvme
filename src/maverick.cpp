#include "maverick.h"

int setOP(int argc, char** argv)
{
    if (argc < 4)
    {
        return TOO_FEW_ARGUMENTS;
    }

    int OP_AMT = atoi(argv[3]);
    if (OP_AMT < 0 || OP_AMT > 50)
    {
        std::cout << "\nError: Invalid over-previsioning range! (Valid OP range is between 0% and 50%)" << std::endl;
        return INVALID_ARGUMENTS;
    }

    char* devPath = argv[2];
    std::cout << "Setting OP for device " << devPath << " to " << OP_AMT << std::endl;

    return 0;
}

int getOP(int argc, char** argv)
{
    if (argc < 3)
    {
        return TOO_FEW_ARGUMENTS;
    }

    char* devPath = argv[2];

    return 0;
}
