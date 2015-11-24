#include <queue>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include "outoforder.hpp"

#define IPC 2

struct instruction {
    struct operand {
        bool is_imm;
        int field;
    };
    unsigned addr;
    operation_type op;
    std::array<operand, MAX_ARITY> src;
    int dest_arf_index;
};
std::queue<instruction> instn_buffer;

std::string toupper(std::string& str) {
    for (size_t i=0; i < str.size(); ++i) {
        str[i] = std::toupper(str[i]);
    }
    return str;
}

operation_type decode_instn(std::string instn) {
    instn = toupper(instn);
    if (instn == "ADD")      return ADD;
    else if (instn == "SUB") return SUB;
    else if (instn == "MUL") return MUL;
    else if (instn == "DIV") return DIV;
    else if (instn == "AND") return AND;
    else if (instn == "OR")  return OR;
    else if (instn == "XOR") return XOR;
    else if (instn == "LD")  return LD;
    else if (instn == "ST")  return ST;
    else                     return INVALID; 
}

bool isALUOperation(operation_type op) {
    return op >= ADD && op <= XOR;
}

bool isLoad(operation_type op) {
    return op == LD;
}

bool isStore(operation_type op) {
    return op == ST;
}

//void readConfigFile(std::string config_filename) {
    //std::ifstream fin(config_filename);
//}

void inputConfiguration(std::string config_filename) {
    // Take user input for buffer sizes
    unsigned size;
    int nargs;
    FILE *conf_file = fopen(config_filename.c_str(), "r");
    nargs = fscanf(conf_file, "Size of the reservation station = %d\n", &size);
    rs.setSize(size);
    nargs = fscanf(conf_file, "\nSize of the re-order buffer = %d\n", &size);
    rob.setSize(size);
    nargs = fscanf(conf_file, "\nSize of the store buffer = %d\n", &size);
    sb.setSize(size);

    // Take user input for ALU operation latencies
    int latency;
    nargs = fscanf(conf_file, "\nADD latency = %d\n", &latency);
    IntegerALU::setLatency(ADD, latency);
    nargs = fscanf(conf_file, "\nSUB latency = %d\n", &latency);
    IntegerALU::setLatency(SUB, latency);
    nargs = fscanf(conf_file, "\nMUL latency = %d\n", &latency);
    IntegerALU::setLatency(MUL, latency);
    nargs = fscanf(conf_file, "\nDIV latency = %d\n", &latency);
    IntegerALU::setLatency(DIV, latency);
    nargs = fscanf(conf_file, "\nAND latency = %d\n", &latency);
    IntegerALU::setLatency(AND, latency);
    nargs = fscanf(conf_file, "\nOR latency = %d\n", &latency);
    IntegerALU::setLatency(OR, latency);
    nargs = fscanf(conf_file, "\nXOR latency = %d", &latency);
    IntegerALU::setLatency(XOR, latency);
}

void initialiseMemoryAndARF() {
  for (size_t i=0; i<memory.size(); i++) {
      memory[i] = 1;
  }
  arf.initialise();
}

void fetchDecodeInstructions(std::string instruction_filename) {

    std::ifstream fin(instruction_filename);
    std::string line;
    int i = 1;
    while (std::getline(fin, line)) {

        if (line.empty()) continue;

        instruction instn;
        std::istringstream iss(line);
        std::string op, src[MAX_ARITY], dest;

        iss >> op;
        instn.op = decode_instn(op);
        if (instn.op == INVALID) {
            std::cerr << "Instruction " << i << ": operation invalid." << std::endl;
            std::exit(1);
        }

        if (instn.op == ST) {
            instn.dest_arf_index = -1;
        } else {
            iss >> dest;
            try {
                if (dest[0] == 'R' || dest[0] == 'r') {
                    instn.dest_arf_index = std::stoi(dest.substr(1))-1;
                } else {
                    instn.dest_arf_index = std::stoi(dest.substr(1));
                }
            } catch (std::invalid_argument e) {
                std::cerr << "Instruction " << i << ": destination operand invalid." << std::endl;
                std::exit(1);
            }
        }
 
        try {
            for (int j = 0; j < arity[instn.op]; ++j)
            {
                iss >> src[j];
                if (src[j][0] == 'R' || src[j][0] == 'r') {
                    instn.src[j].field = std::stoi(src[j].substr(1))-1;
                    instn.src[j].is_imm = false;
                } else {
                    instn.src[j].field = std::stoi(src[j]);
                    instn.src[j].is_imm = true;
                }
            }
        } catch (std::invalid_argument e) {
            std::cerr << "Instruction " << i << ": source operand invalid." << std::endl;
            std::exit(1);
        }

        instn.addr = i++;
        instn_buffer.push(instn);
    }

    //for (size_t i = 0; i < instn_buffer.size(); ++i) {
        //instruction &in = instn_buffer[i];
        //std::cout << in.op << "\t";
        //for (int j = 0; j < arity[in.op]; ++j) {
            //std::cout << in.src[j].is_imm << " " << in.src[j].field << ", ";
        //}
        //std::cout << in.dest.is_imm << " " << in.dest_arf_index << std::endl;
    //}
}

// Allocate reservation station entry for given instruction after register renaming
void allocateResStnEntry(const instruction& instn, int dest_rrf_index) {
    // set reservation station entry values
    rs_entry rse;
    rse.op_type = instn.op;
    rse.addr = instn.addr;
    rse.dest = dest_rrf_index;
    // lookup operands
    for (int i = 0; i < arity[rse.op_type]; ++i)
    {
        // if src[i] is an immediate operand, directly copy data to res stn
        if (instn.src[i].is_imm) {
            rse.src[i].ready = true;
            rse.src[i].field = instn.src[i].field;
        } else {
            // get ARF index of src i
            int r = instn.src[i].field;
            // if ARF reg r is not busy, directly copy data from r to res stn
            if (!arf[r].busy) {
                rse.src[i].ready = true;
                rse.src[i].field = arf[r].data;
            } else {
                int s = arf[r].tag;
                // if r is busy and its RRF reg s is valid, directly copy data from s
                if (rrf[s].valid) {
                    rse.src[i].ready = true;
                    rse.src[i].field = rrf[s].data;
                // else, copy r's tag
                } else {
                    rse.src[i].ready = false;
                    rse.src[i].field = s;
                }
            }
        }
    }
    rs.push(rse);
}

// Allocate re-order buffer entry for given instruction after register renaming
void allocateROBEntry(const instruction& instn, int dest_rrf_index) {
    rob_entry robe;
    robe.addr = instn.addr;
    robe.arf_index = instn.dest_arf_index;
    robe.rrf_index = dest_rrf_index;
    rob.push(robe);
}

void dispatchInstn(const instruction& instn) {
    // Check if there is an entry available in each of
    // the RRF, reservation station and re-order buffer
    if (rrf.full() || rs.full() || rob.full()) throw buffer_overflow_error("RRF/RS/ROB full");

    // find free rename register
    int rrf_index = rrf.nextIndex();
    rrf[rrf_index].busy = true;

    // Allocate reservation station entry
    allocateResStnEntry(instn, rrf_index);

    // Rename destination register
    arf[instn.dest_arf_index].busy = true;
    arf[instn.dest_arf_index].tag = rrf_index;

    // Allocate re-order buffer entry
    allocateROBEntry(instn, rrf_index);
}

void dispatchStoreInstn(const instruction &instn) {
    // Check if there is an entry available in both of
    // the reservation station and re-order buffer
    if (rs.full() || rob.full()) throw buffer_overflow_error("RS/ROB full");
      
    // Allocate reservation station entry
    allocateResStnEntry(instn, -1);
    // Allocate re-order buffer entry
    allocateROBEntry(instn, -1);
}

void dispatch(const instruction &instn) {
    if (isStore(instn.op))
        dispatchStoreInstn(instn);
    else
        dispatchInstn(instn);
}

void forwardOperand(int tag, int data) {
    // Forward results to reservation station
    for (int i=0; i < rs.size(); ++i) {
        if (!rs[i].busy) continue;
        for (int j=0; j < MAX_ARITY; ++j) {
            rs_entry::op_info &s = rs[i].src[j];
            if (s.ready) continue;
            if (s.field == tag) {
                s.ready = true;
                s.field = data;
            }
        }
    }
    // Forward results to RRF
    rrf[tag].valid = true;
    rrf[tag].data = data;
}

void execute() {
    // Execute pending instructions for another cycle
    for (size_t i=0; i < alus.size(); ++i) {
        alus[i].updateTimer();
    }
    ldu.updateTimer();
    stu.updateTimer();

    // Forward results from CDB if available
    if (cdb.busy) {
        forwardOperand(cdb.tag, cdb.data);
        cdb.busy = false;
    }

    // Issue ready instructions to functional units
    for (size_t i : rs.indices_in_q_order()) {
        rs_entry &rse = rs[i];
        if (!rse.all_ops_ready()) continue;
        operation_type op = rse.op_type;
        //TODO: Deal with load/stores
        if (isALUOperation(op)) {
            auto alu = std::find_if_not(
                    alus.begin(), alus.end(),
                    [] (IntegerALU a) { return a.busy; } );
            if (alu == alus.end()) continue; // if there is no free ALU
            alu->executeInstn(i);
        } else if (isLoad(op)) {
            if (ldu.busy) continue;
            ldu.executeInstn(i);
        } else if (isStore(op)) {
            if (stu.busy) continue;
            stu.executeInstn(i);
        }
    }
}

void complete() {
    mem_bus.busy = false;
    if (rob.empty()) return;
    rob_entry& head = rob.front();

    // find the res stn entry corresponding to this rob entry, if it exists
    int rs_index = -1;
    for (int i=0; i < rs.size(); ++i) {
        if (rs[i].addr == head.addr) {
            rs_index = i;
            break;
        }
    }
      
    // if the res stn entry was found (wiil only occur when there is a load/store instn)
    if (rs_index >= 0) {
        rs_entry& rse = rs[rs_index];
        if (isStore(rse.op_type)) {
            // Insert a store entry in the store buffer
            if (sb.full()) return;
            sb_entry sbe;
            sbe.mem_addr = rse.src[0].field; // Dest memory address
            sbe.data = rse.src[1].field; // Data to be written
            sb.push(sbe);
            // Remove the store instn from head of ROB
            rob.pop();
        } else if (isLoad(rse.op_type)) {
            unsigned load_mem_addr = rse.src[0].field;
            int i = 0;
            int index = (sb.tail()) ? sb.tail() - 1 : sb.size() - 1;
            for (; i < sb.entryCount(); ++i) {
                // Load forwarding
                if (sb[index].mem_addr == load_mem_addr) {
                    sb[index].rrf_indices_to_update.push(rse.dest);
                    break;
                }
                index = (index) ? index - 1 : sb.size() - 1;
            }
            // Load bypassing if no matching store buffer entry found
            if (i == sb.entryCount()) {
                mem_bus.busy = true;
                forwardOperand(rse.dest, memory[load_mem_addr]);
            }
            // NOTE:- In the case of load, the rob entry is left at the head
            // to be removed in the next iteration
        }
        // Clear res stn entry
        rs.pop(rs_index);
    } else {
        // Write back to ARF
        arf_entry& r = arf[head.arf_index];
        rrf_entry& s = rrf[head.rrf_index];
        if (!s.valid) return;
        r.data = s.data;
        if (r.tag == head.rrf_index) {
            r.busy = false;
        }
        rrf.pop(head.rrf_index);
        rob.pop();
    }
}

void retire() {
    // Wait for memory bus to become free
    // NOTE:- This gives preference to loads over stores as they get
    // a chance to use the bus in the previous complete() step
    if (mem_bus.busy) return;
    if (sb.empty()) return;

    // Get the first entry in the buffer
    sb_entry& head = sb.front();
    // Update memory
    memory[head.mem_addr] = head.data;
    // As long as some pending load needs this data to be forwarded, do so
    if (!head.rrf_indices_to_update.empty()) {
        int rrf_index = head.rrf_indices_to_update.front();
        forwardOperand(rrf_index, head.data);
        head.rrf_indices_to_update.pop();
        if (!head.rrf_indices_to_update.empty())
            return;
    }
    // Remove this entry from the head of the queue
    sb.pop();
}

int main() {
    const static std::string config_filename = "input_files/config.txt";
    const static std::string instruction_filename = "input_files/input.txt";
    unsigned long cycle = 0;
    inputConfiguration(config_filename);
    initialiseMemoryAndARF();
    fetchDecodeInstructions(instruction_filename);
    std::cout << std::setfill('*') << std::setw(80) << "" << std::endl;
    std::cout << std::setfill(' ') << std::setw(43) << "CYCLE " << cycle << std::endl;
    std::cout << std::setfill('*') << std::setw(80) << "" << std::endl;
    std::cout << arf << rrf << rs << rob;
    do {
        ++cycle;
        for (unsigned i=0; i < IPC && !instn_buffer.empty(); ++i) {
            instruction instn = instn_buffer.front();
            try {
                dispatch(instn);
                instn_buffer.pop();
            } catch (buffer_overflow_error e) {
                break;
            }
        }
        execute();
        complete();
	retire();
        
        std::cout << std::setfill('*') << std::setw(80) << "" << std::endl;
        std::cout << std::setfill(' ') << std::setw(43) << "CYCLE " << cycle << std::endl;
        std::cout << std::setfill('*') << std::setw(80) << "" << std::endl;
        std::cout << arf << rrf << rs << rob;
    } while (!rob.empty() || !sb.empty());
    std::cout << "Total number of cycles = " << cycle << std::endl;
    std::cout << arf;
    //std::cout << memory[99] << std::endl;
    return 0;
}
