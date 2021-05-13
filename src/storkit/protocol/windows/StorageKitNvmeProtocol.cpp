#include <wtypesbase.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include "win_nvme.h" // customized version of 
#include "StorageKitNvmeProtocol.h"

using namespace std;

#define IDENTIFY_BUFFER_LENGTH                          4096
#define IDENTIFY_CONTROLLER_VENDOR_ID_INDEX             0
#define IDENTIFY_CONTROLLER_NO_NAMESPACE_INDEX          516
#define ADAPTER_PROTOCOL_ID                             49
#define DEVICE_PROTOCOL_ID                              50
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER                         0x00000001
#define STORAGE_HW_FIRMWARE_REQUEST_FLAG_SWITCH_TO_EXISTING_FIRMWARE        0x80000000

#define IOCTL_STORAGE_FIRMWARE_DOWNLOAD         CTL_CODE(IOCTL_STORAGE_BASE, 0x0701, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_STORAGE_FIRMWARE_ACTIVATE         CTL_CODE(IOCTL_STORAGE_BASE, 0x0702, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_STORAGE_PROTOCOL_COMMAND          CTL_CODE(IOCTL_STORAGE_BASE, 0x04F0, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

const STORAGE_PROPERTY_ID StorageAdapterProtocolSpecificProperty = static_cast<STORAGE_PROPERTY_ID>(ADAPTER_PROTOCOL_ID);
const STORAGE_PROPERTY_ID StorageDeviceProtocolSpecificProperty = static_cast<STORAGE_PROPERTY_ID>(DEVICE_PROTOCOL_ID);

typedef enum _STORAGE_PROTOCOL_TYPE
{
    ProtocolTypeUnknown = 0x00,
    ProtocolTypeScsi,
    ProtocolTypeAta,
    ProtocolTypeNvme,
    ProtocolTypeSd,
    ProtocolTypeUfs,
    ProtocolTypeProprietary = 0x7E,
    ProtocolTypeMaxReserved = 0x7F
} STORAGE_PROTOCOL_TYPE, *PSTORAGE_PROTOCOL_TYPE;

typedef enum _STORAGE_PROTOCOL_NVME_DATA_TYPE
{
    NVMeDataTypeUnknown = 0,
    NVMeDataTypeIdentify,
    NVMeDataTypeLogPage,
    NVMeDataTypeFeature,
} STORAGE_PROTOCOL_NVME_DATA_TYPE, *PSTORAGE_PROTOCOL_NVME_DATA_TYPE;

typedef struct _STORAGE_PROTOCOL_SPECIFIC_DATA
{
    STORAGE_PROTOCOL_TYPE ProtocolType;
    U32 DataType;
    U32 ProtocolDataRequestValue;
    U32 ProtocolDataRequestSubValue;
    U32 ProtocolDataOffset;
    U32 ProtocolDataLength;
    U32 FixedProtocolReturnData;
    U32 ProtocolDataRequestSubValue2;
    U32 Reserved[2];
} STORAGE_PROTOCOL_SPECIFIC_DATA, *PSTORAGE_PROTOCOL_SPECIFIC_DATA;

typedef struct _STORAGE_PROTOCOL_DATA_DESCRIPTOR
{
    U32 Version;
    U32 Size;
    STORAGE_PROTOCOL_SPECIFIC_DATA ProtocolSpecificData;
} STORAGE_PROTOCOL_DATA_DESCRIPTOR, *PSTORAGE_PROTOCOL_DATA_DESCRIPTOR;

typedef struct _STORAGE_HW_FIRMWARE_DOWNLOAD {
    U32 Version;
    U32 Size;
    U32 Flags;
    U8 Slot;
    U8 Reserved[3];
    U64 Offset;
    U64 BufferSize;
    U8 ImageBuffer[ANYSIZE_ARRAY];
} STORAGE_HW_FIRMWARE_DOWNLOAD, *PSTORAGE_HW_FIRMWARE_DOWNLOAD;

typedef struct _STORAGE_HW_FIRMWARE_ACTIVATE {
    U32 Version;
    U32 Size;
    U32 Flags;
    U8 Slot;
    U8 Reserved0[3];
} STORAGE_HW_FIRMWARE_ACTIVATE, *PSTORAGE_HW_FIRMWARE_ACTIVATE;

typedef struct _STORAGE_PROTOCOL_COMMAND {

    ULONG   Version;                        // STORAGE_PROTOCOL_STRUCTURE_VERSION
    ULONG   Length;                         // sizeof(STORAGE_PROTOCOL_COMMAND)

    STORAGE_PROTOCOL_TYPE  ProtocolType;
    ULONG   Flags;                          // Flags for the request

    ULONG   ReturnStatus;                   // return value
    ULONG   ErrorCode;                      // return value, optional

    ULONG   CommandLength;                  // non-zero value should be set by caller
    ULONG   ErrorInfoLength;                // optional, can be zero
    ULONG   DataToDeviceTransferLength;     // optional, can be zero. Used by WRITE type of request.
    ULONG   DataFromDeviceTransferLength;   // optional, can be zero. Used by READ type of request.

    ULONG   TimeOutValue;                   // in unit of seconds

    ULONG   ErrorInfoOffset;                // offsets need to be pointer aligned
    ULONG   DataToDeviceBufferOffset;       // offsets need to be pointer aligned
    ULONG   DataFromDeviceBufferOffset;     // offsets need to be pointer aligned

    ULONG   CommandSpecific;                // optional information passed along with Command.
    ULONG   Reserved0;

    ULONG   FixedProtocolReturnData;        // return data, optional. Some protocol, such as NVMe, may return a small amount data (DWORD0 from completion queue entry) without the need of separate device data transfer.
    ULONG   Reserved1[3];

    _Field_size_bytes_full_(CommandLength) UCHAR Command[ANYSIZE_ARRAY];

} STORAGE_PROTOCOL_COMMAND, *PSTORAGE_PROTOCOL_COMMAND;

U16 getWord(U8* buffer, U32 index)
{
    return static_cast<U16>((buffer[(index * 2) + 1] << 8) | buffer[(index) * 2]);
}

U32 getU32(U8* buffer, U32 byteIndex)
{
    return static_cast<U32>(buffer[byteIndex])                |
            (static_cast<U32>(buffer[byteIndex + 1]) << 8)    |
            (static_cast<U32>(buffer[byteIndex + 2]) << 16)   |
            (static_cast<U32>(buffer[byteIndex + 3]) << 24);
}

bool getStorageProperty(const DeviceHandle &handle, SKAlignedBuffer *dataBuffer, U32 dataType, U32 requestValue, U32 nsid)
{
    if (!dataBuffer)
    {
        return false;
    }

    U32 dataBufferSize = dataBuffer->GetSizeInByte();
    U16 cmdLength = FIELD_OFFSET(STORAGE_PROPERTY_QUERY, AdditionalParameters) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    U8 *dataBufferWithLeftSpare = dataBuffer->ToDataBufferWithLeftSpare(cmdLength);
    U32 lengthOfDataBufferWithLeftSpare = dataBuffer->GetSizeInByte() + cmdLength;

    ZeroMemory(dataBufferWithLeftSpare, lengthOfDataBufferWithLeftSpare);

    PSTORAGE_PROPERTY_QUERY query = (PSTORAGE_PROPERTY_QUERY)dataBufferWithLeftSpare;
    query->PropertyId = StorageAdapterProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;
    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = dataType;
    protocolData->ProtocolDataRequestValue = requestValue;
    protocolData->ProtocolDataRequestSubValue = nsid;
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = dataBufferSize;

    int success = DeviceIoControl(handle,
                                  IOCTL_STORAGE_QUERY_PROPERTY,
                                  dataBufferWithLeftSpare,
                                  lengthOfDataBufferWithLeftSpare,
                                  dataBufferWithLeftSpare,
                                  lengthOfDataBufferWithLeftSpare,
                                  nullptr,
                                  nullptr
                                  );

    if (success)
    {
        PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)dataBufferWithLeftSpare;
        protocolData = &protocolDataDescr->ProtocolSpecificData;

        bool isValidDescriptorData = !(protocolDataDescr->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR) || protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR));
        bool isValidProtocolData = !(protocolData->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) || protocolData->ProtocolDataLength < dataBufferSize);

        return (isValidDescriptorData && isValidProtocolData);
    }

    return success;
}

bool identifyDevice(const DeviceHandle &handle, SKAlignedBuffer *dataBuffer, U32 cns, U32 nsid)
{
    int success = getStorageProperty(handle, dataBuffer, NVMeDataTypeIdentify, cns, nsid);
    if (success)
    {
        U16 vendorId = getWord(dataBuffer->ToDataBuffer(), IDENTIFY_CONTROLLER_VENDOR_ID_INDEX);
        U32 noOfNamespace = getU32(dataBuffer->ToDataBuffer(), IDENTIFY_CONTROLLER_NO_NAMESPACE_INDEX);

        return !(!vendorId && !noOfNamespace);
    }
    return success;
}

bool getInfoLogPage(const DeviceHandle &handle, SKAlignedBuffer *dataBuffer, U32 logId, U32 nsid)
{
    return getStorageProperty(handle, dataBuffer, NVMeDataTypeLogPage, logId, nsid);
}

bool downloadFirmware(const DeviceHandle &handle, U32 offset, SKAlignedBuffer *dataBuffer)
{
    SKAlignedBuffer buffer(sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD));
    PSTORAGE_HW_FIRMWARE_DOWNLOAD firmwareDownload = (PSTORAGE_HW_FIRMWARE_DOWNLOAD)buffer.ToDataBuffer();

    firmwareDownload->Version = sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD);
    firmwareDownload->Size = sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD);
    firmwareDownload->Offset = offset;
    firmwareDownload->BufferSize = dataBuffer->GetSizeInByte();
    firmwareDownload->Flags = STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER;
    CopyMemory(firmwareDownload->ImageBuffer, dataBuffer->ToDataBuffer(), dataBuffer->GetSizeInByte());

    int success = DeviceIoControl(handle,
                                  IOCTL_STORAGE_FIRMWARE_DOWNLOAD,
                                  buffer.ToDataBuffer(),
                                  buffer.GetSizeInByte(),
                                  buffer.ToDataBuffer(),
                                  buffer.GetSizeInByte(),
                                  nullptr,
                                  nullptr
                                  );

    return success;
}

bool commitFirmware(const DeviceHandle &handle, U8 slot)
{
    SKAlignedBuffer buffer(sizeof(STORAGE_HW_FIRMWARE_ACTIVATE));

    PSTORAGE_HW_FIRMWARE_ACTIVATE firmwareActivate = (PSTORAGE_HW_FIRMWARE_ACTIVATE)&buffer;
    firmwareActivate->Version = sizeof(STORAGE_HW_FIRMWARE_ACTIVATE);
    firmwareActivate->Flags = STORAGE_HW_FIRMWARE_REQUEST_FLAG_SWITCH_TO_EXISTING_FIRMWARE;
    firmwareActivate->Size = sizeof(STORAGE_HW_FIRMWARE_ACTIVATE);
    firmwareActivate->Slot = slot;

    int success = DeviceIoControl(handle,
                                  IOCTL_STORAGE_FIRMWARE_ACTIVATE,
                                  buffer.ToDataBuffer(),
                                  buffer.GetSizeInByte(),
                                  buffer.ToDataBuffer(),
                                  buffer.GetSizeInByte(),
                                  nullptr,
                                  nullptr
                                  );

    return success;
}

bool getSmart(const DeviceHandle &handle, SKAlignedBuffer *dataBuffer, U32 cns, U32 nsid)
{    
    return getStorageProperty(handle, dataBuffer, NVMeDataTypeLogPage, cns, nsid);
}

SKScsiCommandDesc *convertNvmeCommandDescToScsi(const SKNvmeCommandDesc *cmdDesc)
{
    if (SKNvmeCommandDesc::COMMAND_WRITE == cmdDesc->inputFields.OpCode)
    {
        if (false == cmdDesc->isExtCommand)
        {
            return SKScsiCommandDesc::createWrite10Desc(cmdDesc->inputFields.IOCommand.Lba, cmdDesc->inputFields.IOCommand.SectorCount);
        }
        else
        {
            return SKScsiCommandDesc::createWrite16Desc(cmdDesc->inputFields.IOCommand.Lba, cmdDesc->inputFields.IOCommand.SectorCount);
        }
    }
    else if (SKNvmeCommandDesc::COMMAND_READ == cmdDesc->inputFields.OpCode)
    {
        if (false == cmdDesc->isExtCommand)
        {
            return SKScsiCommandDesc::createRead10Desc(cmdDesc->inputFields.IOCommand.Lba, cmdDesc->inputFields.IOCommand.SectorCount);
        }
        else
        {
            return SKScsiCommandDesc::createRead16Desc(cmdDesc->inputFields.IOCommand.Lba, cmdDesc->inputFields.IOCommand.SectorCount);
        }
    }
}

bool write(const DeviceHandle &handle, const SKNvmeCommandDesc *cmdDesc, SKAlignedBuffer *dataBuffer)
{
    SKScsiSense lastSense;
    U16 lastError;
    SKReturnCode error = SKReturnCode::SKErrorUnknown;;
    SKScsiCommandDesc* scsiCmdDesc = convertNvmeCommandDescToScsi(cmdDesc);
    error = SKScsiProtocol::issueScsiCommand(handle, scsiCmdDesc, dataBuffer, lastError, lastSense);
    delete scsiCmdDesc;
    return SKReturnCode::SKSucceeded == error ? true : false;
}

bool read(const DeviceHandle &handle, const SKNvmeCommandDesc *cmdDesc, SKAlignedBuffer *dataBuffer)
{
    SKScsiSense lastSense;
    U16 lastError;
    SKReturnCode error = SKReturnCode::SKErrorUnknown;;
    SKScsiCommandDesc* scsiCmdDesc = convertNvmeCommandDescToScsi(cmdDesc);
    error = SKScsiProtocol::issueScsiCommand(handle, scsiCmdDesc, dataBuffer, lastError, lastSense);
    delete scsiCmdDesc;
    return SKReturnCode::SKSucceeded == error ? true : false;
}

int SKNvmeProtocol::getNsid(const std::string& devicePath, const DeviceHandle &handle)
{    
    int nsid = 0;
    SKAlignedBuffer dataBuffer(IDENTIFY_BUFFER_LENGTH);
    int retCode = identifyDevice(handle, &dataBuffer, NVME_IDENTIFY_CNS_CONTROLLER, 0);

    if (retCode)
    {
        U32 noOfNamespace = getU32(dataBuffer.ToDataBuffer(), IDENTIFY_CONTROLLER_NO_NAMESPACE_INDEX);
        if (1 == noOfNamespace) {
            // No namespace management
            nsid = 1;
        }
        else
        {
            nsid = 0;
        }
    }

    return nsid;
}

SKReturnCode SKNvmeProtocol::issueNvmeCommand(const DeviceHandle &handle, const SKNvmeCommandDesc *cmdDesc, SKAlignedBuffer *buffer, U16 &lastError)
{
    bool success;
    if (SKNvmeCommandDesc::SKNvmeFieldFormatting::COMMAND_ADMIN == cmdDesc->fieldFormatting)
    {
        if (SKNvmeCommandDesc::COMMAND_IDENTIFY_DEVICE == cmdDesc->inputFields.OpCode)
        {
            success = identifyDevice(handle, buffer, cmdDesc->inputFields.AdminCommand.Cdw10, cmdDesc->inputFields.AdminCommand.Nsid);
        }
        else if (SKNvmeCommandDesc::COMMAND_DOWNLOAD_FIRMWARE == cmdDesc->inputFields.OpCode)
        {
            U32 offset = cmdDesc->inputFields.AdminCommand.Cdw11 << 2;
            success = downloadFirmware(handle, offset, buffer);
        }
        else if (SKNvmeCommandDesc::COMMAND_ACTIVE_FIRMWARE == cmdDesc->inputFields.OpCode)
        {
            U8 slot = (cmdDesc->inputFields.AdminCommand.Cdw10 >> 3) & 0b111;
            success = commitFirmware(handle, slot);
        }
        else if (SKNvmeCommandDesc::COMMAND_GET_LOG_PAGE == cmdDesc->inputFields.OpCode)
        {
            U32 logTypeID = cmdDesc->inputFields.AdminCommand.Cdw10 & 0xFF;
            success = getInfoLogPage(handle, buffer, logTypeID, cmdDesc->inputFields.AdminCommand.Nsid);
        }
        else if (SKNvmeCommandDesc::COMMAND_SMART == cmdDesc->inputFields.OpCode)
        {
            success = getSmart(handle, buffer, cmdDesc->inputFields.AdminCommand.Cdw10, cmdDesc->inputFields.AdminCommand.Nsid);
        }
        else
        {
            int cmdBufferLen = sizeof(PSTORAGE_PROTOCOL_COMMAND) + buffer->GetSizeInByte();
            SKAlignedBuffer* cmdBuffer = new SKAlignedBuffer(cmdBufferLen);
            memset(cmdBuffer->ToDataBuffer(), 0, cmdBufferLen);

            // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ns-ntddstor-_storage_protocol_command?redirectedfrom=MSDN
            PSTORAGE_PROTOCOL_COMMAND protocolCommand = (PSTORAGE_PROTOCOL_COMMAND) cmdBuffer->ToDataBuffer();

            protocolCommand->Version = 0x1;  
            protocolCommand->Length = sizeof(STORAGE_PROTOCOL_COMMAND);  
            protocolCommand->ProtocolType = ProtocolTypeNvme;  
            protocolCommand->Flags = 0x80000000;  
            protocolCommand->CommandLength = 0x40;
            protocolCommand->ErrorInfoLength = sizeof(NVME_ERROR_INFO_LOG);  
            protocolCommand->DataFromDeviceTransferLength = cmdBufferLen;  
            protocolCommand->TimeOutValue = 10;  
            protocolCommand->ErrorInfoOffset = FIELD_OFFSET(STORAGE_PROTOCOL_COMMAND, Command) + 0x40;  
            protocolCommand->DataFromDeviceBufferOffset = protocolCommand->ErrorInfoOffset + protocolCommand->ErrorInfoLength;  
            protocolCommand->CommandSpecific = 0x01;

            PWIN_NVME_COMMAND command = (PWIN_NVME_COMMAND) protocolCommand->Command;

            command->CDW0.OPC = cmdDesc->inputFields.OpCode;
            command->u.GENERAL.CDW10 = cmdDesc->inputFields.AdminCommand.Cdw10;
            command->u.GENERAL.CDW12 = cmdDesc->inputFields.AdminCommand.Cdw12;


            return SKErrorInvalidCommand;
        }

        if (!success)
        {
            lastError = static_cast<U16>(GetLastError());
            return SKErrorIO;
        }

        return SKSucceeded;
    }
    else if (SKNvmeCommandDesc::SKNvmeFieldFormatting::COMMAND_IO == cmdDesc->fieldFormatting)
    {
        if(SKNvmeCommandDesc::COMMAND_WRITE == cmdDesc->inputFields.OpCode)
        {            
            success = write(handle, cmdDesc, buffer);
        }
        else if(SKNvmeCommandDesc::COMMAND_READ == cmdDesc->inputFields.OpCode)
        {
            success = read(handle, cmdDesc, buffer);
        }
        else
        {
            return SKErrorInvalidCommand;
        }

        if (!success)
        {
            lastError = static_cast<U16>(GetLastError());
            return SKErrorIO;
        }

        return SKSucceeded;
    }
}

SKNvmeProtocol::SKNvmeProtocol(const std::string &devicePath, const DeviceHandle &handle) :
    SKStorageProtocol(devicePath, handle)
{
}

SKNvmeProtocol::~SKNvmeProtocol()
{
}

SKReturnCode SKNvmeProtocol::issueCommand(const SKNvmeCommandDesc *cmdDesc, SKAlignedBuffer *buffer)
{
    return issueNvmeCommand(this->handle, cmdDesc, buffer, this->lastError);
}
