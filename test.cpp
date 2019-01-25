#include "app.h"
#include <iostream>
#include <string.h>

using namespace std;

#define ASSERT(func, cond) \
if(cond) \
    cout <<  func << ":TEST PASSED\n"; \
else { \
    cout << func << ":TEST FAILED\n"; \
    exit(1); \
}

void test1(void) {
    char *file = "file_test.dat";
    int obj1 = 10, obj2 = 20, test_obj1 = 0, test_obj2 = 0;
    _write(file, 0, sizeof(obj1), &obj1);
    _write(file, sizeof(obj1), sizeof(obj2), &obj2);
    _read(file, 0, sizeof(test_obj1), &test_obj1);
    _read(file, sizeof(test_obj1), sizeof(test_obj2), &test_obj2);

    ASSERT("test1", obj1 == test_obj1 && obj2 == test_obj2)
}

void test2(void) {
    char *file = "file_test.dat";
    char obj1 = 10, obj2 = 20, test_obj1 = 0, test_obj2 = 0;
    _write(file, 0, sizeof(obj1), &obj1);
    _write(file, sizeof(obj1), sizeof(obj2), &obj2);
    _read(file, 0, sizeof(test_obj1), &test_obj1);
    _read(file, sizeof(test_obj1), sizeof(test_obj2), &test_obj2);

    ASSERT("test2", obj1 == test_obj1 && obj2 == test_obj2)
}

void test3(void) {
    char *file = "file_test.dat";
    float obj1 = 10, obj2 = 20, test_obj1 = 0, test_obj2 = 0;
    _write(file, 0, sizeof(obj1), &obj1);
    _write(file, sizeof(obj1), sizeof(obj2), &obj2);
    _read(file, 0, sizeof(test_obj1), &test_obj1);
    _read(file, sizeof(test_obj1), sizeof(test_obj2), &test_obj2);

    ASSERT("test3", obj1 == test_obj1 && obj2 == test_obj2)
}

void test4(void) {
    char *file = "file_test.dat";
    char obj1 = 10, test_obj1 = 0;
    float obj2 = 20, test_obj2 = 0;
    _write(file, 0, sizeof(obj1), &obj1);
    _write(file, sizeof(obj1), sizeof(obj2), &obj2);
    _read(file, 0, sizeof(test_obj1), &test_obj1);
    _read(file, sizeof(test_obj1), sizeof(test_obj2), &test_obj2);

    ASSERT("test4", obj1 == test_obj1 && obj2 == test_obj2)
}

void test_cache1(void) {
    NVM mem("cache.dat", 1024, 10, 2);
    unsigned char data[1024] = {};
    unsigned char test_data[1024] = {};
    strcpy((char*)data, "CODE");
    mem.write(0, &data, 1024, 0);
    mem.read(0, &test_data, 1024, 0);

    mem.cache_flush();
    ASSERT("test_cache1", 0 == strcmp((const char*)test_data, (const char*)data))
}
void test_cache2(void) {
    NVM mem("cache.dat", 1024, 10, 2);
    unsigned char data[1024] = {};
    unsigned char test_data[1024] = {};
    strcpy((char*)data, "CODE");
    mem.read(0, &test_data, 1024, 0);
    ASSERT("test_cache2", 0 == strcmp((const char*)test_data, (const char*)data))
}

void test_cache3(void) {
    unsigned char data[1024] = {};
    unsigned char test_data[1024] = {};
    strcpy((char*)data, "BODE");
    NVM mem("cache.dat", 1024, 10, 2);
    mem.write(1, &data, 1024, 0);
    mem.read(0, &test_data, 1024, 0);
    mem.cache_flush();
    ASSERT("test_cache3", 0 == strcmp((const char*)test_data, "CODE"))
}

void test_cache4(void) {
    unsigned char test_data[1024] = {};
    NVM mem("cache.dat", 1024, 10, 2);
    mem.read(1, &test_data, 1024, 0);
    ASSERT("test_cache4", 0 == strcmp((const char*)test_data, "BODE"))
}

void test_cache5(void) {
    // overwrite detection
    unsigned char data[1024] = {};
    unsigned char test_data[1024] = {};
    strcpy((char*)data, "DODE");
    strcpy((char*)data+1024-strlen("AODE")-2, "AODE"); // take care of null terminator and checksum
    NVM mem("cache.dat", 1024, 10, 2);
    mem.write(2, &data, 1024, 0);
    mem.write(3, &data, 1024, 0);
    mem.cache_flush();
    mem.read(2, &test_data, 1024, 0);
    ASSERT("test_cache5:1", 0 == strcmp((const char*)test_data, "DODE"))
    ASSERT("test_cache5:2", 0 == strcmp((const char*)test_data+1024-strlen("AODE")-2, "AODE"))
}

void test_cache6(void) {
    // multiple page wrte read
    unsigned char data[2048] = {};
    unsigned char test_data[2048] = {};
    strcpy((char*)data, "CODE");
    strcpy((char*)data+1024, "DODE");
    NVM mem("cache.dat", 1024, 10, 2);
    mem.write(0, &data, sizeof(data), 0);
    mem.cache_flush();
    mem.read(0, &test_data, sizeof(test_data), 0);
    ASSERT("test_cache6:1", 0 == strcmp((const char*)test_data, "CODE"))
    ASSERT("test_cache6:2", 0 == strcmp((const char*)test_data+1024, "DODE"))
}

void test_cache7(void) {
    // offset check
    unsigned char data1[] = "BEAD", data2[] = "CEAD";
    unsigned char test_data[5] = {};
    NVM mem("cache.dat", 1024, 10, 2);
    mem.write(0, &data2, sizeof(data2), 10);
    mem.write(0, &data1, sizeof(data1), 0);
    mem.cache_flush();
    mem.read(0, &test_data, sizeof(test_data), 10);
    ASSERT("test_cache7:1", 0 == strcmp((const char*)test_data, "CEAD"))
    mem.read(0, &test_data, sizeof(test_data), 0);
    ASSERT("test_cache7:2", 0 == strcmp((const char*)test_data, "BEAD"))

    mem.write(0, &data1, sizeof(data1), 0);
    mem.cache_flush();
    mem.write(0, &data2, sizeof(data2), sizeof(data1));
    mem.cache_flush();
    mem.read(0, &test_data, sizeof(test_data), 0);
    ASSERT("test_cache7:3", 0 == strcmp((const char*)test_data, "BEAD"))
    mem.read(0, &test_data, sizeof(test_data), sizeof(data1));
    ASSERT("test_cache7:4", 0 == strcmp((const char*)test_data, "CEAD"))
}

void test_cache8(void) {
    // checksum override protection
    unsigned char data[1024] = {};
    unsigned char test_data[1024] = {};
    strcpy((char*)data, "DODE");
    strcpy((char*)data+1024-strlen("AODE")-1, "AODE"); // take care of null terminator
    NVM mem("cache.dat", 1024, 10, 2);
    mem.write(2, &data, 1024, 0);
    mem.write(3, &data, 1024, 0);
    mem.cache_flush();
    mem.read(2, &test_data, 1024, 0);
    ASSERT("test_cache8:1", 0 == strcmp((const char*)test_data, "DODE"))
    ASSERT("test_cache8:2", 0 != strcmp((const char*)test_data+1024-strlen("AODE")-1, "AODE"))
}

void test_cache9(void) {
    // out of memory check
    {
        NVM mem("cache.dat", 1024, 1, 1);
        unsigned char data[2056] = "CODE";
        ASSERT("test_cache9:1", gpNvm_Result::OUT_OF_MEM == mem.write(0, &data, sizeof(data), 0))
    }
    {
        NVM mem("cache.dat", 1024, 2, 1);
        unsigned char data[3096] = "CODE";
        ASSERT("test_cache9:2", gpNvm_Result::OUT_OF_MEM == mem.write(0, &data, sizeof(data), 0))
    }

}

void test_cache10(void) {
    // redundant page writing check
    char *file = "cache.dat";
    NVM mem(file, 1024, 4, 2);
    unsigned char data[] = "ABAB";
    unsigned char test_data[5] = {};
    mem.write(1, &data, sizeof(data), 0);
    mem.cache_flush();

    _read(file, 1024*1, sizeof(test_data), &test_data);
    ASSERT("test_cache10:1", 0 == strcmp((const char*)data, (const char*)test_data))
    _read(file, 1024*3, sizeof(test_data), &test_data);
    ASSERT("test_cache10:2", 0 == strcmp((const char*)data, (const char*)test_data))
}

void test_attr_1(void) {
    ATTR_TANK tank;

    meta_t meta, test_meta;
    meta.current_page = 1 + (sizeof(meta_t) / PAGE_SIZE);
    meta.current_offset = 0;
    strcpy((char*)meta.INIT_SEQ, "CODE");
    memset(meta.ATTR_MAP, 0, sizeof(meta.ATTR_MAP));

    NVM mem("ATTR_TANK.dat", 1024, 10, 2);
    mem.read(0, &test_meta, sizeof(test_meta), 0);

    ASSERT("test_attr_1:1", 0 == memcmp(&meta.INIT_SEQ, &test_meta.INIT_SEQ, sizeof(meta.INIT_SEQ)))
    ASSERT("test_attr_1:2", 0 == memcmp(&meta.current_page, &test_meta.current_page, sizeof(meta.current_page)))
}

void test_attr_2(void) {
    {
        ATTR_TANK tank;
        unsigned char data = 0xBC;
        tank.set_attribute(10, sizeof(data), &data);
    }

    {
        ATTR_TANK tank;
        unsigned char data = 0xBC, test_data = 0, length = 0;
        tank.get_attribute(10, &length, &test_data);
        ASSERT("test_attr_2:1", data == test_data)
        ASSERT("test_attr_2:2", length == sizeof(data))
    }
}

void test_attr_3(void) {
    unsigned long int data = 0xBECEDEAE, test_data = 0;
    unsigned char length = 0;
    gpNvm_SetAttribute(11, sizeof(data), (UInt8*)&data);
    gpNvm_GetAttribute(11, &length, (UInt8*)&test_data);

    ASSERT("test_attr_3:1", data == test_data)
    ASSERT("test_attr_3:2", length == sizeof(data))
}

void test_attr_4(void) {
    unsigned char data1 = 0xBE, data2 = 0xAC, test_data = 0;
    unsigned char length = 0;
    gpNvm_SetAttribute(0, sizeof(data1), (UInt8*)&data1);
    gpNvm_SetAttribute(1, sizeof(data2), (UInt8*)&data2);
    gpNvm_SetAttribute(2, sizeof(data2), (UInt8*)&data2);
    gpNvm_SetAttribute(3, sizeof(data1), (UInt8*)&data1);

    gpNvm_GetAttribute(0, &length, (UInt8*)&test_data);
    printf("%x %x\n", data1, test_data);
    ASSERT("test_attr_4:1", data1 == test_data)
    ASSERT("test_attr_4:2", length == sizeof(data1))

    gpNvm_GetAttribute(1, &length, (UInt8*)&test_data);
    ASSERT("test_attr_4:3", data2 == test_data)
    ASSERT("test_attr_4:4", length == sizeof(data2))

    gpNvm_GetAttribute(2, &length, (UInt8*)&test_data);
    ASSERT("test_attr_4:3", data2 == test_data)
    ASSERT("test_attr_4:4", length == sizeof(data2))

    gpNvm_GetAttribute(3, &length, (UInt8*)&test_data);
    ASSERT("test_attr_4:3", data1 == test_data)
    ASSERT("test_attr_4:4", length == sizeof(data1))
}

void test_mem_1(void) {
    char *file = "mem_corruption.dat";
    NVM mem(file, 1024, 10, 2, false);
    unsigned char data[1024] = "CODE";
    unsigned char test_data[1024] = {};

    // remove corrutpion from previous runs of test
    _write(file, 0, sizeof(test_data), &test_data);

    mem.write(0, &data, sizeof(data), 0);
    mem.cache_flush();
    mem.read(0, &test_data, sizeof(test_data), 0);
    ASSERT("test_mem_1:1", 0 == strcmp((const char*)test_data, "CODE"))
    // corrupt the mem
    data[0] = 'B';
    _write(file, 0, sizeof(data[0]), &data[0]);
    ASSERT("test_mem_1:2", gpNvm_Result::MEM_CORRUPTION == mem.read(0, &test_data, sizeof(test_data), 0))
}

void test_mem_2(void) {
    char *file = "mem_correction.dat";
    NVM mem(file, 1024, 10, 2);
    unsigned char data[1024] = "CODE";
    unsigned char test_data[1024] = {};

    mem.write(0, &data, sizeof(data), 0);
    mem.cache_flush();
    mem.read(0, &test_data, sizeof(test_data), 0);
    ASSERT("test_mem_2:1", 0 == strcmp((const char*)test_data, "CODE"))

    // corrupt the mem
    data[0] = 'B';
    _write(file, 0, sizeof(data[0]), &data[0]);

    // corruption should be fixed
    memset(&test_data, 0, sizeof(test_data));
    mem.read(0, &test_data, sizeof(test_data), 0);
    ASSERT("test_mem_2:2", 0 == strcmp((const char*)test_data, "CODE"))
}

int main(void) {
    cout << "File read/write tests\n";
    test1();
    test2();
    test3();
    test4();

    cout << "Cache read/write tests\n";
    test_cache1();
    test_cache2();
    test_cache3();
    test_cache4();
    test_cache5();
    test_cache6();
    test_cache7();
    test_cache8();
    test_cache9();
    test_cache10();

    cout << "ATTR_TANK tests\n";
    test_attr_1();
    test_attr_2();
    test_attr_3();
    test_attr_4();

    cout << "Mem corruption tests\n";
    test_mem_1();
    test_mem_2();

    cout << "All tests passed\n";
    return 0;
}
