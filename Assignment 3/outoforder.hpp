#include <vector>
#include <array>
#include <queue>
#include <iostream>
#include <iomanip>
#include <unordered_set>
#include <algorithm>
#include <stdlib.h>

#define ARF_SIZE 8
#define RRF_SIZE 8
#define MEM_SIZE 100
#define MAX_ARITY 2
#define ALU_COUNT 2

class buffer_overflow_error : std::runtime_error {
    public:
        buffer_overflow_error(const std::string& what_arg) : runtime_error(what_arg) {}
};

//class buffer_underflow_error : std::runtime_error {
    //public:
        //buffer_underflow_error(const std::string& what_arg) : runtime_error(what_arg) {}
//};

struct bus {
    bool busy;
    int tag;
    int data;
} cdb, mem_bus;

struct arf_entry {
    bool busy;
    int tag;
    int data;
};

class ArchitectedRegisterFile {
    std::array<arf_entry, ARF_SIZE> _arf;

    public:
    arf_entry& operator[] (size_t i) {
        return _arf[i];
    }
    const arf_entry& operator[] (size_t i) const {
        return _arf[i];
    }
    int size() {
        return _arf.size();
    }
    void initialise() {
        for (size_t i = 0;i<_arf.size();i++)
        {
            _arf[i].data = 1; 
        }
    }
    friend std::ostream& operator<< (std::ostream&, ArchitectedRegisterFile&);
} arf;

std::ostream& operator<< (std::ostream& out, ArchitectedRegisterFile& arf) {
    out << std::setfill('=') << std::setw(25) << "" << std::endl;
    out << std::setfill(' ') << "ARF" << std::endl;
    out << std::setfill('-') << std::setw(25) << "" << std::endl;
    out << std::setfill(' ')
        << std::setw(5) << "Reg"
        << std::setw(5) << "Busy"
        << std::setw(5) << "Tag"
        << std::setw(10) << "Data" << std::endl;
    for (int i=0; i < arf.size(); ++i) {
        out << std::setw(5) << "R"+std::to_string(i+1)
            << std::setw(5) << arf[i].busy;
        if (arf[i].busy)
            out << std::setw(5) << "S"+std::to_string(arf[i].tag);
        else
            out << std::setw(5) << arf[i].tag;
        out << std::setw(10) << arf[i].data << std::endl;
    }
    out << std::endl;
    return out;
}

struct rrf_entry {
    bool busy;
    bool valid;
    int data;
};

class RenameRegisterFile {
    std::array<rrf_entry, RRF_SIZE> _rrf;
    int _tail = 0;
    int find_free_entry() {
        auto e = std::find_if_not(
                _rrf.cbegin(), _rrf.cend(),
                [] (rrf_entry re) {
                return re.busy;
                } );
        return e - _rrf.cbegin();
    }

    public:
    rrf_entry& operator[] (size_t i) {
        return _rrf[i];
    }
    const rrf_entry& operator[] (size_t i) const {
        return _rrf[i];
    }
    int size() {
        return _rrf.size();
    }
    bool full() {
        _tail = find_free_entry();
        return _tail == size();
    }
    int nextIndex() {
        return _tail;
    }
    void push(const rrf_entry &re) {
        _rrf[_tail] = re;
        _tail = find_free_entry();
    }
    void pop(size_t i) {
        _rrf[i].busy = false;
        _rrf[i].valid = false;
        _rrf[i].data = 0;
    }
    friend std::ostream& operator<< (std::ostream&, RenameRegisterFile&);
} rrf;

std::array<unsigned, MEM_SIZE> memory;

std::ostream& operator<< (std::ostream& out, RenameRegisterFile& rrf) {
    out << std::setfill('=') << std::setw(25) << "" << std::endl;
    out << std::setfill(' ') << "RRF" << std::endl;
    out << std::setfill('-') << std::setw(25) << "" << std::endl;
    out << std::setfill(' ')
        << std::setw(5) << "Reg"
        << std::setw(5) << "Busy"
        << std::setw(5) << "Val"
        << std::setw(10) << "Data" << std::endl;
    for (int i=0; i < rrf.size(); ++i) {
        out << std::setw(5) << "S"+std::to_string(i+1)
            << std::setw(5) << rrf[i].busy
            << std::setw(5) << rrf[i].valid
            << std::setw(10) << rrf[i].data << std::endl;
    }
    out << std::endl;
    return out;
}

enum operation_type {
    INVALID = -1,
    ADD, SUB, MUL, DIV, AND, OR, XOR,
    LD, ST
};

std::ostream& operator<< (std::ostream& out, operation_type op) {
    switch (op) {
        case ADD: out << "ADD"; break;
        case SUB: out << "SUB"; break;
        case MUL: out << "MUL"; break;
        case DIV: out << "DIV"; break;
        case AND: out << "AND"; break;
        case OR:  out << "OR"; break;
        case XOR: out << "XOR"; break;
        case LD:  out << "LD"; break;
        case ST:  out << "ST"; break;
        default:  out << "INVALID";
    }
    return out;
}

int arity[] = {
    2, 2, 2, 2, 2, 2, 2,
    1, 2
};

struct rs_entry {
    struct op_info {
        bool ready = false;
        int field = 0;
    };
    bool busy;
    unsigned addr;
    std::array<op_info, MAX_ARITY> src;
    int dest;
    operation_type op_type = INVALID;
    //int exec_unit;
    //bool issued;
    bool all_ops_ready() {
        if (op_type == INVALID)
            return false;
        bool ret = true;
        for (int i=0; i < arity[op_type]; ++i)
            ret = ret && src[i].ready;
        return ret;
    }
};

class ReservationStation {
    static unsigned _last_q_pos;

    std::vector<rs_entry> _rs;
    std::vector<unsigned> _q_pos;
    int _tail = 0;

    size_t find_free_entry() {
        auto e = std::find_if_not(
                _rs.cbegin(), _rs.cend(),
                [] (rs_entry re) { return re.busy; } );
        return e - _rs.cbegin();
    }

    public:
    rs_entry& operator[] (size_t i) {
        return _rs[i];
    }
    const rs_entry& operator[] (size_t i) const {
        return _rs[i];
    }
    int size() {
        return _rs.size();
    }
    void setSize(size_t sz) {
        _rs.resize(sz);
        _q_pos.resize(sz);
        for (size_t i = 0; i < sz; ++i)
            _q_pos[i] = 0;
    }
    bool full() {
        _tail = find_free_entry();
        return _tail == size();
    }
    void push(const rs_entry &re) {
        _rs[_tail] = re;
        _rs[_tail].busy = true;
        _q_pos[_tail] = _last_q_pos++;
        _tail = find_free_entry();
    }
    void pop(size_t index) {
        _rs[index].busy = false;
        for (int i=0; i < MAX_ARITY; ++i) {
            _rs[index].src[i].ready = false;
            _rs[index].src[i].field = 0;
        }
        _rs[index].dest = 0;
        _rs[index].addr = 0;
        _rs[index].op_type = INVALID;
        for (size_t i = 0; i < _q_pos.size(); ++i) {
            if (_q_pos[i] > _q_pos[index])
                _q_pos[i]--;
        }
        _q_pos[index] = 0;
        _last_q_pos--;
    }
    std::vector<size_t> indices_in_q_order() {
        std::vector<size_t> indices;
        for (size_t i = 0; i < _rs.size(); ++i) {
            if (_rs[i].busy)
                indices.push_back(i);
        }
        std::sort(indices.begin(), indices.end(),
                [&] (size_t i, size_t j) {
                    return _q_pos[i] != 0 && _q_pos[i] < _q_pos[j];
                } );
        return indices;
    }
    friend std::ostream& operator<< (std::ostream& out, ReservationStation& rrf);
} rs;
unsigned ReservationStation::_last_q_pos = 1;

std::ostream& operator<< (std::ostream& out, ReservationStation& rs) {
    out << std::setfill('=') << std::setw(75) << "" << std::endl;
    out << std::setfill(' ') << "Reservation Station" << std::endl;
    out << std::setfill('-') << std::setw(75) << "" << std::endl;
    out << std::setfill(' ')
        << std::setw(5) << "Idx"
        << std::setw(5) << "Busy"
        << std::setw(5) << "Instn";
    for (int j=0; j < MAX_ARITY; ++j) {
        out << std::setw(20) << "Operand "+std::to_string(j+1);
    }
    out << std::setw(10) << "Dest"
        << std::setw(10) << "Operation" << std::endl;

    out << std::setw(5) << ""
        << std::setw(5) << ""
        << std::setw(5) << "";
    for (int j=0; j < MAX_ARITY; ++j) {
        out << std::setw(10) << "Ready"
            << std::setw(10) << "Field";
    }
    out << std::setw(10) << ""
        << std::setw(10) << "" << std::endl;

    for (int i=0; i < rs.size(); ++i) {
        out << std::setw(5) << i
            << std::setw(5) << rs[i].busy
            << std::setw(5) << rs[i].addr;
        for (int j=0; j < MAX_ARITY; ++j) {
            out << std::setw(10) << rs[i].src[j].ready;
            if (!rs[i].busy || rs[i].src[j].ready)
                out << std::setw(10) << rs[i].src[j].field;
            else
                out << std::setw(10) << "S"+std::to_string(rs[i].src[j].field);
        }
        if (!rs[i].busy)
            out << std::setw(10) << rs[i].dest;
        else
            out << std::setw(10) << "S"+std::to_string(rs[i].dest);
        out << std::setw(10) << rs[i].op_type << std::endl;
    }
    out << std::endl;
    return out;
}

struct rob_entry {
    bool busy;
    //bool issued;
    //bool finished;
    unsigned addr;
    int arf_index;
    int rrf_index;
    //bool speculative;
    //bool valid;
};

class ReOrderBuffer {
    std::vector<rob_entry> _rob;
    int _head = 0;
    int _tail = 0;
    unsigned _entry_count = 0;

    public:
    rob_entry& operator[] (size_t i) {
        return _rob[i];
    }
    const rob_entry& operator[] (size_t i) const {
        return _rob[i];
    }
    int head() {
        return _head;
    }
    int tail() {
        return _tail;
    }
    int size() {
        return _rob.size();
    }
    void setSize(size_t sz) {
        _rob.resize(sz);
    }
    int entryCount() {
        return _entry_count;
    }
    bool empty() {
        return _entry_count == 0;
    }
    bool full() {
        return _entry_count == _rob.size();
    }
    void push(const rob_entry &re) {
        _rob[_tail] = re;
        _rob[_tail].busy = true;
        _tail = (_tail+1) % size();
        _entry_count++;
    }
    void pop() {
        //if (empty()) throw buffer_underflow_error("ROB empty!");
        _rob[_head].busy = false;
        _rob[_head].addr = 0;
        _rob[_head].rrf_index = 0;
        _rob[_head].arf_index = 0;
        _head = (_head+1) % size();
        _entry_count--;
    }
    rob_entry& front() {
        return _rob[_head];
    }
    friend std::ostream& operator<< (std::ostream&, ReOrderBuffer&);
} rob;

std::ostream& operator<< (std::ostream& out, ReOrderBuffer& rob) {
    out << std::setfill('=') << std::setw(40) << "" << std::endl;
    out << std::setfill(' ') << "Re-order Buffer" << std::endl;
    out << std::setfill('-') << std::setw(40) << "" << std::endl;
    out << std::setfill(' ')
        << std::setw(5) << "Idx"
        << std::setw(5) << "Busy"
        << std::setw(10) << "Instn"
        << std::setw(10) << "ARF reg"
        << std::setw(10) << "RRF reg" << std::endl;

    for (int i=0; i < rob.size(); ++i) {
        out << std::setw(5) << i
            << std::setw(5) << rob[i].busy
            << std::setw(10) << rob[i].addr;
        if (!rob[i].busy)
            out << std::setw(10) << rob[i].arf_index
                << std::setw(10) << rob[i].rrf_index << std::endl;
        else
            out << std::setw(10) << 'R'+std::to_string(rob[i].arf_index)
                << std::setw(10) << 'S'+std::to_string(rob[i].rrf_index) << std::endl;
    }
    out << std::endl;
    return out;
}

struct sb_entry {
    bool busy;
    int data;
    unsigned mem_addr;
    std::queue<int> rrf_indices_to_update;
};

class StoreBuffer {
    std::vector<sb_entry> _sb;
    int _head = 0;
    int _tail = 0;
    unsigned _entry_count = 0;

    public:
    sb_entry& operator[] (size_t i) {
        return _sb[i];
    }
    const sb_entry& operator[] (size_t i) const {
        return _sb[i];
    }
    int head() {
        return _head;
    }
    int tail() {
        return _tail;
    }
    int size() {
        return _sb.size();
    }
    void setSize(size_t sz) {
        _sb.resize(sz);
    }
    int entryCount() {
        return _entry_count;
    }
    bool empty() {
        return _entry_count == 0;
    }
    bool full() {
        return _entry_count == _sb.size();
    }
    void push(const sb_entry &sbe) {
        _sb[_tail] = sbe;
        _sb[_tail].busy = true;
        _tail = (_tail+1) % size();
        _entry_count++;
    }
    void pop() {
        //if (empty()) throw buffer_underflow_error("ROB empty!");
        _sb[_head].busy = false;
        _sb[_head].mem_addr = 0;
        _sb[_head].data = 0;
        _head = (_head+1) % size();
        _entry_count--;
    }
    sb_entry& front() {
        return _sb[_head];
    }
} sb;

class FunctionalUnit {
    protected:
    int _timer = 0; // works like a countdown timer depending on operation latency
    int _rs_index = -1; // reservation station entry dispatched to this execution unit
    public:
    bool busy = false;
    virtual void executeInstn(int) = 0;
    virtual void updateTimer() = 0;
};

class LoadUnit : public FunctionalUnit {
    const static int _latency = 1;
    //int _dest_rrf_index = -1;
    //int _result;

    public:
    static int latency() {
        return _latency;
    }
    void executeInstn(int rs_index) {
        _rs_index = rs_index;
        //_dest_rrf_index = rs[rs_index].dest;
        //unsigned addr = rs[rs_index].src[0].field;
        //_result = memory[addr];
        _timer = latency();
        busy = true;
        //rs.pop(rs_index);
    }
    void updateTimer() {
        if (_timer > 0)
            _timer--;
        if (_timer == 0 && busy && !cdb.busy) {
            //cdb.busy = true;
            //cdb.tag = _dest_rrf_index;
            //cdb.data = _result;
            busy = false;
        }
    }
} ldu;

class StoreUnit : public FunctionalUnit {
    const static int _latency = 1;

    public:
    static int latency() {
        return _latency;
    }
    void executeInstn(int rs_index) {
        _rs_index = rs_index;
        _timer = latency();
        busy = true;
        //rs.pop(rs_index);
    }
    void updateTimer() {
        if (_timer > 0)
            _timer--;
        if (_timer == 0 && busy) {
            busy = false;
        }
    }
} stu;

class IntegerALU : public FunctionalUnit {
    static std::array<int, XOR-ADD+1> _latency; //Latency of each operation
    int _dest_rrf_index = -1;
    int _result;

    public:
    static int latency(operation_type op) {
        return _latency[op-ADD];
    }
    static void setLatency(operation_type op, int lat) {
        _latency[op-ADD] = lat;
    }
    int computeResult(int op1, int op2, operation_type op) {
        switch(op) {
            case ADD: return op1 + op2;
            case SUB: return op1 - op2;
            case MUL: return op1 * op2;
            case DIV: return op1 / op2;
            case AND: return op1 & op2;
            case OR:  return op1 | op2;
            case XOR: return op1 ^ op2;
            default:  throw std::invalid_argument("Invalid ALU operation");
        }
    }
    void executeInstn(int rs_index) {
        int op1, op2;
        operation_type op;
        _rs_index = rs_index;
        op1 = rs[rs_index].src[0].field;
        op2 = rs[rs_index].src[1].field;
        op = rs[rs_index].op_type;
        _result = computeResult(op1, op2, op);
        _dest_rrf_index = rs[rs_index].dest;
        _timer = latency(op);
        busy = true;
        rs.pop(rs_index);
    }
    void updateTimer() {
        if (_timer > 0)
            _timer--;
        if (_timer == 0 && busy && !cdb.busy) {
            cdb.busy = true;
            cdb.tag = _dest_rrf_index;
            cdb.data = _result;
            busy = false;
        }
    }
};
std::array<int, XOR-ADD+1> IntegerALU::_latency; //Latency of each operation
std::array<IntegerALU, ALU_COUNT> alus;
