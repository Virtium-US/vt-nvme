#include "maverick.h"

int setOP(int argc, char** argv)
{
    if (argc < 3)
    {
        return INVALID_ARGUMENTS;
    }

    int OP_AMT = atoi(argv[2]);
    
    std::cout << "OP_AMT: " << OP_AMT << std::endl;

    return 0;
}
