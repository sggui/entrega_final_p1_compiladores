#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_SIZE 1024
#define MAX_TOKEN_SIZE 256
#define MAX_VARIABLES 100
#define MAX_TOKENS 100
#define MAX_INSTRUCTIONS 1000
#define MAX_PROGRAM_SIZE 10000

#define INITIAL_MEMORY_ADDRESS 0x80 // 80 in hexadecimal
#define TEMP_MEMORY_START 0xC8 // 200 in decimal (C8 in hex)
#define CODE_START_ADDRESS 0x00 // Starting address for code

// Token types
typedef enum {
    TOKEN_EOF,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_VARIABLE,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EQUALS,
    TOKEN_COLON,
    TOKEN_PROGRAMA,
    TOKEN_INICIO,
    TOKEN_FIM,
    TOKEN_RES,
    TOKEN_QUOTE,
    TOKEN_UNKNOWN
} TokenType;

// Instruction types
typedef enum {
    INSTR_NOP,
    INSTR_STA,
    INSTR_LDA,
    INSTR_ADD,
    INSTR_OR,
    INSTR_AND,
    INSTR_NOT,
    INSTR_JMP,
    INSTR_JN,
    INSTR_JZ,
    INSTR_HLT
} InstructionType;

// Instruction structure
typedef struct {
    InstructionType type;
    int operand;    // -1 if no operand
    int address;    // Address of the instruction in the program
} Instruction;

// Token structure
typedef struct {
    TokenType type;
    char value[MAX_TOKEN_SIZE];
    int position;
} Token;

// Lexer structure
typedef struct {
    const char *input;
    int position;
    int length;
    Token current_token;
} Lexer;

// Variable structure
typedef struct {
    char name[MAX_TOKEN_SIZE];
    int address;
    int value;
    int initialized;
} Variable;

// Compiler structure
typedef struct {
    Variable variables[MAX_VARIABLES];
    int var_count;
    int next_address;
    int temp_address;
    Instruction instructions[MAX_INSTRUCTIONS];
    int instruction_count;
    Lexer lexer;
} Compiler;

// Forward declarations for recursive descent parser
int parse_expression(Compiler *c);
int parse_term(Compiler *c);
int parse_factor(Compiler *c);

// Compiler initialization
void init_compiler(Compiler *c) {
    c->var_count = 0;
    c->next_address = INITIAL_MEMORY_ADDRESS;
    c->temp_address = TEMP_MEMORY_START;
    c->instruction_count = 0;
}

// Add an instruction to the compiler
int add_instruction(Compiler *c, InstructionType type, int operand) {
    if (c->instruction_count >= MAX_INSTRUCTIONS) {
        fprintf(stderr, "Error: Instruction buffer overflow\n");
        return -1;
    }
    
    c->instructions[c->instruction_count].type = type;
    c->instructions[c->instruction_count].operand = operand;
    c->instructions[c->instruction_count].address = c->instruction_count;
    
    return c->instruction_count++;
}

// Modify an existing instruction
void modify_instruction(Compiler *c, int index, InstructionType type, int operand) {
    if (index < 0 || index >= c->instruction_count) {
        fprintf(stderr, "Error: Invalid instruction index %d\n", index);
        return;
    }
    
    c->instructions[index].type = type;
    c->instructions[index].operand = operand;
}

// Variable management functions
int find_variable(Compiler *c, const char *name) {
    for (int i = 0; i < c->var_count; i++) {
        if (strcmp(c->variables[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int add_variable(Compiler *c, const char *name, int value, int initialized) {
    int index = find_variable(c, name);
    if (index >= 0) {
        // If the variable exists but wasn't initialized, update it
        if (!c->variables[index].initialized && initialized) {
            c->variables[index].value = value;
            c->variables[index].initialized = initialized;
        }
        return index;
    }
    strcpy(c->variables[c->var_count].name, name);
    c->variables[c->var_count].address = c->next_address++;
    c->variables[c->var_count].value = value;
    c->variables[c->var_count].initialized = initialized;
    return c->var_count++;
}

int add_constant(Compiler *c, int value) {
    char constant_name[MAX_TOKEN_SIZE];
    sprintf(constant_name, "_const_%d", value);
    int index = find_variable(c, constant_name);
    if (index >= 0) {
        return index;
    }
    return add_variable(c, constant_name, value, 1);
}

int get_temp_address(Compiler *c) {
    return c->temp_address++;
}

// Lexer functions
void init_lexer(Lexer *lexer, const char *input) {
    lexer->input = input;
    lexer->position = 0;
    lexer->length = strlen(input);
    // Initialize with an empty token
    lexer->current_token.type = TOKEN_UNKNOWN;
    lexer->current_token.value[0] = '\0';
    lexer->current_token.position = 0;
}

// Skip whitespace characters
void skip_whitespace(Lexer *lexer) {
    while (lexer->position < lexer->length && 
           (lexer->input[lexer->position] == ' ' || 
            lexer->input[lexer->position] == '\t')) {
        lexer->position++;
    }
}

// Check if string is a reserved keyword
TokenType check_keyword(const char *str) {
    if (strcmp(str, "PROGRAMA") == 0) return TOKEN_PROGRAMA;
    if (strcmp(str, "INICIO") == 0) return TOKEN_INICIO;
    if (strcmp(str, "FIM") == 0) return TOKEN_FIM;
    if (strcmp(str, "RES") == 0) return TOKEN_RES;
    return TOKEN_VARIABLE;
}

// Get the next token
Token get_next_token(Lexer *lexer) {
    Token token;
    token.position = lexer->position;
    token.value[0] = '\0';
    
    skip_whitespace(lexer);
    
    if (lexer->position >= lexer->length) {
        token.type = TOKEN_EOF;
        return token;
    }
    
    char c = lexer->input[lexer->position];
    
    // Handle newlines
    if (c == '\n') {
        lexer->position++;
        while (lexer->position < lexer->length && 
               (lexer->input[lexer->position] == '\n' || 
                lexer->input[lexer->position] == '\r')) {
            lexer->position++;
        }
        token.type = TOKEN_UNKNOWN; // Skip newlines for now
        token.value[0] = '\n';
        token.value[1] = '\0';
        return token;
    }
    
    // Handle numbers
    if (isdigit(c)) {
        int i = 0;
        while (lexer->position < lexer->length && 
               isdigit(lexer->input[lexer->position])) {
            token.value[i++] = lexer->input[lexer->position++];
        }
        token.value[i] = '\0';
        token.type = TOKEN_NUMBER;
        return token;
    }
    
    // Handle identifiers and keywords
    if (isalpha(c) || c == '_') {
        int i = 0;
        while (lexer->position < lexer->length && 
               (isalnum(lexer->input[lexer->position]) || 
                lexer->input[lexer->position] == '_')) {
            token.value[i++] = lexer->input[lexer->position++];
        }
        token.value[i] = '\0';
        token.type = check_keyword(token.value);
        return token;
    }
    
    // Handle quotation mark for program name
    if (c == '"') {
        token.type = TOKEN_QUOTE;
        token.value[0] = c;
        token.value[1] = '\0';
        lexer->position++;
        return token;
    }
    
    // Handle operators and special characters
    lexer->position++;
    token.value[0] = c;
    token.value[1] = '\0';
    
    switch (c) {
        case '+': token.type = TOKEN_PLUS; break;
        case '-': token.type = TOKEN_MINUS; break;
        case '*': token.type = TOKEN_MULTIPLY; break;
        case '/': token.type = TOKEN_DIVIDE; break;
        case '(': token.type = TOKEN_LPAREN; break;
        case ')': token.type = TOKEN_RPAREN; break;
        case '=': token.type = TOKEN_EQUALS; break;
        case ':': token.type = TOKEN_COLON; break;
        default: token.type = TOKEN_UNKNOWN; break;
    }
    
    return token;
}

// Function to advance to the next valid token
void advance(Lexer *lexer) {
    do {
        lexer->current_token = get_next_token(lexer);
    } while (lexer->current_token.type == TOKEN_UNKNOWN);
}

// Check if the current token is of the expected type
int match(Lexer *lexer, TokenType type) {
    if (lexer->current_token.type == type) {
        advance(lexer);
        return 1;
    }
    return 0;
}

// Helper functions
void clean_line(char *line) {
    char *comment = strchr(line, ';');
    if (comment) {
        *comment = '\0';
    }
    int len = strlen(line);
    while (len > 0 && isspace(line[len-1])) {
        line[--len] = '\0';
    }
}

// Generate code to load a value into the accumulator
void load_accumulator(Compiler *c, int address) {
    add_instruction(c, INSTR_LDA, address);
}

// Generate code to store the accumulator value to an address
void store_accumulator(Compiler *c, int address) {
    add_instruction(c, INSTR_STA, address);
}

// Code generation for multiplication
void generate_multiplication(Compiler *c, int operand1_addr, int operand2_addr, int result_addr) {
    int counter_addr = get_temp_address(c);
    
    // Initialize result as 0
    int zero_idx = find_variable(c, "_zero");
    if (zero_idx < 0) {
        zero_idx = add_variable(c, "_zero", 0, 1);
    }
    load_accumulator(c, c->variables[zero_idx].address);
    store_accumulator(c, result_addr);
    
    // Initialize counter with operand1
    load_accumulator(c, operand1_addr);
    store_accumulator(c, counter_addr);
    
    // Start of multiplication loop
    int loop_start = c->instruction_count;
    
    // Check if counter is zero
    load_accumulator(c, counter_addr);
    int jz_instr = add_instruction(c, INSTR_JZ, -1); // Placeholder for exit address
    
    // Add operand2 to result
    load_accumulator(c, result_addr);
    add_instruction(c, INSTR_ADD, operand2_addr);
    store_accumulator(c, result_addr);
    
    // Decrement counter
    load_accumulator(c, counter_addr);
    int neg_one_idx = find_variable(c, "_neg_one");
    if (neg_one_idx < 0) {
        neg_one_idx = add_variable(c, "_neg_one", 255, 1); // 255 in 8 bits = -1
    }
    add_instruction(c, INSTR_ADD, c->variables[neg_one_idx].address);
    store_accumulator(c, counter_addr);
    
    // Jump back to start of loop
    add_instruction(c, INSTR_JMP, loop_start * 2);  // Adjusting for correct address
    
    // Update JZ exit address
    modify_instruction(c, jz_instr, INSTR_JZ, c->instruction_count * 2);
}

// Code generation for division
void generate_division(Compiler *c, int dividend_addr, int divisor_addr, int result_addr) {
    int remainder_addr = get_temp_address(c);
    int temp_addr = get_temp_address(c);
    
    // Initialize result as 0
    int zero_idx = find_variable(c, "_zero");
    if (zero_idx < 0) {
        zero_idx = add_variable(c, "_zero", 0, 1);
    }
    load_accumulator(c, c->variables[zero_idx].address);
    store_accumulator(c, result_addr);
    
    // Initialize remainder with dividend
    load_accumulator(c, dividend_addr);
    store_accumulator(c, remainder_addr);
    
    // Start of division loop
    int loop_start = c->instruction_count;
    
    // Check if remainder < divisor (exit condition)
    load_accumulator(c, remainder_addr);
    add_instruction(c, INSTR_NOT, -1);  // NOT for complementary
    add_instruction(c, INSTR_ADD, divisor_addr);  // A + (~B + 1) = A - B
    int one_idx = add_variable(c, "_one", 1, 1);
    add_instruction(c, INSTR_ADD, c->variables[one_idx].address);
    
    // If result is negative, jump to exit
    int jn_instr = add_instruction(c, INSTR_JN, -1);  // Placeholder
    
    // Subtract divisor from remainder
    load_accumulator(c, remainder_addr);
    add_instruction(c, INSTR_NOT, -1);
    add_instruction(c, INSTR_ADD, c->variables[one_idx].address);  // -remainder
    add_instruction(c, INSTR_ADD, divisor_addr);
    add_instruction(c, INSTR_NOT, -1);
    add_instruction(c, INSTR_ADD, c->variables[one_idx].address);  // -((-remainder) + divisor) = remainder - divisor
    store_accumulator(c, remainder_addr);
    
    // Increment result
    load_accumulator(c, result_addr);
    add_instruction(c, INSTR_ADD, c->variables[one_idx].address);
    store_accumulator(c, result_addr);
    
    // Jump back to start of loop
    add_instruction(c, INSTR_JMP, loop_start * 2);
    
    // Update JN exit address
    modify_instruction(c, jn_instr, INSTR_JN, c->instruction_count * 2);
}

// Recursive descent parser
int parse_factor(Compiler *c) {
    Lexer *lexer = &c->lexer;
    int result_addr = get_temp_address(c);
    
    // Handle numbers
    if (lexer->current_token.type == TOKEN_NUMBER) {
        int value = atoi(lexer->current_token.value);
        int const_idx = add_constant(c, value);
        load_accumulator(c, c->variables[const_idx].address);
        store_accumulator(c, result_addr);
        advance(lexer);
    }
    // Handle variables
    else if (lexer->current_token.type == TOKEN_VARIABLE) {
        char var_name[MAX_TOKEN_SIZE];
        strcpy(var_name, lexer->current_token.value);
        int var_idx = find_variable(c, var_name);
        if (var_idx < 0) {
            var_idx = add_variable(c, var_name, 0, 0);
        }
        load_accumulator(c, c->variables[var_idx].address);
        store_accumulator(c, result_addr);
        advance(lexer);
    }
    // Handle parenthesized expressions
    else if (lexer->current_token.type == TOKEN_LPAREN) {
        advance(lexer); // Consume '('
        int expr_result = parse_expression(c);
        
        // Check for closing parenthesis
        if (lexer->current_token.type != TOKEN_RPAREN) {
            fprintf(stderr, "Error: Expected closing parenthesis\n");
            return result_addr; // Return address to continue compilation
        }
        advance(lexer); // Consume ')'
        
        // Copy expression result to result address
        load_accumulator(c, expr_result);
        store_accumulator(c, result_addr);
    }
    // Handle unary minus
    else if (lexer->current_token.type == TOKEN_MINUS) {
        advance(lexer); // Consume '-'
        
        // Parse the factor following the minus
        int factor_addr = parse_factor(c);
        
        // Calculate 2's complement to negate the value
        load_accumulator(c, factor_addr);
        add_instruction(c, INSTR_NOT, -1);
        int one_idx = add_variable(c, "_one", 1, 1);
        add_instruction(c, INSTR_ADD, c->variables[one_idx].address);
        store_accumulator(c, result_addr);
    }
    else {
        fprintf(stderr, "Error: Unexpected token in factor at position %d\n", lexer->current_token.position);
        // Advance to try to recover from errors
        advance(lexer);
    }
    
    return result_addr;
}

int parse_term(Compiler *c) {
    Lexer *lexer = &c->lexer;
    
    // Parse the first factor
    int left_addr = parse_factor(c);
    
    // Process * and / operators repeatedly
    while (lexer->current_token.type == TOKEN_MULTIPLY || 
           lexer->current_token.type == TOKEN_DIVIDE) {
        TokenType op_type = lexer->current_token.type;
        advance(lexer); // Consume the operator
        
        // Parse the next factor
        int right_addr = parse_factor(c);
        int result_addr = get_temp_address(c);
        
        // Generate code for the operation
        if (op_type == TOKEN_MULTIPLY) {
            generate_multiplication(c, left_addr, right_addr, result_addr);
        } else { // TOKEN_DIVIDE
            generate_division(c, left_addr, right_addr, result_addr);
        }
        
        // The result of this operation becomes the left operand for the next
        left_addr = result_addr;
    }
    
    return left_addr;
}

int parse_expression(Compiler *c) {
    Lexer *lexer = &c->lexer;
    
    // Parse the first term
    int left_addr = parse_term(c);
    
    // Process + and - operators repeatedly
    while (lexer->current_token.type == TOKEN_PLUS || 
           lexer->current_token.type == TOKEN_MINUS) {
        TokenType op_type = lexer->current_token.type;
        advance(lexer); // Consume the operator
        
        // Parse the next term
        int right_addr = parse_term(c);
        int result_addr = get_temp_address(c);
        
        // Generate code for the operation
        if (op_type == TOKEN_PLUS) {
            load_accumulator(c, left_addr);
            add_instruction(c, INSTR_ADD, right_addr);
            store_accumulator(c, result_addr);
        } else { // TOKEN_MINUS
            // For subtraction, negate the second operand and add
            load_accumulator(c, right_addr);
            add_instruction(c, INSTR_NOT, -1);
            int one_idx = add_variable(c, "_one", 1, 1);
            add_instruction(c, INSTR_ADD, c->variables[one_idx].address);
            store_accumulator(c, right_addr);
            
            load_accumulator(c, left_addr);
            add_instruction(c, INSTR_ADD, right_addr);
            store_accumulator(c, result_addr);
        }
        
        // The result of this operation becomes the left operand for the next
        left_addr = result_addr;
    }
    
    return left_addr;
}

// Parse a variable assignment
int parse_assignment(Compiler *c) {
    Lexer *lexer = &c->lexer;
    char var_name[MAX_TOKEN_SIZE];
    
    // Check for variable name
    if (lexer->current_token.type != TOKEN_VARIABLE) {
        fprintf(stderr, "Error: Expected variable name in assignment\n");
        return -1;
    }
    
    strcpy(var_name, lexer->current_token.value);
    advance(lexer); // Consume variable name
    
    // Check for equals sign
    if (lexer->current_token.type != TOKEN_EQUALS) {
        fprintf(stderr, "Error: Expected '=' in assignment\n");
        return -1;
    }
    advance(lexer); // Consume '='
    
    // Parse the expression
    int expr_result = parse_expression(c);
    
    // Store the result in the variable
    int var_idx = add_variable(c, var_name, 0, 1);
    int var_addr = c->variables[var_idx].address;
    
    load_accumulator(c, expr_result);
    store_accumulator(c, var_addr);
    
    return var_addr;
}

// Parse the result statement
int parse_result(Compiler *c) {
    Lexer *lexer = &c->lexer;
    
    // Check for RES keyword
    if (lexer->current_token.type != TOKEN_RES) {
        fprintf(stderr, "Error: Expected 'RES' keyword\n");
        return -1;
    }
    advance(lexer); // Consume RES
    
    // Check for equals sign
    if (lexer->current_token.type != TOKEN_EQUALS) {
        fprintf(stderr, "Error: Expected '=' after RES\n");
        return -1;
    }
    advance(lexer); // Consume '='
    
    // Parse the expression
    int result_addr = parse_expression(c);
    
    // The result is left in the accumulator
    load_accumulator(c, result_addr);
    
    return result_addr;
}

// Parse the program identifier
int parse_program_identifier(Compiler *c) {
    Lexer *lexer = &c->lexer;
    
    // Check for opening quote
    if (lexer->current_token.type != TOKEN_QUOTE) {
        fprintf(stderr, "Error: Expected '\"' for program name\n");
        return -1;
    }
    advance(lexer); // Consume opening quote
    
    // Check for identifier
    if (lexer->current_token.type != TOKEN_VARIABLE) {
        fprintf(stderr, "Error: Expected program name\n");
        return -1;
    }
    
    // Store program name (could be used for metadata)
    char program_name[MAX_TOKEN_SIZE];
    strcpy(program_name, lexer->current_token.value);
    advance(lexer); // Consume program name
    
    // Check for closing quote
    if (lexer->current_token.type != TOKEN_QUOTE) {
        fprintf(stderr, "Error: Expected closing '\"' for program name\n");
        return -1;
    }
    advance(lexer); // Consume closing quote
    
    return 0;
}

// Parse the entire program module
int parse_module(Compiler *c) {
    Lexer *lexer = &c->lexer;
    
    // Check for PROGRAMA keyword
    if (lexer->current_token.type != TOKEN_PROGRAMA) {
        fprintf(stderr, "Error: Expected 'PROGRAMA' keyword\n");
        return -1;
    }
    advance(lexer); // Consume PROGRAMA
    
    // Parse program identifier
    if (parse_program_identifier(c) < 0) {
        return -1;
    }
    
    // Check for colon
    if (lexer->current_token.type != TOKEN_COLON) {
        fprintf(stderr, "Error: Expected ':' after program name\n");
        return -1;
    }
    advance(lexer); // Consume colon
    
    // Check for INICIO keyword
    if (lexer->current_token.type != TOKEN_INICIO) {
        fprintf(stderr, "Error: Expected 'INICIO' keyword\n");
        return -1;
    }
    advance(lexer); // Consume INICIO
    
    // Parse statements until we find the RES or FIM
    while (lexer->current_token.type != TOKEN_RES && 
           lexer->current_token.type != TOKEN_FIM) {
        if (lexer->current_token.type == TOKEN_EOF) {
            fprintf(stderr, "Error: Unexpected end of file\n");
            return -1;
        }
        
        // Parse assignment statement
        if (lexer->current_token.type == TOKEN_VARIABLE) {
            if (parse_assignment(c) < 0) {
                return -1;
            }
        } else {
            fprintf(stderr, "Error: Expected variable assignment\n");
            advance(lexer); // Try to recover
        }
    }
    
    // Parse result statement if present
    if (lexer->current_token.type == TOKEN_RES) {
        if (parse_result(c) < 0) {
            return -1;
        }
    }
    
    // Check for FIM keyword
    if (lexer->current_token.type != TOKEN_FIM) {
        fprintf(stderr, "Error: Expected 'FIM' keyword\n");
        return -1;
    }
    advance(lexer); // Consume FIM
    
    return 0;
}

// Convert instructions to assembly code
void generate_assembly_code(Compiler *c, FILE *output) {
    for (int i = 0; i < c->instruction_count; i++) {
        Instruction *instr = &c->instructions[i];
        
        switch (instr->type) {
            case INSTR_NOP:
                fprintf(output, "NOP\n");
                break;
            case INSTR_STA:
                fprintf(output, "STA 0x%X\n", instr->operand);
                break;
            case INSTR_LDA:
                fprintf(output, "LDA 0x%X\n", instr->operand);
                break;
            case INSTR_ADD:
                fprintf(output, "ADD 0x%X\n", instr->operand);
                break;
            case INSTR_OR:
                fprintf(output, "OR 0x%X\n", instr->operand);
                break;
            case INSTR_AND:
                fprintf(output, "AND 0x%X\n", instr->operand);
                break;
            case INSTR_NOT:
                fprintf(output, "NOT\n");
                break;
            case INSTR_JMP:
                fprintf(output, "JMP 0x%X\n", instr->operand);
                break;
            case INSTR_JN:
                fprintf(output, "JN 0x%X\n", instr->operand);
                break;
            case INSTR_JZ:
                fprintf(output, "JZ 0x%X\n", instr->operand);
                break;
            case INSTR_HLT:
                fprintf(output, "HLT\n");
                break;
            default:
                fprintf(stderr, "Error: Unknown instruction type %d\n", instr->type);
                break;
        }
    }
}

// Generate the data section
void generate_data_section(Compiler *c, FILE *output) {
    fprintf(output, ".DATA\n");
    for (int i = 0; i < c->var_count; i++) {
        fprintf(output, "0x%X 0x%X\n", c->variables[i].address, c->variables[i].value);
    }
}

// Main compilation function
void compile(const char *source_code, FILE *output) {
    Compiler compiler;
    init_compiler(&compiler);
    
    // Add constant values
    add_variable(&compiler, "_zero", 0, 1);
    add_variable(&compiler, "_one", 1, 1);
    add_variable(&compiler, "_neg_one", 255, 1); // 255 in 8 bits = -1
    
    // Initialize lexer
    init_lexer(&compiler.lexer, source_code);
    advance(&compiler.lexer); // Get first token
    
    // Parse the module
    if (parse_module(&compiler) < 0) {
        fprintf(stderr, "Compilation failed due to errors\n");
        return;
    }
    
    // Add halt instruction
    add_instruction(&compiler, INSTR_HLT, -1);
    
    // Generate final assembly code
    generate_data_section(&compiler, output);
    fprintf(output, ".CODE\n");
    generate_assembly_code(&compiler, output);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }
    
    FILE *input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Error opening input file: %s\n", argv[1]);
        return 1;
    }
    
    char source_code[MAX_PROGRAM_SIZE] = {0};
    char buffer[MAX_LINE_SIZE];
    
    while (fgets(buffer, MAX_LINE_SIZE, input)) {
        strcat(source_code, buffer);
    }
    
    fclose(input);
    
    FILE *output = fopen(argv[2], "w");
    if (!output) {
        fprintf(stderr, "Error opening output file: %s\n", argv[2]);
        return 1;
    }
    
    compile(source_code, output);
    fclose(output);
    
    printf("Compilation completed successfully!\n");
    return 0;
}
