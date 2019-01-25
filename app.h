#pragma once
#include <stddef.h>
#include <cstring>

#include <iostream>

typedef unsigned char UInt8;
typedef UInt8 gpNvm_AttrId;
// typedef UInt8 gpNvm_Result;

typedef UInt8 pageId;

enum class gpNvm_Result : UInt8 {
    SUCCESS,
    PAGE_FAULT,
    DEVICE_FAIL,
    MEM_CORRUPTION,
    OUT_OF_MEM
};

/* @brief Write data to the underlying memory device
 *
 * @param[in] dev    - device to write to
 * @param[in] offset - offset in memory
 * @param[in] length - length of data to be written
 * @param[in] data   - pointer to the data to be written
 *
 * @return gpNvm_Result
 */
gpNvm_Result _write(char *dev, size_t offset, size_t length, void *data);

/* @brief Read data from the underlying memory device
 *
 * @param[in] dev    - device to read from
 * @param[in] offset - offset in memory
 * @param[in] length - length of data to be read
 * @param[in] data   - pointer to fill the read data
 *
 * @return gpNvm_Result
 */
gpNvm_Result _read(char *dev, size_t offset, size_t length, void *data);

typedef struct CacheElement {
    bool keep;
    size_t pageId;
    bool updated;
    UInt8 *mem;
} cache_t;

/* @brief Abstraction of Non volatile memory with
 * paging, caching, error detection and correction support
 */
class NVM {
private:
    char *dev; // memory device
    size_t raw_page_size; // page size in bytes
    size_t data_page_size; // logical page size for data
    size_t checksum_size;
    size_t num_pages; // total number of logical pages
    size_t num_redundant_pages; // total number of logical pages with redundancy
    bool with_redundancy;
    size_t cache_size; // cache size in num of pages in cache
    cache_t *cache;

    /* @brief Get a page from cache
     *
     * @param[in] pageId    - logical page id
     *
     * @return index of cache element if present, else -1
     */
    int get_page_from_cache(size_t pageId) {
        for(int i = 0; i < cache_size; i++) {
            if(cache[i].pageId == pageId) {
                return i;
            }
        }
        return -1;
    }

    /* @brief Swap the given page in to cache
     *
     * @param[in] pageId    - logical page id to cache
     * @param[out] c        - index of cache element where the page is cached in
     *
     * @return gpNvm_Result
     */
    gpNvm_Result swap_page(size_t pageId, int &c) {
        // TODO: Implement one of the caching algorithms like LRU
        gpNvm_Result rc = gpNvm_Result::PAGE_FAULT;
        for(int i = 0; i < cache_size; i++) {
            // find a page which can be cached out
            if(!cache[i].keep) {
                rc = gpNvm_Result::SUCCESS;
                // write it to memory only if there are updates
                if(cache[i].updated) {
                    // Update checksum
                    UInt8 chksum = chksum8(cache[i].mem, data_page_size);
                    cache[i].mem[raw_page_size-checksum_size] = chksum;
                    rc = _write(dev, cache[i].pageId * raw_page_size, raw_page_size, cache[i].mem);
                    if(with_redundancy) {
                        rc = _write(dev, (cache[i].pageId + num_redundant_pages) * raw_page_size, raw_page_size, cache[i].mem);
                        if(rc != gpNvm_Result::SUCCESS) {
                            break;
                        }
                    }
                }
                // swap in the requested page
                if(rc == gpNvm_Result::SUCCESS) {
                    memset(cache[i].mem, 0, raw_page_size);
                    rc = _read(dev, pageId * raw_page_size, raw_page_size, cache[i].mem);
                    if(rc == gpNvm_Result::SUCCESS) {
                        // Check for mem corruption
                        UInt8 chksum = chksum8(cache[i].mem, data_page_size);
                        if(chksum != cache[i].mem[raw_page_size-checksum_size]) {
                            if(with_redundancy) {
                                // read from redundant page
                                rc = _read(dev, (pageId + num_redundant_pages) * raw_page_size, raw_page_size, cache[i].mem);
                                if(rc == gpNvm_Result::SUCCESS) {
                                    chksum = chksum8(cache[i].mem, data_page_size);
                                    if(chksum != cache[i].mem[raw_page_size-checksum_size]) {
                                        return gpNvm_Result::MEM_CORRUPTION;
                                    }
                                    // write back to corrutped page - mem correction
                                    rc = _write(dev, cache[i].pageId * raw_page_size, raw_page_size, cache[i].mem);
                                }
                            }
                            else {
                                return gpNvm_Result::MEM_CORRUPTION;
                            }
                        }
                    }
                }
                if(rc == gpNvm_Result::SUCCESS) {
                    cache[i].pageId = pageId;
                    c = i;
                }
                break;
            }
        }
        return rc;
    }

    // Using 1byte checksum - ref https://stackoverflow.com/questions/31151032/writing-an-8-bit-checksum-in-c
    UInt8 chksum8(const unsigned char *buff, size_t len) {
        unsigned int sum;       // nothing gained in using smaller types!
        for ( sum = 0 ; len != 0 ; len-- )
            sum += *(buff++);   // parenthesis not required!
        return (UInt8)sum;
    }
public:
    /* @brief Constructor
     *
     * @param[in] i_dev                 - memory device
     * @param[in] i_page_size           - page size in bytes
     * @param[in] i_num_pages           - total number of pages
     * @param[in] i_cache_size          - cache size in number of pages
     * @param[in] i_with_mem_correction - if memory corruption correction is required, by default turned on
     *
     * @return gpNvm_Result
     */
    NVM(char *i_dev, size_t i_page_size, size_t i_num_pages, size_t i_cache_size, bool i_with_mem_correction=true) {
        dev         = i_dev;
        raw_page_size   = i_page_size;
        num_pages   = i_num_pages;
        cache_size  = i_cache_size;
        cache       = new cache_t[cache_size];
        with_redundancy = i_with_mem_correction;
        for(int i = 0; i < cache_size; i++) {
            cache[i].keep    = 0;
            cache[i].pageId  = num_pages; // one past last page as invalid id, because 0 is valid page
            cache[i].updated = 0;
            cache[i].mem     = new UInt8[raw_page_size];
        }
        checksum_size = sizeof(UInt8);
        data_page_size = raw_page_size - checksum_size;
        if(with_redundancy) {
            num_pages = num_pages - (num_pages%2); // round of the pages to multiples of 2
            num_redundant_pages = num_pages/2;
        }
        else {
            num_redundant_pages = num_pages;
        }
    }

    ~NVM() {
        for(int i = 0; i < cache_size; i++) {
            delete []cache[i].mem;
        }
        delete []cache;
    }

    /* @brief Read memory at a given offset and of given length starting from a given page
     *
     * @param[in] pageId        - logical page id
     * @param[out] mem          - data pointer to read the memory into
     * @param[in] len           - number of bytes to be read
     * @param[in] offset        - offset in page where the read should begin from
     *
     * @return gpNvm_Result
     */
    gpNvm_Result read(size_t pageId, void *mem, size_t len, size_t offset=0) {
        size_t pages_read = 0;
        while(len) {
            if(pageId >= num_pages) {
                return gpNvm_Result::OUT_OF_MEM;
            }
            // see if the requested page is in cache
            int c = get_page_from_cache(pageId);
            if(c < 0) {
                // swap in the page if required
                gpNvm_Result rc = swap_page(pageId, c);
                if(rc != gpNvm_Result::SUCCESS) {
                    return rc;
                }
            }

            // calculate the bytes of relevant data in the current page
            size_t bytes = (len > data_page_size) ? (data_page_size - offset) : len;
            memcpy((UInt8*)mem+(pages_read * data_page_size), cache[c].mem+offset, bytes);
            len -= bytes;
            pageId++;
            pages_read++;
            offset = 0; // since data is contiguous it has to begin from 0 of next page
        }

        return gpNvm_Result::SUCCESS;
    }

    /* @brief Write data to the memory at a given offset and of given length starting from a given page
     *
     * @param[in] pageId        - logical page id
     * @param[out] mem          - data to be written
     * @param[in] len           - number of bytes to be written
     * @param[in] offset        - offset in page where the write should begin from
     *
     * @return gpNvm_Result
     */
    gpNvm_Result write(size_t pageId, void *mem, size_t len, size_t offset=0) {
        size_t pages_written = 0;
        while(len) {
            if(pageId >= num_pages) {
                return gpNvm_Result::OUT_OF_MEM;
            }
            // see if the requested page is in cache
            int c = get_page_from_cache(pageId);
            if(c < 0) {
                // swap in the page if required
                gpNvm_Result rc = swap_page(pageId, c);
                if(rc != gpNvm_Result::SUCCESS) {
                    return rc;
                }
            }
            // mark as updated to that next cache flush commits it to memory
            cache[c].updated = true;
            // calculate the bytes of relevant data in the current page
            size_t bytes = (len > data_page_size) ? (data_page_size-offset) : len;
            memcpy(cache[c].mem+offset, (UInt8*)mem+(pages_written * data_page_size), bytes);
            len -= bytes;
            pageId++;
            pages_written++;
            offset = 0; // since data is contiguous it has to begin from 0 of next page
        }

        return gpNvm_Result::SUCCESS;
    }

    /* @brief Commit the cache contents on to the memory device
     *
     * @return gpNvm_Result
     */
    gpNvm_Result cache_flush(void) {
        gpNvm_Result rc = gpNvm_Result::SUCCESS;
        for(int i = 0; i < cache_size; i++) {
            if(cache[i].updated) {
                // Update checksum
                UInt8 chksum = chksum8(cache[i].mem, data_page_size);
                cache[i].mem[raw_page_size-checksum_size] = chksum;
                rc = _write(dev, cache[i].pageId * raw_page_size, raw_page_size, cache[i].mem);
                if(rc != gpNvm_Result::SUCCESS) {
                    break;
                }
                if(with_redundancy) {
                    rc = _write(dev, (cache[i].pageId + num_redundant_pages) * raw_page_size, raw_page_size, cache[i].mem);
                    if(rc != gpNvm_Result::SUCCESS) {
                        break;
                    }
                }
            }
        }
        return rc;
    }

    /* @brief Get actual page size in memory
     *
     * @return page size in bytes
     */
    size_t get_page_size(void) {
        return data_page_size;
    }
};

#define ATTR_TANK_DEV "ATTR_TANK.dat"
#define MAX_ATTRIBUTES 256
#define PAGE_SIZE 1024
#define NUM_PAGES 50
#define CACHE_SIZE 2

typedef struct {
    size_t len;
    size_t page;
    size_t offset;
} attr_info_t;

typedef struct {
    size_t current_page;
    size_t current_offset;
    uint8_t INIT_SEQ[5];
    attr_info_t ATTR_MAP[MAX_ATTRIBUTES];
} meta_t;

/* ATTR_TANK - an abstraction for the attributes container,
 * providing init, getter and setter methods
 */
class ATTR_TANK {
private:
    meta_t meta;
    NVM *mem;
public:
    ATTR_TANK() {
        mem = new NVM(ATTR_TANK_DEV, PAGE_SIZE, NUM_PAGES, CACHE_SIZE);

        // read page 0
        mem->read(0, &meta, sizeof(meta), 0);

        if(strcmp((const char*)meta.INIT_SEQ, "CODE")) {
            // First time init
            meta.current_page = 1 + (sizeof(meta_t) / mem->get_page_size());
            meta.current_offset = 0;
            strcpy((char*)meta.INIT_SEQ, "CODE");
            memset(meta.ATTR_MAP, 0, sizeof(meta.ATTR_MAP));

            // write to NVM
            mem->write(0, &meta, sizeof(meta), 0);
            mem->cache_flush();
        }
    }

    ~ATTR_TANK() {
        delete mem;
    }

    gpNvm_Result set_attribute(gpNvm_AttrId attrId, UInt8 length, UInt8 *pValue) {
        gpNvm_Result rc = gpNvm_Result::SUCCESS;

        do {
            // if attribute is previously set or not
            // If the attribute is set with data longer than current one, we simply
            // acquire another memory block and update the attribute there, abadoning
            // the earlier one. This can be improved to reclaim such memory holes
            if(meta.ATTR_MAP[attrId].len < length) {
                // create a space
                meta.ATTR_MAP[attrId].len = length;
                meta.ATTR_MAP[attrId].page = meta.current_page;
                meta.ATTR_MAP[attrId].offset = meta.current_offset;
                meta.current_page = ((meta.current_page * mem->get_page_size()) + meta.current_offset + length) / mem->get_page_size();
                meta.current_offset = ((meta.current_page * mem->get_page_size()) + meta.current_offset + length) % mem->get_page_size();
                rc = mem->write(0, &meta, sizeof(meta), 0);
                if(rc !=  gpNvm_Result::SUCCESS) {
                    break;
                }
            }

            rc = mem->write(meta.ATTR_MAP[attrId].page, pValue, length, meta.ATTR_MAP[attrId].offset);
            if(rc != gpNvm_Result::SUCCESS) {
                break;
            }
            // TODO: create a separate task to commit - minimizing write cycles
            rc = mem->cache_flush();
            if(rc != gpNvm_Result::SUCCESS) {
                break;
            }
        } while(0);

        return (gpNvm_Result)rc;
    }

    gpNvm_Result get_attribute(gpNvm_AttrId attrId, UInt8 *length, UInt8 *pValue) {
        *length = meta.ATTR_MAP[attrId].len;
        return (gpNvm_Result)mem->read(meta.ATTR_MAP[attrId].page, pValue, *length, meta.ATTR_MAP[attrId].offset);
    }

};

gpNvm_Result gpNvm_GetAttribute(gpNvm_AttrId attrId, UInt8 *length, UInt8 *pValue);
gpNvm_Result gpNvm_SetAttribute(gpNvm_AttrId attrId, UInt8 length, UInt8 *pValue);
