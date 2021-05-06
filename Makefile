#Target
TARGET			= vtnvme
# Flags
CXXFLAGS		= -std=c++14 -O2 -MD -Wall -Wextra -g -fPIC
DEFINES         = -D__KERNEL__ 

# Compiler
CXX				= g++

# Define directories
BUILD_DIR		= ./build

CFLAGS			= -I. -Isrc/ -Isrc/storkit/include

# Files
SOURCES			+= \
	src/vt-nvme.cpp \
	src/commands.cpp \
	src/maverick.cpp \
	src/storkit/include/StorageKitAlignedBuffer.cpp \
	src/storkit/include/StorageKitBufferParser.cpp \
	src/storkit/include/StorageKitParser.cpp \
	src/storkit/include/StorageKitStorageDeviceUtils.cpp \
	src/storkit/include/StorageKitStringUtility.cpp \
	src/storkit/descriptor/StorageKitCommandDesc.cpp \
	src/storkit/descriptor/StorageKitAtaCommandDesc.cpp \
	src/storkit/descriptor/StorageKitScsiCommandDesc.cpp \
	src/storkit/descriptor/StorageKitNvmeCommandDesc.cpp \
	src/storkit/descriptor/StorageKitU9VcCommandDesc.cpp \
	src/storkit/storage/StorageKitAtaDevice.cpp \
	src/storkit/storage/StorageKitSatDevice.cpp \
	src/storkit/storage/StorageKitHypDevice.cpp \
	src/storkit/storage/StorageKitScsiDevice.cpp \
	src/storkit/storage/StorageKitStorageDevice.cpp \
	src/storkit/storage/StorageKitNvmeDevice.cpp \
	src/storkit/protocol/StorageKitScsiProtocolCommon.cpp \
	src/storkit/protocol/StorageKitNvmeProtocolCommon.cpp \

ifeq ($(OS), Windows_NT)
	DEFINES += -DWIN32
	SOURCES			+= \
	src/storkit/protocol/windows/StorageKitAtaProtocol.cpp \
	src/storkit/protocol/windows/StorageKitScsiProtocol.cpp \
	src/storkit/protocol/windows/StorageKitStorageProtocol.cpp \
	src/storkit/protocol/windows/StorageKitNvmeProtocol.cpp
else
	DEFINES += -D__LINUX__
	SOURCES			+= \
	src/storkit/protocol/linux/StorageKitAtaProtocol.cpp \
	src/storkit/protocol/linux/StorageKitScsiProtocol.cpp \
	src/storkit/protocol/linux/StorageKitStorageProtocol.cpp \
	src/storkit/protocol/linux/StorageKitNvmeProtocol.cpp
endif

OBJECTS			= $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

.PHONY: dirs all clean
all: dirs $(BUILD_DIR)/$(TARGET)

dirs:
	@echo "Create directories"
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(dir $(OBJECTS))

$(BUILD_DIR)/%.o: %.cpp
	@echo "Compiling for $(OS): $< --> $@"
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(CFLAGS) $(DEFINES)

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@echo "Linking: $@"
	@$(CXX) -o $@ $^ $(LFLAGS) $(DEFINES) $(LIBS)
	@echo "Executable file: $@"

clean:
	@echo "Clean objects"
	@rm -rf $(BUILD_DIR)
	@rm -rf $(dir $(OBJECTS))
