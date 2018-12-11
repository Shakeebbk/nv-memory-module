#include <iostream>
#include <fstream>
#include "app.h"

using namespace std;

gpNvm_Result _write(char *dev, size_t offset, size_t length, void *data) {
    fstream file;
    file.open(dev, ios::binary | std::ios_base::in | std::ios_base::out);
    if(!file) {
        return gpNvm_Result::DEVICE_FAIL;
    }
    file.seekp(offset, std::ios_base::beg);
    file.write((const char*)data, length);
    file.close();
    return gpNvm_Result::SUCCESS;
}
gpNvm_Result _read(char *dev, size_t offset, size_t length, void *data) {
    fstream file;
    file.open(dev, ios::binary | std::ios_base::in);
    if(!file) {
        return gpNvm_Result::DEVICE_FAIL;
    }
    file.seekg(offset, std::ios_base::beg);
    file.read((char*)data, length);
    file.close();
    return gpNvm_Result::SUCCESS;
}

gpNvm_Result gpNvm_GetAttribute(gpNvm_AttrId attrId, UInt8 *length, UInt8 *pValue) {
    ATTR_TANK tank;
    return tank.get_attribute(attrId, length, pValue);
}
gpNvm_Result gpNvm_SetAttribute(gpNvm_AttrId attrId, UInt8 length, UInt8 *pValue) {
    ATTR_TANK tank;
    return tank.set_attribute(attrId, length, pValue);
}
