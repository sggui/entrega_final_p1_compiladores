#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Capacidade da memória do Neander
#define MEMORY_CAPACITY 256

/**
 * Converte um valor para inteiro a partir de uma string
 */
int string_to_number(const char* text) {
    // Lida com prefixo hexadecimal explícito
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        return strtol(text + 2, NULL, 16);
    }
    
    // Caso contrário, trata como decimal
    return atoi(text);
}

/**
 * Programa principal para conversão de arquivo .asm para arquivo binário Neander
 */
int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Uso: %s <input.asm> <output.mem>\n", argv[0]);
        return 1;
    }

    // Abre arquivo de entrada
    FILE* input_file = fopen(argv[1], "r");
    if (!input_file) {
        printf("Erro: Não foi possível abrir o arquivo de entrada %s\n", argv[1]);
        return 1;
    }

    // Inicializa a memória com zeros
    unsigned short memory[MEMORY_CAPACITY] = {0};
    int highest_address = 0;
    int code_position = 0;
    int data_section = 0;
    
    // Processa o arquivo linha por linha
    char line[256];
    while (fgets(line, sizeof(line), input_file)) {
        // Remove quebra de linha
        line[strcspn(line, "\n")] = 0;
        
        // Ignora linhas vazias
        if (strlen(line) == 0) {
            continue;
        }
        
        // Verifica seções
        if (strcmp(line, ".DATA") == 0) {
            data_section = 1;
            continue;
        } else if (strcmp(line, ".CODE") == 0) {
            data_section = 0;
            code_position = 0;
            continue;
        }
        
        // Processamento da seção de dados
        if (data_section) {
            // Formato esperado: 0xAA 0xBB (endereço valor)
            char addr_str[32] = {0};
            char value_str[32] = {0};
            
            if (sscanf(line, "%s %s", addr_str, value_str) == 2) {
                int addr = string_to_number(addr_str);
                int value = string_to_number(value_str);
                
                memory[addr] = value;
                
                // Atualiza o endereço mais alto
                if (addr > highest_address) {
                    highest_address = addr;
                }
            }
        } 
        // Processamento da seção de código
        else {
            char opcode[16] = {0};
            char operand[16] = {0};
            
            int items = sscanf(line, "%s %s", opcode, operand);
            
            if (items >= 1) {
                unsigned short op_value = 0;
                
                // Mapeia mnemônicos para valores de opcode
                if (strcmp(opcode, "NOP") == 0) {
                    op_value = 0x0000;
                } else if (strcmp(opcode, "STA") == 0) {
                    op_value = 0x0010;
                } else if (strcmp(opcode, "LDA") == 0) {
                    op_value = 0x0020;
                } else if (strcmp(opcode, "ADD") == 0) {
                    op_value = 0x0030;
                } else if (strcmp(opcode, "OR") == 0) {
                    op_value = 0x0040;
                } else if (strcmp(opcode, "AND") == 0) {
                    op_value = 0x0050;
                } else if (strcmp(opcode, "NOT") == 0) {
                    op_value = 0x0060;
                } else if (strcmp(opcode, "JMP") == 0) {
                    op_value = 0x0080;
                } else if (strcmp(opcode, "JN") == 0) {
                    op_value = 0x0090;
                } else if (strcmp(opcode, "JZ") == 0) {
                    op_value = 0x00A0;
                } else if (strcmp(opcode, "HLT") == 0) {
                    op_value = 0x00F0;
                } else {
                    printf("Aviso: Mnemônico desconhecido: %s\n", opcode);
                }
                
                // Armazena o opcode
                memory[code_position] = op_value;
                
                // Se a instrução tem operando, armazena na posição seguinte
                if (items == 2 && strcmp(opcode, "NOP") != 0 && strcmp(opcode, "HLT") != 0) {
                    int operand_value = string_to_number(operand);
                    memory[code_position + 1] = operand_value;
                    code_position++;
                }
                
                code_position++;
                
                // Atualiza o endereço mais alto
                if (code_position > highest_address) {
                    highest_address = code_position;
                }
            }
        }
    }
    
    fclose(input_file);
    
    // Cria cópia temporária da memória
    unsigned short temp_memory[MEMORY_CAPACITY] = {0};
    for (int i = 0; i <= highest_address; i++) {
        temp_memory[i] = memory[i];
    }

    // Adiciona cabeçalho de identificação do Neander
    memory[0] = 0x4e03;  // 'N' e '.'
    memory[1] = 0x5244;  // 'R' e 'D'

    // Copia a memória original após o cabeçalho
    for (int i = 0; i <= highest_address; i++) {
        memory[i + 2] = temp_memory[i];
    }

    // Ajusta o endereço máximo
    highest_address += 2;
    
    // Abre arquivo para escrita binária
    FILE* output_file = fopen(argv[2], "wb");
    if (!output_file) {
        printf("Erro: Não foi possível criar o arquivo de saída %s\n", argv[2]);
        return 1;
    }
    
    // Grava o arquivo binário
    fwrite(memory, sizeof(unsigned short), highest_address + 1, output_file);
    fclose(output_file);
    
    printf("Conversão concluída com sucesso!\n");
    printf("Arquivo binário Neander gerado: %s\n", argv[2]);
    
    return 0;
}
