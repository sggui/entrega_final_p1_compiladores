#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define MAX_LABELS 256
#define MAX_MEMORY_SIZE 256 // Maximum memory size for Neander
#define MEM_VALUE_SIZE 8

// Neander operation codes (in binary)
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

// Structure to hold label information
typedef struct {
    char name[MAX_LINE_LENGTH];
    int address;
} Label;

// Structure to hold assembled code
typedef struct {
    unsigned char memory[MAX_MEMORY_SIZE];
    int memory_size;
    Label labels[MAX_LABELS];
    int label_count;
    int current_section; // 0 for .CODE, 1 for .DATA
    int code_start;
    int data_start;
} Assembler;

// Initialize the assembler
void init_assembler(Assembler *as) {
    memset(as->memory, 0, MAX_MEMORY_SIZE);
    as->memory_size = 0;
    as->label_count = 0;
    as->current_section = -1;  // Not set yet
    as->code_start = 0;
    as->data_start = 0;
}

// Trim leading and trailing whitespace
void trim(char *str) {
    char *start = str;
    char *end;
    
    // Skip leading whitespace
    while (isspace((unsigned char)*start)) start++;
    
    if (*start == 0) {  // All spaces
        *str = 0;
        return;
    }
    
    // Trim trailing whitespace
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
    
    // Copy the trimmed string back to the original
    if (str != start) {
        memmove(str, start, end - start + 2);
    }
}

// Parse a hexadecimal value
int parse_hex(const char *str) {
    unsigned int value;
    if (sscanf(str, "0x%X", &value) == 1) {
        return value;
    }
    return -1;  // Error
}

// Add a memory value at the current address
void add_memory_value(Assembler *as, unsigned char value) {
    if (as->memory_size < MAX_MEMORY_SIZE) {
        as->memory[as->memory_size++] = value;
    } else {
        fprintf(stderr, "Error: Memory overflow at position %d. Limiting to %d positions.\n", 
                as->memory_size, MAX_MEMORY_SIZE);
        // Don't exit, just warn and continue with limited memory
    }
}

// First pass: collect labels and section markers
void first_pass(Assembler *as, FILE *input) {
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    int address = 0;
    
    while (fgets(line, MAX_LINE_LENGTH, input)) {
        line_number++;
        
        // Remove newline character if present
        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Remove comments (anything after a semicolon)
        char *comment = strchr(line, ';');
        if (comment) {
            *comment = '\0';
        }
        
        // Trim whitespace
        trim(line);
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        // Check for section markers
        if (strcmp(line, ".CODE") == 0) {
            as->current_section = 0;
            as->code_start = address;
            continue;
        } else if (strcmp(line, ".DATA") == 0) {
            as->current_section = 1;
            as->data_start = address;
            continue;
        }
        
        // If in .DATA section, each line is addr value
        if (as->current_section == 1) {
            char *addr_str = strtok(line, " \t");
            char *value_str = strtok(NULL, " \t");
            
            if (addr_str && value_str) {
                int addr = parse_hex(addr_str);
                int value = parse_hex(value_str);
                
                if (addr >= 0 && value >= 0) {
                    // In .DATA section, we just record the address but don't increment
                    // the code address counter
                    continue;
                }
            }
            fprintf(stderr, "Error: Invalid data format at line %d: %s\n", line_number, line);
        }
        // For .CODE section and any labels, increment the address
        address++;
    }
    
    // Reset the file position
    rewind(input);
}

// Process a single instruction
void process_instruction(Assembler *as, char *instr, char *operand) {
    // Convert instruction to uppercase
    for (char *p = instr; *p; ++p) *p = toupper(*p);
    
    if (strcmp(instr, "NOP") == 0) {
        add_memory_value(as, OP_NOP);
    } else if (strcmp(instr, "STA") == 0) {
        add_memory_value(as, OP_STA);
        // Parse hex address and add it
        int addr = parse_hex(operand);
        if (addr >= 0) {
            add_memory_value(as, addr);
        } else {
            fprintf(stderr, "Error: Invalid operand for STA: %s\n", operand);
            exit(1);
        }
    } else if (strcmp(instr, "LDA") == 0) {
        add_memory_value(as, OP_LDA);
        int addr = parse_hex(operand);
        if (addr >= 0) {
            add_memory_value(as, addr);
        } else {
            fprintf(stderr, "Error: Invalid operand for LDA: %s\n", operand);
            exit(1);
        }
    } else if (strcmp(instr, "ADD") == 0) {
        add_memory_value(as, OP_ADD);
        int addr = parse_hex(operand);
        if (addr >= 0) {
            add_memory_value(as, addr);
        } else {
            fprintf(stderr, "Error: Invalid operand for ADD: %s\n", operand);
            exit(1);
        }
    } else if (strcmp(instr, "OR") == 0) {
        add_memory_value(as, OP_OR);
        int addr = parse_hex(operand);
        if (addr >= 0) {
            add_memory_value(as, addr);
        } else {
            fprintf(stderr, "Error: Invalid operand for OR: %s\n", operand);
            exit(1);
        }
    } else if (strcmp(instr, "AND") == 0) {
        add_memory_value(as, OP_AND);
        int addr = parse_hex(operand);
        if (addr >= 0) {
            add_memory_value(as, addr);
        } else {
            fprintf(stderr, "Error: Invalid operand for AND: %s\n", operand);
            exit(1);
        }
    } else if (strcmp(instr, "NOT") == 0) {
        add_memory_value(as, OP_NOT);
    } else if (strcmp(instr, "JMP") == 0) {
        add_memory_value(as, OP_JMP);
        int addr = parse_hex(operand);
        if (addr >= 0) {
            add_memory_value(as, addr);
        } else {
            fprintf(stderr, "Error: Invalid operand for JMP: %s\n", operand);
            exit(1);
        }
    } else if (strcmp(instr, "JN") == 0) {
        add_memory_value(as, OP_JN);
        int addr = parse_hex(operand);
        if (addr >= 0) {
            add_memory_value(as, addr);
        } else {
            fprintf(stderr, "Error: Invalid operand for JN: %s\n", operand);
            exit(1);
        }
    } else if (strcmp(instr, "JZ") == 0) {
        add_memory_value(as, OP_JZ);
        int addr = parse_hex(operand);
        if (addr >= 0) {
            add_memory_value(as, addr);
        } else {
            fprintf(stderr, "Error: Invalid operand for JZ: %s\n", operand);
            exit(1);
        }
    } else if (strcmp(instr, "HLT") == 0) {
        add_memory_value(as, OP_HLT);
    } else {
        fprintf(stderr, "Error: Unknown instruction: %s\n", instr);
        exit(1);
    }
}

// Second pass: actually assemble the code
void second_pass(Assembler *as, FILE *input) {
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    int current_code_address = 0;
    
    as->current_section = -1;  // Reset section marker
    
    // Initialize memory to NOP
    memset(as->memory, OP_NOP, MAX_MEMORY_SIZE);
    
    while (fgets(line, MAX_LINE_LENGTH, input)) {
        line_number++;
        
        // Remove newline character if present
        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Remove comments (anything after a semicolon)
        char *comment = strchr(line, ';');
        if (comment) {
            *comment = '\0';
        }
        
        // Trim whitespace
        trim(line);
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        // Check for section markers
        if (strcmp(line, ".CODE") == 0) {
            as->current_section = 0;
            current_code_address = 0;
            continue;
        } else if (strcmp(line, ".DATA") == 0) {
            as->current_section = 1;
            continue;
        }
        
        // Process .DATA section
        if (as->current_section == 1) {
            char *addr_str = strtok(line, " \t");
            char *value_str = strtok(NULL, " \t");
            
            if (addr_str && value_str) {
                int addr = parse_hex(addr_str);
                int value = parse_hex(value_str);
                
                if (addr >= 0 && value >= 0 && addr < MAX_MEMORY_SIZE) {
                    as->memory[addr] = (unsigned char)value;
                    // Update memory_size if necessary
                    if (addr >= as->memory_size) {
                        as->memory_size = addr + 1;
                    }
                } else {
                    fprintf(stderr, "Error at line %d: Invalid data format or address out of range\n", line_number);
                    exit(1);
                }
            }
        }
        // Process .CODE section
        else if (as->current_section == 0) {
            char instr[MAX_LINE_LENGTH] = {0};
            char operand[MAX_LINE_LENGTH] = {0};
            
            if (sscanf(line, "%s %s", instr, operand) >= 1) {
                // Convert instruction to uppercase
                for (char *p = instr; *p; ++p) *p = toupper(*p);
                
                if (strcmp(instr, "NOP") == 0) {
                    as->memory[current_code_address++] = OP_NOP;
                } else if (strcmp(instr, "STA") == 0) {
                    as->memory[current_code_address++] = OP_STA;
                    // Parse hex address and add it
                    int addr = parse_hex(operand);
                    if (addr >= 0) {
                        as->memory[current_code_address++] = addr;
                    } else {
                        fprintf(stderr, "Error: Invalid operand for STA: %s\n", operand);
                        exit(1);
                    }
                } else if (strcmp(instr, "LDA") == 0) {
                    as->memory[current_code_address++] = OP_LDA;
                    int addr = parse_hex(operand);
                    if (addr >= 0) {
                        as->memory[current_code_address++] = addr;
                    } else {
                        fprintf(stderr, "Error: Invalid operand for LDA: %s\n", operand);
                        exit(1);
                    }
                } else if (strcmp(instr, "ADD") == 0) {
                    as->memory[current_code_address++] = OP_ADD;
                    int addr = parse_hex(operand);
                    if (addr >= 0) {
                        as->memory[current_code_address++] = addr;
                    } else {
                        fprintf(stderr, "Error: Invalid operand for ADD: %s\n", operand);
                        exit(1);
                    }
                } else if (strcmp(instr, "OR") == 0) {
                    as->memory[current_code_address++] = OP_OR;
                    int addr = parse_hex(operand);
                    if (addr >= 0) {
                        as->memory[current_code_address++] = addr;
                    } else {
                        fprintf(stderr, "Error: Invalid operand for OR: %s\n", operand);
                        exit(1);
                    }
                } else if (strcmp(instr, "AND") == 0) {
                    as->memory[current_code_address++] = OP_AND;
                    int addr = parse_hex(operand);
                    if (addr >= 0) {
                        as->memory[current_code_address++] = addr;
                    } else {
                        fprintf(stderr, "Error: Invalid operand for AND: %s\n", operand);
                        exit(1);
                    }
                } else if (strcmp(instr, "NOT") == 0) {
                    as->memory[current_code_address++] = OP_NOT;
                } else if (strcmp(instr, "JMP") == 0) {
                    as->memory[current_code_address++] = OP_JMP;
                    int addr = parse_hex(operand);
                    if (addr >= 0) {
                        as->memory[current_code_address++] = addr;
                    } else {
                        fprintf(stderr, "Error: Invalid operand for JMP: %s\n", operand);
                        exit(1);
                    }
                } else if (strcmp(instr, "JN") == 0) {
                    as->memory[current_code_address++] = OP_JN;
                    int addr = parse_hex(operand);
                    if (addr >= 0) {
                        as->memory[current_code_address++] = addr;
                    } else {
                        fprintf(stderr, "Error: Invalid operand for JN: %s\n", operand);
                        exit(1);
                    }
                } else if (strcmp(instr, "JZ") == 0) {
                    as->memory[current_code_address++] = OP_JZ;
                    int addr = parse_hex(operand);
                    if (addr >= 0) {
                        as->memory[current_code_address++] = addr;
                    } else {
                        fprintf(stderr, "Error: Invalid operand for JZ: %s\n", operand);
                        exit(1);
                    }
                } else if (strcmp(instr, "HLT") == 0) {
                    as->memory[current_code_address++] = OP_HLT;
                } else {
                    fprintf(stderr, "Error: Unknown instruction: %s\n", instr);
                    exit(1);
                }
                
                // Update memory_size if needed
                if (current_code_address > as->memory_size) {
                    as->memory_size = current_code_address;
                }
            } else {
                fprintf(stderr, "Error at line %d: Invalid instruction format\n", line_number);
                exit(1);
            }
        }
    }
}

// Output memory in MEM format for Neander
void output_mem(Assembler *as, FILE *output) {
    // Neander seems to expect a simpler format with just binary values
    // One byte per line in binary format
    for (int i = 0; i < MAX_MEMORY_SIZE; i++) {
        fprintf(output, "%c%c%c%c%c%c%c%c\n", 
            (as->memory[i] & 0x80) ? '1' : '0',
            (as->memory[i] & 0x40) ? '1' : '0',
            (as->memory[i] & 0x20) ? '1' : '0',
            (as->memory[i] & 0x10) ? '1' : '0',
            (as->memory[i] & 0x08) ? '1' : '0',
            (as->memory[i] & 0x04) ? '1' : '0',
            (as->memory[i] & 0x02) ? '1' : '0',
            (as->memory[i] & 0x01) ? '1' : '0');
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.asm> <output.mem>\n", argv[0]);
        return 1;
    }
    
    FILE *input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file %s\n", argv[1]);
        return 1;
    }
    
    FILE *output = fopen(argv[2], "w");  // Abrir em modo texto
    if (!output) {
        fprintf(stderr, "Error: Cannot open output file %s\n", argv[2]);
        fclose(input);
        return 1;
    }
    
    Assembler as;
    init_assembler(&as);
    
    // First pass to collect labels and sections
    first_pass(&as, input);
    
    // Second pass to generate code
    second_pass(&as, input);
    
    // Output the assembled code in MEM format
    output_mem(&as, output);
    
    fclose(input);
    fclose(output);
    
    printf("Assembly completed successfully. Output written to %s\n", argv[2]);
    return 0;
}
