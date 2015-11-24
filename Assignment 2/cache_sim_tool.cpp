/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2014 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "pin.H"

#define K 1024

/*******************************************************************************************
 * CACHE MODEL SECTION
 *
 * This section contains the declarations and defintions for the cache model
*******************************************************************************************/

/* Abstract class ReplacementPolicy */
class ReplacementPolicy {
    public:
    virtual void updateCounters(int, int) {}
    virtual int lineToReplace(int set_no) = 0;
};

/***********************************************************************************************
 * LRUPolicy 
 * **********************************************************************************************/

/* Declarations */

class LRUPolicy : public ReplacementPolicy {
    int _set_count;
    int _set_line_count;
    int **_line_ctrs;

    public:
    LRUPolicy(int set_count, int set_line_count);
    ~LRUPolicy();
    void updateCounters(int set_no, int line_no);
    int lineToReplace(int set_no);
};

/* Definitions */

LRUPolicy::LRUPolicy(int set_count, int set_line_count) : _set_count(set_count), _set_line_count(set_line_count) {
    _line_ctrs = new int*[_set_count];
    for (int set_no = 0; set_no < _set_count; ++set_no) {
        _line_ctrs[set_no] = new int[_set_line_count];
        for (int line_no = 0; line_no < _set_line_count; ++line_no) {
            _line_ctrs[set_no][line_no] = line_no;
        }
    }
}

LRUPolicy::~LRUPolicy() {
    for (int set_no = 0; set_no < _set_count; ++set_no)
        delete[] _line_ctrs[set_no]; 
    delete[] _line_ctrs;
}

void LRUPolicy::updateCounters(int set_no, int line_no) {
    for (int j = 0; j < _set_line_count; ++j) {
        if ( _line_ctrs[set_no][j] < _line_ctrs[set_no][line_no] )
            _line_ctrs[set_no][j]++;
    }
    _line_ctrs[set_no][line_no] = 0;
}

int LRUPolicy::lineToReplace(int set_no) {
    for (int i = 0; i < _set_line_count; ++i) {
        if ( _line_ctrs[set_no][i] == _set_line_count-1 )
            return i;
    }
    return -1;
}


/***********************************************************************************************
 * LFUPolicy 
 * **********************************************************************************************/

/* Declarations */

class LFUPolicy : public ReplacementPolicy {
    int _set_count;
    int _set_line_count;
    int **_line_ctrs;

    public:
    LFUPolicy(int set_count, int set_line_count);
    ~LFUPolicy();
    void updateCounters(int set_no, int line_no);
    int lineToReplace(int set_no);
};

/* Definitions */

LFUPolicy::LFUPolicy(int set_count, int set_line_count) : _set_count(set_count), _set_line_count(set_line_count) {
    _line_ctrs = new int*[_set_count];
    for (int set_no = 0; set_no < _set_count; ++set_no) {
        _line_ctrs[set_no] = new int[_set_line_count];
        for (int line_no = 0; line_no < _set_line_count; ++line_no) {
            _line_ctrs[set_no][line_no] = 0;
        }
    }
}

LFUPolicy::~LFUPolicy() {
    for (int set_no = 0; set_no < _set_count; ++set_no)
        delete[] _line_ctrs[set_no]; 
    delete[] _line_ctrs;
}

void LFUPolicy::updateCounters(int set_no, int line_no) {
    _line_ctrs[set_no][line_no]++;
}

int LFUPolicy::lineToReplace(int set_no) {
    int min_ctr = _line_ctrs[set_no][0];
    int line_to_replace = 0;
    for (int i = 1; i < _set_line_count; ++i) {
        if ( _line_ctrs[set_no][i] < min_ctr ) {
            min_ctr = _line_ctrs[set_no][i];
            line_to_replace = i;
        }
    }
    return line_to_replace;
}

/***********************************************************************************************
 * RRPolicy 
 * **********************************************************************************************/

class RRPolicy : public ReplacementPolicy {
    int _set_line_count;

    public:
    RRPolicy(int set_line_count) : _set_line_count(set_line_count) { srand(time(NULL)); }
    int lineToReplace(int) { return rand() % _set_line_count; }
};


/***********************************************************************************************
 * Global function definitions
 * *********************************************************************************************/

ReplacementPolicy *stringToRepPolicy(const char *rep_policy, int set_count,
       int set_line_count) {
    if (strcmp(rep_policy, "LRU") == 0) return new LRUPolicy(set_count, set_line_count);
    else if (strcmp(rep_policy, "LFU") == 0) return new LFUPolicy(set_count, set_line_count);
    else if (strcmp(rep_policy, "RR") == 0) return new RRPolicy(set_line_count);
    else return NULL;
}

int log2(int n) {
    int log2n = -1;
    for (; n > 0; n >>= 1, ++log2n);
    return log2n;
}

int bitMask(int no_of_bits) {
    return (1 << no_of_bits) - 1;
}

/***********************************************************************************************
 * CacheLine - Data structure for a single cache line
 * *********************************************************************************************/

struct CacheLine {
    unsigned long _tag;
    //int contents;
    bool _valid;
    bool _dirty;

    public:
    CacheLine() : _tag(0), _valid(false), _dirty(false) {}
};

/***********************************************************************************************
 * Cache - Class which defines the data structures and methods for a single cache level
 * *********************************************************************************************/

/* Declarations */

class Cache {
    //Input parameters
    int _level_no;
    int _size;
    int _line_size;
    int _assoc;
    int _hit_latency;
    ReplacementPolicy *_rep_policy;

    //Computed paramters
    int _line_count;
    int _set_count;
    int _word_bits;
    int _set_bits;
    long _hit_count;
    long _miss_count;

    CacheLine **_lines;

    public:
    
    //Allocate/Deallocate resources and initialize parameters
    void initialize(int level_no, int size, int line_size, int assoc,
           int hit_latency, const char *rep_policy);
    void finalize();

    //Accessors
    int level() { return _level_no; }
    int lineCount() { return _line_count; }
    int setCount() { return _set_count; }
    int associativity() { return _assoc; }
    bool isValidLine(int set_no, int line_no);
    bool isDirtyLine(int set_no, int line_no);
    unsigned long EAToTag(VOID *addr) {
        return (unsigned long)addr & ~bitMask(_word_bits+_set_bits);
    }
    unsigned long EAToSetNo(VOID *addr) {
        return ((unsigned long)addr >> _word_bits) & bitMask(_set_bits);
    }
    unsigned long EAToWordInSet(VOID *addr) {
        return (unsigned long)addr & bitMask(_word_bits);
    }
    unsigned long TagSetToEA(unsigned long tag, unsigned long set) {
        return tag | (set << _word_bits);
    }

    //Return statistics
    long memoryAccesses() { return _hit_count + 2*_miss_count; }
    long hitCount() { return _hit_count; }
    double missRate() { return (double)(_miss_count) / (_hit_count+_miss_count); }

    int lineToReplace(int set_no);
    bool findAddress(VOID *addr, int &set_no, int &line_no);

    //Simulate a cache hierarchy
    friend void readAddress(int level_no, VOID *addr);
    friend void writeAddress(int level_no, VOID *addr);
    friend void evictLinesFromCache(int level_index, int set_no, int line_no);
};

/* Definitions */

// Memory allocation and parameter initialization
void Cache::initialize(int level_no, int size, int line_size,
       int assoc, int hit_latency, const char *rep_policy) {
    _level_no = level_no;
    _size = size;
    _line_size = line_size;
    _assoc = assoc;
    _hit_latency = hit_latency;
    _line_count = _size*K / _line_size;
    _set_count = _line_count / _assoc;
    _word_bits = log2(_line_size);
    _set_bits = log2(_set_count);
    _hit_count = _miss_count = 0;

    /*================================================================
     * NOTE:- Replacement policy must be initialized after the other
     * parameters since the internal data structures of a replacement
     *  policy may require some of these values
     ================================================================= */
    _rep_policy = stringToRepPolicy(rep_policy, _set_count, _assoc);

    _lines = new CacheLine*[_set_count];
    for (int i = 0; i < _set_count; ++i) {
        _lines[i] = new CacheLine[_line_count]; 
    }
}

// Deallocation of resources
void Cache::finalize() {
    for (int i = 0; i < _set_count; ++i)
        delete[] _lines[i];
    delete[] _lines;
}

// Chooses an invalid line to be replaced if the set is not full;
// a line as per the replacement policy otherwise
int Cache::lineToReplace(int set_no) {
    for (int i = 0; i < _assoc; ++i) {
        if (!_lines[set_no][i]._valid)
            return i;
    }
    return _rep_policy->lineToReplace(set_no);
}

// Returns true if addr is there in this cache level. In this case, (set_no, line_no) gives
// the cache line containing addr.
// Returns false if addr is not there. Here, (set_no, line_no) gives the cache line where
// addr should be put as per the replacement policy.
bool Cache::findAddress(VOID *addr, int &set_no, int &line_no) {
    set_no = EAToSetNo(addr);
    for (int i = 0; i < _assoc; ++i) {
        if (_lines[set_no][i]._tag == EAToTag(addr) && _lines[set_no][i]._valid) {
            line_no = i;
            return true;
        }
    }
    line_no = lineToReplace(set_no);
    return false;
}

/*******************************************************************************************
 * Declaration of functions that perform the simulations across the entire cache hierarchy
 * over different levels 
 * ****************************************************************************************/
void readAddress(int level_index, VOID *addr);
void writeAddress(int level_index, VOID *addr);
void evictLinesFromCache(int start_level, int set_no, int line_no);


/*******************************************************************************************
 * INSTRUMENTATION AND ANALYSIS SECTION
 *
 * This section contains the pintool that drives the cache simulation
*******************************************************************************************/

static Cache *cache;
static int level_count;
static int memory_latency;

static KNOB<string> KnobConfFile(KNOB_MODE_WRITEONCE,  "pintool",
        "f", "", "specify file name containing configuration of cache model");

// Perfroms removal of all cache lines triggered due to the eviction of the line
// at (set_no, line_no) at the cache level start_level
void evictLinesFromCache(int start_level, int set_no, int line_no) {

    int max_line_size = cache[start_level]._line_size;

    // evict line from start_level
    CacheLine &l = cache[start_level]._lines[set_no][line_no];
    if (!l._valid)
        return;

    l._valid = false;
    VOID *addr = (VOID *)cache[start_level].TagSetToEA(l._tag, set_no);
    
    // iteratively move down the hierarchy removing required lines
    for (int level_index = start_level-1; level_index >= 0; --level_index) {
        
        Cache &clevel = cache[level_index]; // clevel is current cache level
        unsigned long tag = clevel.EAToTag(addr);

        // If the cache line size of the current level is smaller than the max seen
        // previously, multiple cache lines will need to be evicted.
        // This corresponds to the case where a single cache line at a higher level
        // corresponds to 2 or more lines at a lower level.
        if (clevel._line_size < max_line_size) {

            unsigned long addr_prefix = (unsigned long)addr & ~bitMask(max_line_size);

            // Find all lines in clevel that could have a line with the first part
            // of the address of its entries being addr_prefix

            // Iterate over all sets
            for (int i = 0; i < clevel._set_count; ++i) {

                unsigned long line_addr = clevel.TagSetToEA(tag, i);

                // Iterate over all lines in each set
                for (int j = 0; j < clevel._assoc; ++j) {

                    CacheLine &line = clevel._lines[i][j];

                    // if the line contains valid data and the prefix matches,
                    // invalidate it taking care of dirty eviction
                    if (line._valid && ((line_addr & addr_prefix) == addr_prefix)) {

                        line._valid = false;

                        // in case there is dirty eviction and there is a cache level
                        // above the original start_level, write the evicted lines
                        // to this higher hlevel.
                        if (line._dirty && start_level+1 < level_count) {
                            Cache &hlevel = cache[start_level+1];

                            // if hlevel has smaller line size than clevel,
                            // multiple lines will need to be written in hlevel.
                            if (hlevel._line_size < clevel._line_size) {
                                for (unsigned long addr = line_addr;
                                        (addr & line_addr) == line_addr;
                                        addr += hlevel._line_size) {
                                    writeAddress(start_level+1, (VOID *)addr);
                                }
                                
                            // otherwise, only one write to hlevel is sufficient
                            } else {
                                writeAddress(start_level+1, (VOID *)line_addr);
                            }
                        }
                    }
                }
            }
        
        // If clevel cache line is long enough (>= longest line size so far),
        // only one eviction needs to be done
        } else {

            int set_no = clevel.EAToSetNo(addr);

            // Search for line to be evicted among all the lines in the set
            for (int j = 0; j < clevel._assoc; ++j) {

                CacheLine &line = clevel._lines[set_no][j];

                // If there is a line with the given tag containing valid data,
                // invalidate the data and evict it taking care if the line is dirty
                if (line._valid && line._tag == tag) {

                    line._valid = false;

                    // if the line is dirty and there is a higher cache level,
                    // write the line to it
                    if (line._dirty && start_level+1 < level_count) {
                        Cache &hlevel = cache[start_level+1];
                        unsigned long line_addr = clevel.TagSetToEA(tag, set_no);
                        if (hlevel._line_size < clevel._line_size) {
                            for (unsigned long addr = line_addr;
                                    (addr & line_addr) == line_addr;
                                    addr += hlevel._line_size) {
                                writeAddress(start_level+1, (VOID *)addr);
                            }
                        } else {
                            writeAddress(start_level+1, (VOID *)line_addr);
                        }
                    }
                }
            }

            // update the max_line_size seen so far
            if (clevel._line_size > max_line_size)
                max_line_size = clevel._line_size;
        }
    }
}

// Read an address from the cache hierarchy starting from a given level
void readAddress(int level_index, VOID *addr) {
    if (level_index >= level_count)
        return;
    Cache &clevel = cache[level_index];
    int set_no = -1, line_no = -1;
    bool hit = clevel.findAddress(addr, set_no, line_no);
    if (hit) {
        clevel._hit_count++;
    } else {
        readAddress(level_index+1, addr);
        clevel._miss_count++;
        line_no = clevel.lineToReplace(set_no);
        // remove this line from this and lower levels
        evictLinesFromCache(level_index, set_no, line_no);
        // fill the new line into the cache
        CacheLine &line = clevel._lines[set_no][line_no];
        line._tag = clevel.EAToTag(addr);
        line._valid = true;
        line._dirty = false;
    }
    clevel._rep_policy->updateCounters(set_no, line_no);
}

// Write an address to the cache hierarchy starting from a given level
void writeAddress(int level_index, VOID *addr) {
    if (level_index >= level_count)
        return;
    Cache &clevel = cache[level_index];
    int set_no = -1, line_no = -1;
    bool hit = clevel.findAddress(addr, set_no, line_no);
    CacheLine &line = clevel._lines[set_no][line_no];
    if (hit) {
        clevel._hit_count++;
    } else {
        readAddress(level_index+1, addr);
        clevel._miss_count++;
        line_no = clevel.lineToReplace(set_no);
        // remove this line from this and lower levels
        evictLinesFromCache(level_index, set_no, line_no);
        // fill the new line into the cache
        line._tag = clevel.EAToTag(addr);
        line._valid = true;
    }
    // Mark line as modified
    line._dirty = true;
    clevel._rep_policy->updateCounters(set_no, line_no);
}

// Simulate a memory read access
VOID RecordMemRead(VOID * addr)
{
    readAddress(0, addr);
}

// Simulate a memory write access
VOID RecordMemWrite(VOID * addr)
{
    writeAddress(0, addr);
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_INST_PTR,
                IARG_END);

    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    for (int i = 0; i < level_count; ++i)
    {
        printf("Level %d:-\n", cache[i].level());
        printf("Miss ratio = %lf\n", cache[i].missRate());
        printf("Cache hits = %ld\n", cache[i].hitCount());
        printf("Total memory accesses = %ld\n", cache[i].memoryAccesses());
        printf("\n");
    }
    for (int i = 0; i < level_count; ++i)
    {
        cache[i].finalize();
    }
    delete[] cache;
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    PIN_ERROR( "This Pintool drives a cache simulator\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Read configuration file                                               */
/* ===================================================================== */
void ReadConfFile(std::string conf_filename)
{
    FILE *conf_file = fopen(conf_filename.c_str(), "r");
    int nargs = fscanf(conf_file, "Levels = %d\n", &level_count);
    cache = new Cache[level_count];
    for (int i = 0; i < level_count; ++i)
    {
        int level_no, size, line_size, assoc, hit_latency;
        char rep_policy[5];
        nargs = fscanf(conf_file, "\n[Level %d]\n", &level_no);
        nargs = fscanf(conf_file, "Size = %dKB\n", &size);
        nargs = fscanf(conf_file, "Associativity = %d\n", &assoc);
        nargs = fscanf(conf_file, "Block_size = %dbytes\n", &line_size);
        nargs = fscanf(conf_file, "Hit_Latency = %d\n", &hit_latency);
        nargs = fscanf(conf_file, "Replacement_Policy = %s\n", rep_policy);
        cache[i].initialize(level_no, size, line_size, assoc, hit_latency, rep_policy);
    }
    nargs = fscanf(conf_file, "\n[Main Memory]\n");
    nargs = fscanf(conf_file, "Hit Latency = %d", &memory_latency);
    nargs++;
    fclose(conf_file);
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    ReadConfFile(KnobConfFile.Value());

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
