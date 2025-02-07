#include <new>
#include <cstdlib>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef MEMPLUMBER_FILENAME_LEN
#define MEMPLUMBER_FILENAME_LEN  100
#endif

#ifndef MEMPLUMBER_HASHTABLE_SIZE
#define MEMPLUMBER_HASHTABLE_SIZE 16384
#endif

#ifndef MEMPLUMBER_HASH
#define MEMPLUMBER_HASH(p) (((unsigned long)(p) >> 8) % MEMPLUMBER_HASHTABLE_SIZE)
#endif

#ifndef _THROW_BAD_ALLOC
#define _THROW_BAD_ALLOC
#endif

#ifndef _NOEXCEPT
#define _NOEXCEPT
#endif

class MemPlumberInternal {
    private:
    
    struct new_ptr_list_t {
        new_ptr_list_t* next;
        char file[MEMPLUMBER_FILENAME_LEN];
        int line;
        size_t size;
    };

    new_ptr_list_t* m_PointerListHashtable[MEMPLUMBER_HASHTABLE_SIZE];
    new_ptr_list_t* m_StaticPointerListHashtable[MEMPLUMBER_HASHTABLE_SIZE];

    bool m_Started;
    int m_ProgramStarted;
    bool m_Verbose;
    FILE* m_Dumper;

    // private c'tor
    MemPlumberInternal() {
        m_Started = false;
        m_Verbose = false;

        // zero the hashtables
        for (int i = 0; i < MEMPLUMBER_HASHTABLE_SIZE; i++) {
            m_PointerListHashtable[i] = NULL;
            m_StaticPointerListHashtable[i] = NULL;
        }

        #ifdef COLLECT_STATIC_VAR_DATA
        m_ProgramStarted = 0;
        #else
        m_ProgramStarted = -1;
        #endif //COLLECT_STATIC_VAR_DATA
    }

    FILE* openFile(const char* fileName, bool append) {
        if (strncmp(fileName, "", 1000) == 0) { // dump to stdout
            return stdout;
        }
        else { // dump to file
            FILE* file = NULL;
            if (!append) { // override the file
                file = fopen(fileName, "wt");
            }
            else { // append the file
                file = fopen(fileName, "at"); // try append
                if (!file) { // if append failed, create a new file
                    file = fopen(fileName, "wt");
                }
            }

            if (!file) {
                fprintf(stderr, "WARNING: couldn't open file `%s`\n", fileName);
            }

            return file;
        }
    }

    void closeFile(FILE* file) {
        if (file) {
            fflush(file);
            if (file != stdout) {
                fclose(file);
            }
        }
    }
    
    bool isVerbose() {
        return m_Verbose && m_Dumper != NULL;
    }

    public:

    static MemPlumberInternal& getInstance() {
        static MemPlumberInternal instance;
        return instance;
    }

    void* allocateMemory(std::size_t size, const char* file, int line) {

        // if not started, allocate memory and exit
        if (m_ProgramStarted != 0 && !m_Started) {
            if (isVerbose()) {
                fprintf(m_Dumper, "Request for memory allocation before program started\n");
            }
            return malloc(size);
        }

        // total memory to allocated is the requested size + metadata size
        size_t totalSizeToAllocate = size + sizeof(new_ptr_list_t);

        // allocated memory
        new_ptr_list_t* pointerMetaDataRecord = (new_ptr_list_t*)malloc(totalSizeToAllocate);
        memset(pointerMetaDataRecord, 0, sizeof(new_ptr_list_t));

        // if cannot allocate, return NULL
        if (pointerMetaDataRecord == NULL)
            return pointerMetaDataRecord;
        
        // calculate the actual pointer to provide to the user
        void* actualPointer = (char*)pointerMetaDataRecord + sizeof(new_ptr_list_t);

        // find the hash index for this pointer
        size_t hashIndex = MEMPLUMBER_HASH(actualPointer);

        new_ptr_list_t** hashtable = (m_ProgramStarted == 0 ? m_StaticPointerListHashtable : m_PointerListHashtable);

        // chain this metadata to the linked list of the specific bucket 
        pointerMetaDataRecord->next = hashtable[hashIndex];

        // fill in the metadata
        pointerMetaDataRecord->line = line;
        pointerMetaDataRecord->size = size;
        strncpy(pointerMetaDataRecord->file, file, MEMPLUMBER_FILENAME_LEN - 1);
		pointerMetaDataRecord->file[MEMPLUMBER_FILENAME_LEN - 1] = '\0';

        // put this metadata in the head of the list
        hashtable[hashIndex] = pointerMetaDataRecord;

        if (isVerbose()) {
            if (m_ProgramStarted == 0) {
                fprintf(m_Dumper, "Allocate static variable: %d[bytes] in 0x%p in %s:%d\n", (int)size, actualPointer, file, line);
            } else {
                fprintf(m_Dumper, "Allocate: %d[bytes] in 0x%p in %s:%d\n", (int)size, actualPointer, file, line);
            }
        }

        return actualPointer;
    }

    void freeMemory(void* pointer, const char* file, int line) {

        if (pointer == NULL) {
            return;
        }

        // find the metadata record bucket in the hash table
        size_t hashIndex = MEMPLUMBER_HASH(pointer);
        new_ptr_list_t* metaDataBucketLinkedListElement = m_PointerListHashtable[hashIndex];
	    new_ptr_list_t* metaDataBucketLinkedListPrevElement = NULL;

        // inside the bucket, go over the linked list until you find the specific pointer
        while (metaDataBucketLinkedListElement != NULL) {

            // get the actual pointer from the record
            void* actualPointerInRecord = (char*)metaDataBucketLinkedListElement + sizeof(new_ptr_list_t);

            // if this is not the pointer we're looking for - continue the search
            if (actualPointerInRecord != pointer) {
                metaDataBucketLinkedListPrevElement = metaDataBucketLinkedListElement;
                metaDataBucketLinkedListElement = metaDataBucketLinkedListElement->next;
                continue;
            }
            else { // this is the pointer we're looking for

                // remove the current element from the linked list
                if (metaDataBucketLinkedListPrevElement == NULL) { // this is the first item in the list
                    m_PointerListHashtable[hashIndex] = metaDataBucketLinkedListElement->next;
                }
                else { // this is not the first item in the list
                    metaDataBucketLinkedListPrevElement->next = metaDataBucketLinkedListElement->next;
                }

                if (isVerbose()) {
                    fprintf(m_Dumper, "Free: 0x%p (size %d[bytes]) allocated in: %s:%d\n", 
                        pointer,
                        (int)metaDataBucketLinkedListElement->size,
                        metaDataBucketLinkedListElement->file, 
                        metaDataBucketLinkedListElement->line);
                }

                // free the memory of the current item
                free(metaDataBucketLinkedListElement);

                return;
            }
        }

        // if got to here it means memory was allocated before monitoring started. Simply free the memory and return 
        if (isVerbose()) {
            fprintf(m_Dumper, "Pointer 0x%p wasn't found\n", pointer);
        }

        free(pointer);
    }

    void programStarted() {
        m_ProgramStarted = 1;
    }

    void start(bool verbose, const char* fileDumperName, bool append) {
        m_Started = true;
        m_Verbose = verbose;
        m_Dumper = openFile(fileDumperName, append);
    }

    void stop(bool closeDumper = true) {
        m_Started = false;
        if (closeDumper) {
            closeFile(m_Dumper);
            m_Dumper = NULL;
        }
    }

    void checkLeaks(size_t& memLeakCount, uint64_t& memLeakSize, bool verbose, const char* fileDumperName, bool append) {

        memLeakCount = 0;
        memLeakSize = 0;

        FILE* dumper = NULL;
        if (verbose) {
            dumper = openFile(fileDumperName, append);
        }

        // go over all buckets in the hashmap
        for (int index = 0; index < MEMPLUMBER_HASHTABLE_SIZE; ++index) {
            new_ptr_list_t* metaDataBucketLinkedListElement = m_PointerListHashtable[index];

            // if bucket is empty - continue
            if (metaDataBucketLinkedListElement == NULL) {
                continue;
            }

            // go over all of the elements in the link list in this bucket
            while (metaDataBucketLinkedListElement != NULL) {

                memLeakCount++;
                memLeakSize += (uint64_t)metaDataBucketLinkedListElement->size;

                if (verbose) {
                    fprintf(dumper, "Found leaked object at 0x%p (size %d[bytes]) allocated in: %s:%d\n",
                        (char*) metaDataBucketLinkedListElement + sizeof(new_ptr_list_t), 
                        (int) metaDataBucketLinkedListElement->size,
                        metaDataBucketLinkedListElement->file, 
                        metaDataBucketLinkedListElement->line);
                }

                // go to the next item on the list
                metaDataBucketLinkedListElement = metaDataBucketLinkedListElement->next;
            }
        }

        closeFile(dumper);
    }

    void staticMemAllocation(size_t& memCount, uint64_t& memSize, bool verbose, const char* fileDumperName, bool append) {
        memCount = 0;
        memSize = 0;

        FILE* dumper = NULL;
        if (verbose) {
            dumper = openFile(fileDumperName, append);
        }

        for (int index = 0; index < MEMPLUMBER_HASHTABLE_SIZE; ++index) {
            new_ptr_list_t* metaDataBucketLinkedListElement = m_StaticPointerListHashtable[index];

            // if bucket is empty - continue
            if (metaDataBucketLinkedListElement == NULL) {
                continue;
            }

            // go over all of the elements in the link list in this bucket
            while (metaDataBucketLinkedListElement != NULL) {

                if (verbose) {
                    fprintf(dumper, "Static object allocated at 0x%p (size %d[bytes]) allocated in: %s:%d\n",
                        (char*) metaDataBucketLinkedListElement + sizeof(new_ptr_list_t), 
                        (int) metaDataBucketLinkedListElement->size,
                        metaDataBucketLinkedListElement->file, 
                        metaDataBucketLinkedListElement->line);
                }

                memCount++;
                memSize += (uint64_t)metaDataBucketLinkedListElement->size;

                // go to the next item on the list
                metaDataBucketLinkedListElement = metaDataBucketLinkedListElement->next;
            }
        }

        closeFile(dumper);
    }

    void freeAllMemory() {
        for (int index = 0; index < MEMPLUMBER_HASHTABLE_SIZE; ++index) {
            new_ptr_list_t* metaDataBucketLinkedListElement = m_PointerListHashtable[index];

            // if bucket is empty - continue
            if (metaDataBucketLinkedListElement == NULL) {
                continue;
            }

            // go over all of the elements in the link list in this bucket
            while (metaDataBucketLinkedListElement != NULL) {
                new_ptr_list_t* next = metaDataBucketLinkedListElement->next;

                void* actualPointerInRecord = (char*)metaDataBucketLinkedListElement + sizeof(new_ptr_list_t);

                if (isVerbose()) {
                    fprintf(m_Dumper, "FreeAllMem: freeing 0x%p (size %d[bytes]) allocated in %s:%d\n", 
                        actualPointerInRecord,
                        (int)metaDataBucketLinkedListElement->size,
                        metaDataBucketLinkedListElement->file,
                        metaDataBucketLinkedListElement->line);
                }

                // free the current element
                free(metaDataBucketLinkedListElement);

                // go to the next item on the list
                metaDataBucketLinkedListElement = next;
            }
            
            // done freeing all elements in the linked list, set the hashtable bucket to null
            m_PointerListHashtable[index] = NULL;
        }
        
        closeFile(m_Dumper);
        m_Dumper = NULL;
    }
};


#if defined _MSC_VER || defined _WIN32
// TODO: backtrace() is not supported on Windows.
// We can use dbghelp but it's not supported on MinGW. Need to figure out a way to solve it on all platforms
const char* getCaller() {
    return "Unknown";
}
#else
#include <execinfo.h>
const char* getCaller() {
    void* backtraceArr[3];
    size_t backtraceArrSize;

    // get void*'s for all entries on the stack
    backtraceArrSize = backtrace(backtraceArr, 3);

    if (backtraceArrSize < 3) {
        return "Unknown";
    }

    // get the symbols
    char** backtraceSymbols = backtrace_symbols(backtraceArr, backtraceArrSize);

    // the caller is second in the backtrace
    return backtraceSymbols[2];
}
#endif

void* operator new(std::size_t size, const char* file, int line) {
    return MemPlumberInternal::getInstance().allocateMemory(size, file, line);
}

void* operator new[](std::size_t size, const char* file, int line) {
    return operator new(size, file, line);
}

void* operator new[](size_t size) _THROW_BAD_ALLOC {
	return operator new(size, getCaller(), 0);
}

void* operator new(size_t size) _THROW_BAD_ALLOC {
	return operator new(size, getCaller(), 0);
}

void* operator new(size_t size, const std::nothrow_t&) _NOEXCEPT {
	return operator new(size, getCaller(), 0);
}

void* operator new[](size_t size, const std::nothrow_t&) _NOEXCEPT {
	return operator new[](size, getCaller(), 0);
}

void operator delete(void* pointer, const char* file, int line) {
    MemPlumberInternal::getInstance().freeMemory(pointer, file, line);
}

void operator delete(void* pointer) throw() {
    operator delete(pointer, __FILE__, __LINE__);
}

void operator delete(void* pointer, std::size_t size) {
    operator delete(pointer, __FILE__, __LINE__);
}

void operator delete[](void* pointer) _NOEXCEPT {
    operator delete(pointer, __FILE__, __LINE__);
}

void operator delete[](void* pointer, std::size_t size) {
    operator delete(pointer, __FILE__, __LINE__);
}


void operator delete[](void* pointer, const char* file, int line) {
    operator delete(pointer, file, line);
}

void operator delete(void* pointer, const std::nothrow_t&) throw() {
	operator delete(pointer);
}

void operator delete[](void* pointer, const std::nothrow_t&) throw() {
	operator delete(pointer, std::nothrow);
}

void __mem_leak_check(size_t& memLeakCount, uint64_t& memLeakSize, bool verbose, const char* fileDumperName, bool append) {
    MemPlumberInternal::getInstance().checkLeaks(memLeakCount, memLeakSize, verbose, fileDumperName, append);
}

void __static_mem_check(size_t&  memCount, uint64_t& memSize, bool verbose, const char* fileDumperName, bool append) {
    MemPlumberInternal::getInstance().staticMemAllocation(memCount, memSize, verbose, fileDumperName, append);
}

void __start(bool verbose, const char* fileDumperName, bool append) {
    MemPlumberInternal::getInstance().start(verbose, fileDumperName, append);
}

void __stop() {
    MemPlumberInternal::getInstance().stop();
}

void __stop_and_free_all_mem() {
    MemPlumberInternal::getInstance().stop(false);
    MemPlumberInternal::getInstance().freeAllMemory();
}

void __program_started() {
    MemPlumberInternal::getInstance().programStarted();
}
