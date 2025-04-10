#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 256
#define OP_NOP  0x00  // 00000000
#define OP_STA  0x10  // 00010000
#define OP_LDA  0x20  // 00100000
#define OP_ADD  0x30  // 00110000
#define OP_OR   0x40  // 01000000
#define OP_AND  0x50  // 01010000
#define OP_NOT  0x60  // 01100000
#define OP_JMP  0x80  // 10000000
#define OP_JN   0x90  // 10010000
#define OP_JZ   0xA0  // 10100000
#define OP_HLT  0xF0  // 11110000

typedef struct {
    unsigned char memory[MEMORY_SIZE];
    unsigned char accumulator;
    unsigned char PC;  // Program Counter
    unsigned char N;   // Negative flag
    unsigned char Z;   // Zero flag
} NeanderVM;

void init_vm(NeanderVM *vm) {
    memset(vm->memory, 0, MEMORY_SIZE);
    vm->accumulator = 0;
    vm->PC = 0;
    vm->N = 0;
    vm->Z = 0;
}

void load_program(NeanderVM *vm, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open input file %s\n", filename);
        exit(1);
    }
    
    size_t bytes_read = fread(vm->memory, 1, MEMORY_SIZE, file);
    printf("Loaded %zu bytes from %s\n", bytes_read, filename);
    
    fclose(file);
}

void update_flags(NeanderVM *vm) {
    // Update N flag (negative) - check if bit 7 is set
    vm->N = (vm->accumulator & 0x80) ? 1 : 0;
    
    // Update Z flag (zero)
    vm->Z = (vm->accumulator == 0) ? 1 : 0;
}

void print_state(NeanderVM *vm) {
    printf("AC: %02X  PC: %02X  N: %d  Z: %d\n", 
           vm->accumulator, vm->PC, vm->N, vm->Z);
}

void dump_memory(NeanderVM *vm, int start, int end) {
    printf("Memory dump [%02X-%02X]:\n", start, end);
    for (int i = start; i <= end; i++) {
        if (i % 8 == 0) {
            printf("\n%02X: ", i);
        }
        printf("%02X ", vm->memory[i]);
    }
    printf("\n");
}

int execute_instruction(NeanderVM *vm) {
    unsigned char opcode = vm->memory[vm->PC] & 0xF0;  // Higher 4 bits
    unsigned char operand;
    
    // Print current instruction
    printf("Executing at PC=%02X: ", vm->PC);
    
    switch (opcode) {
        case OP_NOP:
            printf("NOP\n");
            vm->PC++;
            break;
            
        case OP_STA:
            operand = vm->memory[vm->PC + 1];
            printf("STA %02X\n", operand);
            vm->memory[operand] = vm->accumulator;
            vm->PC += 2;
            break;
            
        case OP_LDA:
            operand = vm->memory[vm->PC + 1];
            printf("LDA %02X\n", operand);
            vm->accumulator = vm->memory[operand];
            update_flags(vm);
            vm->PC += 2;
            break;
            
        case OP_ADD:
            operand = vm->memory[vm->PC + 1];
            printf("ADD %02X\n", operand);
            vm->accumulator += vm->memory[operand];
            update_flags(vm);
            vm->PC += 2;
            break;
            
        case OP_OR:
            operand = vm->memory[vm->PC + 1];
            printf("OR %02X\n", operand);
            vm->accumulator |= vm->memory[operand];
            update_flags(vm);
            vm->PC += 2;
            break;
            
        case OP_AND:
            operand = vm->memory[vm->PC + 1];
            printf("AND %02X\n", operand);
            vm->accumulator &= vm->memory[operand];
            update_flags(vm);
            vm->PC += 2;
            break;
            
        case OP_NOT:
            printf("NOT\n");
            vm->accumulator = ~vm->accumulator;
            update_flags(vm);
            vm->PC++;
            break;
            
        case OP_JMP:
            operand = vm->memory[vm->PC + 1];
            printf("JMP %02X\n", operand);
            vm->PC = operand;
            break;
            
        case OP_JN:
            operand = vm->memory[vm->PC + 1];
            printf("JN %02X\n", operand);
            if (vm->N) {
                vm->PC = operand;
            } else {
                vm->PC += 2;
            }
            break;
            
        case OP_JZ:
            operand = vm->memory[vm->PC + 1];
            printf("JZ %02X\n", operand);
            if (vm->Z) {
                vm->PC = operand;
            } else {
                vm->PC += 2;
            }
            break;
            
        case OP_HLT:
            printf("HLT\n");
            return 0;  // Signal to stop execution
            
        default:
            printf("Unknown opcode: %02X\n", opcode);
            vm->PC++;
            break;
    }
    
    return 1;  // Continue execution
}

void run(NeanderVM *vm, int max_steps, int verbose) {
    int steps = 0;
    int running = 1;
    
    printf("Starting execution...\n");
    
    while (running && (max_steps == 0 || steps < max_steps)) {
        if (verbose) {
            print_state(vm);
        }
        
        running = execute_instruction(vm);
        steps++;
        
        if (verbose) {
            printf("\n");
        }
    }
    
    printf("\nExecution finished after %d steps.\n", steps);
    print_state(vm);
    
    // Print data section (addresses 0x80-0x8F by default)
    printf("\nFinal data values:\n");
    dump_memory(vm, 0x80, 0x8F);
}

void print_usage(const char *prog_name) {
    printf("Usage: %s <program.bin> [options]\n", prog_name);
    printf("Options:\n");
    printf("  -s, --steps N     Maximum number of steps to execute (0 for unlimited)\n");
    printf("  -v, --verbose     Print detailed execution information\n");
    printf("  -h, --help        Print this help message\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *filename = NULL;
    int max_steps = 1000;  // Default max steps
    int verbose = 0;       // Default verbosity
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--steps") == 0) {
            if (i + 1 < argc) {
                max_steps = atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (filename == NULL) {
            filename = argv[i];
        }
    }
    
    if (filename == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    NeanderVM vm;
    init_vm(&vm);
    load_program(&vm, filename);
    
    run(&vm, max_steps, verbose);
    
    return 0;
}
