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
    SKBaseDeviceInfo* dev = SKStorageProtocol::scan(devPath);
    SKNvmeProtocol* nvmeInterface = new SKNvmeProtocol(dev->devicePath, dev->deviceHandle);

    SKAlignedBuffer* buffer = new SKAlignedBuffer(BUFFER_SIZE);
    memset(buffer->ToDataBuffer(), 0, BUFFER_SIZE);

    // applying changes to OP
    // setup set op command
    buffer->ToDataBuffer()[0] = 0x27;
    buffer->ToDataBuffer()[1] = 0x05;
    buffer->ToDataBuffer()[2] = 0x01;
    buffer->ToDataBuffer()[3] = OP_AMT;
    SKNvmeCommandDesc* setupSetOP = new SKNvmeCommandDesc(WRITE_TO_DEVICE, 0, SKNvmeCommandDesc::COMMAND_ADMIN);
    setupSetOP->inputFields.OpCode = 0xFD;
    setupSetOP->inputFields.AdminCommand.Cdw12 = 0x00FC;
    setupSetOP->inputFields.AdminCommand.Cdw10 = 128;
    setupSetOP->dataTransferLengthInSectors = BUFFER_SIZE/512;
    nvmeInterface->issueCommand(setupSetOP, buffer);

    // execute set op command
    memset(buffer->ToDataBuffer(), 0, BUFFER_SIZE);
    SKNvmeCommandDesc* setOP = new SKNvmeCommandDesc(READ_FROM_DEVICE, 0, SKNvmeCommandDesc::COMMAND_ADMIN);
    setOP->inputFields.OpCode = 0xFE;
    setOP->inputFields.AdminCommand.Cdw12 = 0x00FD;
    setOP->inputFields.AdminCommand.Cdw10 = 1;
    setOP->dataTransferLengthInSectors = 4;
    nvmeInterface->issueCommand(setOP, buffer);

    if (buffer->ToDataBuffer()[0] != 0 || buffer->ToDataBuffer()[2] != OP_AMT)
    {
        printf("ERROR - set op command status: %d\n", buffer->ToDataBuffer()[0]);
        return COMMAND_FAILED;
    }

    printf("Success! OP of %d will take effect on next boot\n", buffer->ToDataBuffer()[2]);

    delete dev;
    delete buffer;
    delete nvmeInterface;
    delete setupSetOP;
    delete setOP;

    return 0;
}

int getOP(int argc, char** argv)
{
    if (argc < 3)
    {
        return TOO_FEW_ARGUMENTS;
    }

    char* devPath = argv[2];

    SKBaseDeviceInfo* dev = SKStorageProtocol::scan(devPath);
    SKNvmeProtocol* nvmeInterface = new SKNvmeProtocol(dev->devicePath, dev->deviceHandle);

    // setup command
    SKAlignedBuffer* buffer = new SKAlignedBuffer(BUFFER_SIZE);
    memset(buffer->ToDataBuffer(), 0, BUFFER_SIZE);
    buffer->ToDataBuffer()[0] = 0x27;
    buffer->ToDataBuffer()[1] = 0x05;
    SKNvmeCommandDesc* setupGetOP = new SKNvmeCommandDesc(WRITE_TO_DEVICE, 0, SKNvmeCommandDesc::COMMAND_ADMIN);
    setupGetOP->inputFields.OpCode = 0xFD;
    setupGetOP->inputFields.AdminCommand.Cdw12 = 0x00FC;
    setupGetOP->inputFields.AdminCommand.Cdw10 = 128;
    setupGetOP->dataTransferLengthInSectors = BUFFER_SIZE/512;
    nvmeInterface->issueCommand(setupGetOP, buffer);

    // execute command
    memset(buffer->ToDataBuffer(), 0, BUFFER_SIZE);
    SKNvmeCommandDesc* getOP = new SKNvmeCommandDesc(READ_FROM_DEVICE, 0, SKNvmeCommandDesc::COMMAND_ADMIN);
    getOP->inputFields.OpCode = 0xFE;
    getOP->inputFields.AdminCommand.Cdw12 = 0x00FD;
    getOP->inputFields.AdminCommand.Cdw10 = 1;
    getOP->dataTransferLengthInSectors = 4;
    nvmeInterface->issueCommand(getOP, buffer);

    if (buffer->ToDataBuffer()[0] != 0)
    {
        printf("ERROR: Get OP VSC returned non-zero command status: %d\n", buffer->ToDataBuffer()[0]);
        return COMMAND_FAILED;
    }

    printf("Current OP:      %d\n", buffer->ToDataBuffer()[1]);
    printf("OP on next boot: %d\n", buffer->ToDataBuffer()[2]);
    delete dev;
    delete buffer;
    delete nvmeInterface;
    delete setupGetOP;
    delete getOP;

    return 0;
}
