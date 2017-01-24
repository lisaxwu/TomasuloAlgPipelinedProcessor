#ifndef PROCSIM_HPP
#define PROCSIM_HPP

#include <cstdint>
#include <cstdio>


#define DEFAULT_K0 3
#define DEFAULT_K1 2
#define DEFAULT_K2 1
#define DEFAULT_R 2
#define DEFAULT_F 4

typedef struct _proc_inst_t
{
    uint32_t instruction_address;
    int32_t op_code;
    int32_t src_reg[2];
    int32_t dest_reg;
    
    // You may introduce other fields as needed
    
} proc_inst_t;

typedef struct _proc_stats_t
{
    float avg_inst_retired;
    float avg_inst_fired;
    float avg_disp_size;
    unsigned long max_disp_size;
    unsigned long retired_instruction;
    unsigned long cycle_count;
} proc_stats_t;


typedef struct ScheQ
{   
    //bool valid;   //used or unused

    int32_t inst_num;

    int32_t func;
    int32_t dest_reg;
    int32_t dest_reg_tag;
    bool src0_ready;
    int32_t src0_tag;
    bool src1_ready;
    int32_t src1_tag;

    bool intoFU;

    struct ScheQ* next;
    
} ScheQ;

// typedef struct DispQ
// {   
//     proc_inst_t theinst;

//     int32_t inst_num;

//     struct DispQ* next;
    
// } DispQ;

typedef struct DispQ
{   
    unsigned int size;
    int cycle;
    struct DispQ* next;

} DispQ;

typedef struct RegFile
{   
    int32_t tag;
    bool ready;
    
} RegFile;

typedef struct CompQ
{   
    ScheQ* theRS;  
    struct CompQ* next;

} CompQ;


typedef struct InsRecord
{   
    int32_t inst_num;

    int32_t fetch;
    int32_t disp;
    int32_t sched;
    int32_t exec;
    int32_t state;

    bool finished;

    struct InsRecord* next;
    
} InsRecord;


bool read_instruction(proc_inst_t* p_inst);
bool my_read_instruction(proc_inst_t* p_inst);

void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f);
void run_proc(proc_stats_t* p_stats);
void complete_proc(proc_stats_t* p_stats);

#endif /* PROCSIM_HPP */
