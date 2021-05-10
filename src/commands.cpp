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
    std::vector<SKStorageDevice*> storageDevices;
    SKStorageDeviceUtils::scanDevices(storageDevices);

    if (storageDevices.size() == 0) 
    {
        std::cout << "No NVMe devices found. (Do you have admin rights?)" << std::endl;
        return COMMAND_FAILED;
    }

    std::cout << "NVMe devices found: " << std::endl;
    SKAlignedBuffer* identifyInfo = new SKAlignedBuffer(4096);
    for (const auto& device : storageDevices) 
    {
        if (device->getDeviceType() == SKDeviceType::NVME) 
        {
            dynamic_cast<SKNvmeDevice*>(device)->identifyController(identifyInfo);
            char serialNumber[20];
            char modelNumber[40];
            strncpy(serialNumber, (char*) identifyInfo->ToDataBuffer() + 4, 20);
            strncpy(modelNumber, (char*) identifyInfo->ToDataBuffer() + 24, 40);
            printf("- %s %s %s\n", device->getDevicePath().c_str(), serialNumber, modelNumber);
        }

        delete device;
    }

    storageDevices.clear();
    delete identifyInfo;

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
        "get-op", 
        "Returns current over-provisioning information (ex: vtnvme get-op /dev/nvme0n1)", 
        "vtnvme get-op DEVICE",
        getOP
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