#ifndef __MAVERICK_H__
#define __MAVERICK_H__

#include <iostream>

#include "commands.h"

/**
 * Set the over provisioning for the drive. The valid range is 0% thru 50%.
 * Command usage: vtnvme set-op DEVICE OP%
 * 
 * Returns 0 on success or non-zero on failure.
 */ 
int setOP(int argc, char** argv);

/**
 * Prints the over provisioning percentage for the given drive.
 * Command usage: vtnvme get-op DEVICE
 * 
 * returns 0 on success and non-zero on failure
 */ 
int getOP(int argc, char** argv);

#endif
