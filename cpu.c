#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "cpu.h"

struct CPU {

    uint8_t *memory;
    
    uint8_t register_a;
    uint8_t register_x;
    uint8_t register_y;
   
    uint16_t program_counter;
    uint8_t stack_pointer;
    uint8_t status_flags; 

};

CPU* cpu_init(char *filename) {
    
    CPU *cpu = malloc((sizeof(CPU)));
    
    cpu->memory = malloc(0x10000);
    
    cpu->register_a = 0x00;
    cpu->register_x = 0x00;
    cpu->register_y = 0x00;


    cpu->program_counter = 0xc000;
    cpu->stack_pointer = 0xfd;
    cpu->status_flags = 0x24;
    
    FILE *file = fopen(filename, "rb");
    
    if (!file) {
        return NULL; 
    }

    fseek(file, 0, SEEK_END);
    fseek(file, 16, SEEK_SET);
    
    fread(cpu->memory + 0xc000, 1, 16384, file);
    
    fclose(file);

    return cpu;
}

uint16_t read_u16(uint8_t low, uint8_t high) {

    return (low | ((uint16_t)high << 8));

}

uint8_t clear_bit(uint8_t byte, uint8_t i) {
    
        byte = byte | (1 << i);
        return byte ^ (1 << i);

}


uint8_t set_bit(uint8_t byte, uint8_t i) {

        return byte | (1 << i);

}

uint8_t check_negative(uint8_t byte) {

    if ((byte >> 7) == 1) {
        return 1;    
    }

    else {
        return 0;
    }
}

void ldx_check_flags(CPU *cpu) {
    
    cpu->status_flags &= ~(1 << 1);
    cpu->status_flags &= ~(1 << 7);

    if (cpu->register_x == 0) {
        cpu->status_flags = set_bit(cpu->status_flags, 1);
    }

    if (check_negative(cpu->register_x)) {
        cpu->status_flags = set_bit(cpu->status_flags, 7);
    }
}


void lda_check_flags(CPU *cpu) {

    cpu->status_flags &= ~(1 << 1);
    cpu->status_flags &= ~(1 << 7);
    
    if (cpu->register_a == 0) {
        cpu->status_flags = set_bit(cpu->status_flags, 1);
    }

    if (check_negative(cpu->register_a)) {
        cpu->status_flags = set_bit(cpu->status_flags, 7);
    }
}

void ldy_check_flags(CPU *cpu) {

    cpu->status_flags &= ~(1 << 1);
    cpu->status_flags &= ~(1 << 7);
    
    if (cpu->register_y == 0) {
        cpu->status_flags = set_bit(cpu->status_flags, 1);
    }

    if (check_negative(cpu->register_y)) {
        cpu->status_flags = set_bit(cpu->status_flags, 7);
    }
}

void transfer_zero_negative(CPU *cpu, uint8_t value) {

    cpu->status_flags &= ~(1 << 1);
    cpu->status_flags &= ~(1 << 7);
    
    if (value == 0) {
        cpu->status_flags = set_bit(cpu->status_flags, 1);
    }
    if (check_negative(value)) {
        cpu->status_flags = set_bit(cpu->status_flags, 7);
    }
}

uint8_t add_with_carry(CPU *cpu, uint8_t register_value, uint8_t memory_value) {

    uint8_t carry = cpu->status_flags & 1;
    uint16_t sum = register_value + memory_value + carry;
    uint8_t result = (uint8_t)sum;

    cpu->status_flags &= ~(1 << 1);
    cpu->status_flags &= ~(1 << 7);
    cpu->status_flags &= ~(1 << 0);
    cpu->status_flags &= ~(1 << 6);

    if (result == 0) {
        cpu->status_flags = set_bit(cpu->status_flags, 1);
    }

    if (check_negative(result)) {
        cpu->status_flags = set_bit(cpu->status_flags, 7);
    }
    
    if ((sum >> 8) & 1) {
        cpu->status_flags = set_bit(cpu->status_flags, 0);
    }

    if (((result ^ register_value) & (result ^ memory_value)) & 0x80) {
        cpu->status_flags = set_bit(cpu->status_flags, 6);
    }
    
    return result;
}

uint8_t subtract_with_carry(CPU *cpu, uint8_t register_value, uint8_t memory_value) {

    uint8_t carry = cpu->status_flags & 1;
    uint8_t inverted_memory = (uint8_t)(~memory_value);
    uint16_t sum = register_value + inverted_memory + carry;
    uint8_t result = (uint8_t)sum;

    cpu->status_flags &= ~(1 << 1);
    cpu->status_flags &= ~(1 << 7);
    cpu->status_flags &= ~(1 << 0);
    cpu->status_flags &= ~(1 << 6);

    if (result == 0) {
        cpu->status_flags = set_bit(cpu->status_flags, 1);
    }
    if (check_negative(result)) {
        cpu->status_flags = set_bit(cpu->status_flags, 7);
    }
    
    if ((sum >> 8) & 1) {
        cpu->status_flags = set_bit(cpu->status_flags, 0);
    }

    if (((result ^ register_value) & (result ^ inverted_memory)) & 0x80) {
        cpu->status_flags = set_bit(cpu->status_flags, 6);
    }
   
    return result;
}

void compare_values(CPU *cpu, uint8_t reg, uint8_t mem) {
   
    cpu->status_flags &= ~(1 << 1);
    cpu->status_flags &= ~(1 << 7);
    cpu->status_flags &= ~(1 << 0);

    if (reg >= mem) {
        cpu->status_flags = set_bit(cpu->status_flags, 0);
    }
    if (reg == mem) {
        cpu->status_flags = set_bit(cpu->status_flags, 1);
    }
    if (check_negative(reg - mem)) {
        cpu->status_flags = set_bit(cpu->status_flags, 7);
    }
}

void bit_test(CPU *cpu, uint8_t mem) {

    cpu->status_flags &= ~(1 << 1);
    cpu->status_flags &= ~(1 << 6);
    cpu->status_flags &= ~(1 << 7);

    if ((cpu->register_a & mem) == 0) {
        cpu->status_flags = set_bit(cpu->status_flags, 1);
    }
    if ((mem >> 6) & 1) {
        cpu->status_flags = set_bit(cpu->status_flags, 6);
    }
    if (check_negative(mem)) {
        cpu->status_flags = set_bit(cpu->status_flags, 7);
    }
}

void push_stack(CPU *cpu, uint8_t value) {
   
    cpu->memory[0x100 + cpu->stack_pointer] = value;
    cpu->stack_pointer--;
}

uint8_t pull_stack(CPU *cpu) {
  
    cpu->stack_pointer++;
    return cpu->memory[0x100 + cpu->stack_pointer];
}

uint8_t asl_value(CPU *cpu, uint8_t value) {

    cpu->status_flags &= ~(1 << 0);
   
    if ((value >> 7) & 1) {
        cpu->status_flags = set_bit(cpu->status_flags, 0);
    }
    uint8_t result = value << 1;
    transfer_zero_negative(cpu, result);
    return result;
}

uint8_t lsr_value(CPU *cpu, uint8_t value) {
   
    cpu->status_flags &= ~(1 << 0);
   
    if (value & 1) {
        cpu->status_flags = set_bit(cpu->status_flags, 0);
    }
    uint8_t result = value >> 1;
    transfer_zero_negative(cpu, result);
    return result;
}

uint8_t rol_value(CPU *cpu, uint8_t value) {
   
    uint8_t carry = cpu->status_flags & 1;
   
    cpu->status_flags &= ~(1 << 0);
   
    if ((value >> 7) & 1) {
        cpu->status_flags = set_bit(cpu->status_flags, 0);
    }
   
    uint8_t result = (value << 1) | carry;
    transfer_zero_negative(cpu, result);
    return result;
}

uint8_t ror_value(CPU *cpu, uint8_t value) {
   
    uint8_t carry = cpu->status_flags & 1;
   
    cpu->status_flags &= ~(1 << 0);
   
    if (value & 1) {
        cpu->status_flags = set_bit(cpu->status_flags, 0);
    }
   
    uint8_t result = (value >> 1) | (carry << 7);
    transfer_zero_negative(cpu, result);
    return result;
}

void cpu_step(CPU *cpu) {

    uint8_t opcode = cpu->memory[cpu->program_counter];
    
    uint8_t next1 = cpu->memory[cpu->program_counter + 1];
    uint8_t next2 = cpu->memory[cpu->program_counter + 2];

    switch (opcode) {
    
        case (0xA9):
            cpu->register_a = cpu->memory[cpu->program_counter + 1];
            cpu->program_counter += 2;
            lda_check_flags(cpu);
            return;

        case (0xA5):
            cpu->register_a = cpu->memory[cpu->memory[cpu->program_counter + 1]];
            cpu->program_counter += 2;
            lda_check_flags(cpu); 
            return;

        case (0xB5): {
            uint8_t address = cpu->memory[cpu->program_counter + 1] + cpu->register_x; 
            cpu->register_a = cpu->memory[address];
            cpu->program_counter += 2;
            lda_check_flags(cpu);
            return;
        }

        case (0xAD): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]);
            cpu->register_a = cpu->memory[address];
            cpu->program_counter += 3;
            lda_check_flags(cpu);
            return;
        }

        case (0xBD): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]);
            address += cpu->register_x;
            cpu->register_a = cpu->memory[address];
            cpu->program_counter += 3;
            lda_check_flags(cpu);
            return;
        }

        case (0xB9): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]);
            address += cpu->register_y;
            cpu->register_a = cpu->memory[address];
            cpu->program_counter += 3;
            lda_check_flags(cpu);
            return;
        }

        case (0xA1): {
            uint8_t address = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            uint8_t low = cpu->memory[address];
            uint8_t high = cpu->memory[(uint8_t)(address + 1)];
            uint16_t new_address = read_u16(low, high);
            cpu->register_a = cpu->memory[new_address];
            cpu->program_counter += 2;
            lda_check_flags(cpu);
            return;
        }

        case (0xB1): {
            uint8_t address = cpu->memory[cpu->program_counter + 1];
            uint8_t low = cpu->memory[address];
            uint8_t high = cpu->memory[(uint8_t)(address + 1)];
            uint16_t new_address = read_u16(low, high);
            new_address += cpu->register_y;
            cpu->register_a = cpu->memory[new_address];
            cpu->program_counter += 2;
            lda_check_flags(cpu);
            return;
        }

        case (0x85):
            cpu->memory[cpu->memory[cpu->program_counter + 1]] = cpu->register_a;
            cpu->program_counter += 2;
            return;

        case (0x95): {
            uint8_t address = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            cpu->memory[address] = cpu->register_a;
            cpu->program_counter += 2;
            return;
        }
        
        case (0x8D): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high);
            cpu->memory[address] = cpu->register_a;
            cpu->program_counter += 3;
            return;
        }
        
        case (0x9D): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high) + cpu->register_x;
            cpu->memory[address] = cpu->register_a;
            cpu->program_counter += 3;
            return;
        }

        case (0x99): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high) + cpu->register_y;
            cpu->memory[address] = cpu->register_a;
            cpu->program_counter += 3;
            return;
        }
        
        case (0x81): {
            uint8_t address = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            uint8_t low = cpu->memory[address];
            uint8_t high = cpu->memory[(uint8_t)(address + 1)];
            uint16_t address_a = read_u16(low, high);
            cpu->memory[address_a] = cpu->register_a;
            cpu->program_counter += 2;
            return;
        }
        
        case (0x91): {
            uint8_t address = cpu->memory[cpu->program_counter + 1];
            uint8_t low = cpu->memory[address];
            uint8_t high = cpu->memory[(uint8_t)(address + 1)];
            uint16_t new_address = read_u16(low, high);
            new_address += cpu->register_y;
            cpu->memory[new_address] = cpu->register_a;
            cpu->program_counter += 2;
            return;
        }
        case (0xA2):
            cpu->register_x = next1;
            cpu->program_counter += 2;
            ldx_check_flags(cpu);
            return;
        
        case (0xA6):
            cpu->register_x = cpu->memory[next1];
            cpu->program_counter += 2;
            ldx_check_flags(cpu); 
            return;

        case (0xB6): {
            uint8_t address = cpu->memory[cpu->program_counter + 1] + cpu->register_y;
            cpu->register_x = cpu->memory[address];
            cpu->program_counter += 2;
            ldx_check_flags(cpu);
            return;
        }

        case (0xAE): {
            uint16_t address = read_u16(next1, next2);
            cpu->register_x = cpu->memory[address];
            cpu->program_counter += 3;
            ldx_check_flags(cpu);
            return;
        }

        case (0xBE): {
            uint16_t address = read_u16(next1, next2);
            cpu->register_x = cpu->memory[address + cpu->register_y];
            cpu->program_counter += 3;
            ldx_check_flags(cpu);
            return;
        }

        case (0x86): 
            cpu->memory[cpu->memory[cpu->program_counter + 1]] = cpu->register_x;
            cpu->program_counter += 2;
            return;

        case (0x96): {
            uint8_t address = cpu->memory[cpu->program_counter + 1];
            address += cpu->register_y;
            cpu->memory[address] = cpu->register_x;
            cpu->program_counter += 2;
            return;
        }

        case (0x8E): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high);
            cpu->memory[address] = cpu->register_x;
            cpu->program_counter += 3;
            return;
        }

        case (0xA0): {
            cpu->register_y = cpu->memory[cpu->program_counter + 1];
            cpu->program_counter += 2;
            ldy_check_flags(cpu);
            return;
        }

        case (0xA4): {
            cpu->register_y = cpu->memory[cpu->memory[cpu->program_counter + 1]];
            cpu->program_counter += 2;
            ldy_check_flags(cpu);
            return;
        }

        case (0xB4): {
            uint8_t address = cpu->memory[cpu->program_counter + 1];
            address += cpu->register_x;
            cpu->register_y = cpu->memory[address];
            cpu->program_counter += 2;
            ldy_check_flags(cpu);
            return;
        }
        case (0xAC): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high);
            cpu->register_y = cpu->memory[address];
            cpu->program_counter += 3;
            ldy_check_flags(cpu);
            return;
        }
        case (0xBC): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high);
            address += cpu->register_x;
            cpu->register_y = cpu->memory[address];
            cpu->program_counter += 3;
            ldy_check_flags(cpu);
            return;
        }
        case (0x84): 
            cpu->memory[cpu->memory[cpu->program_counter + 1]] = cpu->register_y;
            cpu->program_counter += 2;
            return;

        case (0x94): {
            uint8_t address = cpu->memory[cpu->program_counter + 1];
            address += cpu->register_x;
            cpu->memory[address] = cpu->register_y;
            cpu->program_counter += 2;
            return;
        }

        case (0x8C): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high);
            cpu->memory[address] = cpu->register_y;
            cpu->program_counter += 3;
            return;
        }

        case (0xAA):
            cpu->register_x = cpu->register_a;
            transfer_zero_negative(cpu, cpu->register_x);
            cpu->program_counter++;
            return;

        case (0x8A):
            cpu->register_a = cpu->register_x;
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter++;
            return;

        case (0xA8):
            cpu->register_y = cpu->register_a;
            transfer_zero_negative(cpu, cpu->register_y);
            cpu->program_counter++;
            return;

        case (0x98):
            cpu->register_a = cpu->register_y;
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter++;
            return; 
        case (0x69): {
            uint8_t memory_value = cpu->memory[cpu->program_counter + 1];
            cpu->register_a = add_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }

        case (0x65): {
            uint8_t memory_value = cpu->memory[cpu->memory[cpu->program_counter + 1]];
            cpu->register_a = add_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }

        case (0x75): {
            uint8_t address = cpu->memory[cpu->program_counter + 1];
            address += cpu->register_x;
            uint8_t memory_value = cpu->memory[address];
            cpu->register_a = add_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }

        case (0x6D): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high);
            uint8_t memory_value = cpu->memory[address];
            cpu->register_a = add_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 3;
            return;
        }

        case (0x7D): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high);
            address += cpu->register_x;
            uint8_t memory_value = cpu->memory[address];
            cpu->register_a = add_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 3;
            return;
        }
        
        case (0x79): {
            uint8_t low = cpu->memory[cpu->program_counter + 1];
            uint8_t high = cpu->memory[cpu->program_counter + 2];
            uint16_t address = read_u16(low, high);
            address += cpu->register_y;
            uint8_t memory_value = cpu->memory[address];
            cpu->register_a = add_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 3;
            return;
        }

        case (0x61): {
            uint8_t low_adress = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            uint8_t low = cpu->memory[low_adress];
            low_adress++;
            uint8_t high = cpu->memory[low_adress];
            uint16_t address = read_u16(low, high);
            uint8_t memory_value = cpu->memory[address]; 
            cpu->register_a = add_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }

        case (0x71): {
            uint8_t low_adress = cpu->memory[cpu->program_counter + 1];
            uint8_t low = cpu->memory[low_adress];
            low_adress++;
            uint8_t high = cpu->memory[low_adress];
            uint16_t address = read_u16(low, high);
            address += cpu->register_y;
            uint8_t memory_value = cpu->memory[address]; 
            cpu->register_a = add_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }

        case (0xE9): {
            uint8_t memory_value = cpu->memory[cpu->program_counter + 1];
            cpu->register_a = subtract_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }
        
        case (0xE5): {
            uint8_t memory_value = cpu->memory[cpu->memory[cpu->program_counter + 1]];
            cpu->register_a = subtract_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }
        
        case (0xF5): {
            uint8_t address = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            uint8_t memory_value = cpu->memory[address];
            cpu->register_a = subtract_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }
        case (0xED): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]);
            uint8_t memory_value = cpu->memory[address];
            cpu->register_a = subtract_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 3;
            return;
        }
        case (0xFD): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]) + cpu->register_x;
            uint8_t memory_value = cpu->memory[address];
            cpu->register_a = subtract_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 3;
            return;
        }
        case (0xF9): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]) + cpu->register_y;
            uint8_t memory_value = cpu->memory[address];
            cpu->register_a = subtract_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 3;
            return;
        }
        case (0xE1): {
            uint8_t low_adress = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            uint8_t low = cpu->memory[low_adress];
            low_adress++;
            uint8_t high = cpu->memory[low_adress];
            uint16_t address = read_u16(low, high);
            uint8_t memory_value = cpu->memory[address]; 
            cpu->register_a = subtract_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }
        case (0xF1): {
            uint8_t low_adress = cpu->memory[cpu->program_counter + 1];
            uint8_t low = cpu->memory[low_adress];
            low_adress++;
            uint8_t high = cpu->memory[low_adress];
            uint16_t address = read_u16(low, high);
            address += cpu->register_y;
            uint8_t memory_value = cpu->memory[address]; 
            cpu->register_a = subtract_with_carry(cpu, cpu->register_a, memory_value);
            cpu->program_counter += 2;
            return;
        }

        case (0xE6): {
            uint8_t address = cpu->memory[cpu->program_counter + 1];
            cpu->memory[address]++;
            transfer_zero_negative(cpu, cpu->memory[address]);
            cpu->program_counter += 2;
            return;
        }
        case (0xF6): {
            uint8_t address = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            cpu->memory[address]++;
            transfer_zero_negative(cpu, cpu->memory[address]);
            cpu->program_counter += 2;
            return;
        }
        case (0xEE): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]);
            cpu->memory[address]++;
            transfer_zero_negative(cpu, cpu->memory[address]);
            cpu->program_counter += 3;
            return;
        }
        case (0xFE): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]) + cpu->register_x;
            cpu->memory[address]++;
            transfer_zero_negative(cpu, cpu->memory[address]);
            cpu->program_counter += 3;
            return;
        }

        case (0xE8):
            cpu->register_x++;
            transfer_zero_negative(cpu, cpu->register_x);
            cpu->program_counter++;
            return;
        case (0xC8):
            cpu->register_y++;
            transfer_zero_negative(cpu, cpu->register_y);
            cpu->program_counter++;
            return;
        case (0xCA):
            cpu->register_x--;
            transfer_zero_negative(cpu, cpu->register_x);
            cpu->program_counter++;
            return;
        case (0x88):
            cpu->register_y--;
            transfer_zero_negative(cpu, cpu->register_y);
            cpu->program_counter++;
            return;

        case (0xC6): {
            uint8_t address = cpu->memory[cpu->program_counter + 1];
            cpu->memory[address]--;
            transfer_zero_negative(cpu, cpu->memory[address]);
            cpu->program_counter += 2;
            return;
        }
        case (0xD6): {
            uint8_t address = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            cpu->memory[address]--;
            transfer_zero_negative(cpu, cpu->memory[address]);
            cpu->program_counter += 2;
            return;
        }
        case (0xCE): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]);
            cpu->memory[address]--;
            transfer_zero_negative(cpu, cpu->memory[address]);
            cpu->program_counter += 3;
            return;
        }
        case (0xDE): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]) + cpu->register_x;
            cpu->memory[address]--;
            transfer_zero_negative(cpu, cpu->memory[address]);
            cpu->program_counter += 3;
            return;
        }

        case (0x0A):
            cpu->register_a = asl_value(cpu, cpu->register_a);
            cpu->program_counter++;
            return;
        case (0x06): {
            uint8_t addr = cpu->memory[cpu->program_counter + 1];
            cpu->memory[addr] = asl_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 2;
            return;
        }
        case (0x16): {
            uint8_t addr = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            cpu->memory[addr] = asl_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 2;
            return;
        }
        case (0x0E): {
            uint16_t addr = read_u16(next1, next2);
            cpu->memory[addr] = asl_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }
        case (0x1E): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_x;
            cpu->memory[addr] = asl_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }

        case (0x4A):
            cpu->register_a = lsr_value(cpu, cpu->register_a);
            cpu->program_counter++;
            return;
        case (0x46): {
            uint8_t addr = cpu->memory[cpu->program_counter + 1];
            cpu->memory[addr] = lsr_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 2;
            return;
        }
        case (0x56): {
            uint8_t addr = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            cpu->memory[addr] = lsr_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 2;
            return;
        }
        case (0x4E): {
            uint16_t addr = read_u16(next1, next2);
            cpu->memory[addr] = lsr_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }
        case (0x5E): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_x;
            cpu->memory[addr] = lsr_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }

        case (0x2A):
            cpu->register_a = rol_value(cpu, cpu->register_a);
            cpu->program_counter++;
            return;
        case (0x26): {
            uint8_t addr = cpu->memory[cpu->program_counter + 1];
            cpu->memory[addr] = rol_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 2;
            return;
        }
        case (0x36): {
            uint8_t addr = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            cpu->memory[addr] = rol_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 2;
            return;
        }
        case (0x2E): {
            uint16_t addr = read_u16(next1, next2);
            cpu->memory[addr] = rol_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }
        case (0x3E): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_x;
            cpu->memory[addr] = rol_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }

        case (0x6A):
            cpu->register_a = ror_value(cpu, cpu->register_a);
            cpu->program_counter++;
            return;
        case (0x66): {
            uint8_t addr = cpu->memory[cpu->program_counter + 1];
            cpu->memory[addr] = ror_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 2;
            return;
        }
        case (0x76): {
            uint8_t addr = cpu->memory[cpu->program_counter + 1] + cpu->register_x;
            cpu->memory[addr] = ror_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 2;
            return;
        }
        case (0x6E): {
            uint16_t addr = read_u16(next1, next2);
            cpu->memory[addr] = ror_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }
        case (0x7E): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_x;
            cpu->memory[addr] = ror_value(cpu, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }
   
        case (0x29):
            cpu->register_a &= next1;
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        case (0x25):
            cpu->register_a &= cpu->memory[next1];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        case (0x35):
            cpu->register_a &= cpu->memory[(uint8_t)(next1 + cpu->register_x)];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        case (0x2D): {
            uint16_t addr = read_u16(next1, next2);
            cpu->register_a &= cpu->memory[addr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 3;
            return;
        }
        case (0x3D): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_x;
            cpu->register_a &= cpu->memory[addr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 3;
            return;
        }
        case (0x39): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_y;
            cpu->register_a &= cpu->memory[addr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 3;
            return;
        }
        case (0x21): {
            uint8_t addr = next1 + cpu->register_x;
            uint16_t ptr = read_u16(cpu->memory[addr], cpu->memory[(uint8_t)(addr + 1)]);
            cpu->register_a &= cpu->memory[ptr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        }
        case (0x31): {
            uint8_t addr = next1;
            uint16_t ptr = read_u16(cpu->memory[addr], cpu->memory[(uint8_t)(addr + 1)]) + cpu->register_y;
            cpu->register_a &= cpu->memory[ptr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        }

        case (0x09):
            cpu->register_a |= next1;
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        case (0x05):
            cpu->register_a |= cpu->memory[next1];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        case (0x15):
            cpu->register_a |= cpu->memory[(uint8_t)(next1 + cpu->register_x)];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        case (0x0D): {
            uint16_t addr = read_u16(next1, next2);
            cpu->register_a |= cpu->memory[addr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 3;
            return;
        }
        case (0x1D): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_x;
            cpu->register_a |= cpu->memory[addr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 3;
            return;
        }
        case (0x19): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_y;
            cpu->register_a |= cpu->memory[addr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 3;
            return;
        }
        case (0x01): {
            uint8_t addr = next1 + cpu->register_x;
            uint16_t ptr = read_u16(cpu->memory[addr], cpu->memory[(uint8_t)(addr + 1)]);
            cpu->register_a |= cpu->memory[ptr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        }
        case (0x11): {
            uint8_t addr = next1;
            uint16_t ptr = read_u16(cpu->memory[addr], cpu->memory[(uint8_t)(addr + 1)]) + cpu->register_y;
            cpu->register_a |= cpu->memory[ptr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        }

        case (0x49):
            cpu->register_a ^= next1;
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        case (0x45):
            cpu->register_a ^= cpu->memory[next1];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        case (0x55):
            cpu->register_a ^= cpu->memory[(uint8_t)(next1 + cpu->register_x)];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        case (0x4D): {
            uint16_t addr = read_u16(next1, next2);
            cpu->register_a ^= cpu->memory[addr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 3;
            return;
        }
        case (0x5D): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_x;
            cpu->register_a ^= cpu->memory[addr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 3;
            return;
        }
        case (0x59): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_y;
            cpu->register_a ^= cpu->memory[addr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 3;
            return;
        }
        case (0x41): {
            uint8_t addr = next1 + cpu->register_x;
            uint16_t ptr = read_u16(cpu->memory[addr], cpu->memory[(uint8_t)(addr + 1)]);
            cpu->register_a ^= cpu->memory[ptr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        }
        case (0x51): {
            uint8_t addr = next1;
            uint16_t ptr = read_u16(cpu->memory[addr], cpu->memory[(uint8_t)(addr + 1)]) + cpu->register_y;
            cpu->register_a ^= cpu->memory[ptr];
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter += 2;
            return;
        }

        case (0x24): {
            uint8_t memory_value = cpu->memory[cpu->memory[cpu->program_counter + 1]];
            bit_test(cpu, memory_value);
            cpu->program_counter += 2;
            return;
        }
        case (0x2C): {
            uint16_t address = read_u16(cpu->memory[cpu->program_counter + 1], cpu->memory[cpu->program_counter + 2]);
            uint8_t memory_value = cpu->memory[address];
            bit_test(cpu, memory_value);
            cpu->program_counter += 3;
            return;
        }

        case (0xC9):
           
            compare_values(cpu, cpu->register_a, next1);
            cpu->program_counter += 2;
            return;
        
        case (0xC5):
            
            compare_values(cpu, cpu->register_a, cpu->memory[next1]);
            cpu->program_counter += 2;
            return;
        
        case (0xD5):
            compare_values(cpu, cpu->register_a, cpu->memory[(uint8_t)(next1 + cpu->register_x)]);
            cpu->program_counter += 2;
            return;
        
        case (0xCD): {
            uint16_t addr = read_u16(next1, next2);
            compare_values(cpu, cpu->register_a, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }
        
        case (0xDD): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_x;
            compare_values(cpu, cpu->register_a, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }
        
        case (0xD9): {
            uint16_t addr = read_u16(next1, next2) + cpu->register_y;
            compare_values(cpu, cpu->register_a, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }
        
        case (0xC1): {
            uint8_t addr = next1 + cpu->register_x;
            uint16_t ptr = read_u16(cpu->memory[addr], cpu->memory[(uint8_t)(addr + 1)]);
            compare_values(cpu, cpu->register_a, cpu->memory[ptr]);
            cpu->program_counter += 2;
            return;
        }
        
        case (0xD1): {
            uint8_t addr = next1;
            uint16_t ptr = read_u16(cpu->memory[addr], cpu->memory[(uint8_t)(addr + 1)]) + cpu->register_y;
            compare_values(cpu, cpu->register_a, cpu->memory[ptr]);
            cpu->program_counter += 2;
            return;
        }
        
        case (0xE0):
            compare_values(cpu, cpu->register_x, next1);
            cpu->program_counter += 2;
            return;
        case (0xE4):
            compare_values(cpu, cpu->register_x, cpu->memory[next1]);
            cpu->program_counter += 2;
            return;
        
        case (0xEC): {
            uint16_t addr = read_u16(next1, next2);
            compare_values(cpu, cpu->register_x, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }
        
        case (0xC0):
            compare_values(cpu, cpu->register_y, next1);
            cpu->program_counter += 2;
            return;
        case (0xC4):
            compare_values(cpu, cpu->register_y, cpu->memory[next1]);
            cpu->program_counter += 2;
            return;
        case (0xCC): {
            uint16_t addr = read_u16(next1, next2);
            compare_values(cpu, cpu->register_y, cpu->memory[addr]);
            cpu->program_counter += 3;
            return;
        }

        case (0x10): {
            if (((cpu->status_flags >> 7) & 1) == 0) {
                int8_t offset = (int8_t)next1;
                cpu->program_counter += 2 + offset;
            } else {
                cpu->program_counter += 2;
            }
            return;
        }
        case (0x30): {
            if (((cpu->status_flags >> 7) & 1) != 0) {
                int8_t offset = (int8_t)next1;
                cpu->program_counter += 2 + offset;
            } else {
                cpu->program_counter += 2;
            }
            return;
        }
        case (0x50): {
            if (((cpu->status_flags >> 6) & 1) == 0) {
                int8_t offset = (int8_t)next1;
                cpu->program_counter += 2 + offset;
            } else {
                cpu->program_counter += 2;
            }
            return;
        }
        case (0x70): {
            if (((cpu->status_flags >> 6) & 1) != 0) {
                int8_t offset = (int8_t)next1;
                cpu->program_counter += 2 + offset;
            } else {
                cpu->program_counter += 2;
            }
            return;
        }
        case (0x90): {
            if ((cpu->status_flags & 1) == 0) {
                int8_t offset = (int8_t)next1;
                cpu->program_counter += 2 + offset;
            } else {
                cpu->program_counter += 2;
            }
            return;
        }
        case (0xB0): {
            if ((cpu->status_flags & 1) != 0) {
                int8_t offset = (int8_t)next1;
                cpu->program_counter += 2 + offset;
            } else {
                cpu->program_counter += 2;
            }
            return;
        }
        case (0xD0): {
            
            if (((cpu->status_flags >> 1) & 1) == 0) {
                int8_t offset = (int8_t)next1;
                cpu->program_counter += 2 + offset;
            } 

            else {
                cpu->program_counter += 2;
            }

            return;
        }
        case (0xF0): {
            
            if (((cpu->status_flags >> 1) & 1) != 0) {
                int8_t offset = (int8_t)next1;
                cpu->program_counter += 2 + offset;
            } 
            
            else {
                cpu->program_counter += 2;
            }
            return;
        }
   
        case (0x4C):
            
            cpu->program_counter = read_u16(next1, next2);
            return; 
        
        case (0x6C): {
           
            uint16_t address = read_u16(next1, next2);
            uint8_t low = cpu->memory[address];
            uint8_t high;
           
            if ((address & 0x00FF) == 0x00FF) {
                high = cpu->memory[address - 0xFF];
            } 

            else {
                high = cpu->memory[address + 1];
            }
            
            cpu->program_counter = read_u16(low, high);
            return;
        }
   
        case (0x20): {
           
            uint16_t return_address = cpu->program_counter + 2;
            uint8_t high = (return_address >> 8);
            uint8_t low = (return_address);
           
            cpu->memory[0x100 + cpu->stack_pointer] = high;
            cpu->stack_pointer--;

            cpu->memory[0x100 + cpu->stack_pointer] = low;
            cpu->stack_pointer--;

            cpu->program_counter = read_u16(next1, next2);
            return;
        }
        
        case (0x60): {
            
            uint16_t return_address = 0x0000;
            cpu->stack_pointer++;
            return_address |= cpu->memory[0x100 + cpu->stack_pointer]; 
            cpu->stack_pointer++;
            return_address |= ((uint16_t)cpu->memory[0x100 + cpu->stack_pointer]) << 8;
            cpu->program_counter = return_address;
            cpu->program_counter++;
            return;
        }

        case (0x00): { 
           
            uint16_t return_address = cpu->program_counter + 2;
            uint8_t high = (return_address >> 8);
            uint8_t low = (return_address);
           
            cpu->memory[0x100 + cpu->stack_pointer] = high;
            cpu->stack_pointer--;

            cpu->memory[0x100 + cpu->stack_pointer] = low;
            cpu->stack_pointer--;
            
            cpu->memory[0x100 + cpu->stack_pointer] = set_bit(cpu->status_flags, 4);
            cpu->stack_pointer--;
            
            uint16_t new_address = 0x0000;
            new_address |= cpu->memory[0xFFFE];
            new_address |= ((uint16_t)cpu->memory[0xFFFF]) << 8;
            
            cpu->program_counter = new_address;
            cpu->status_flags = set_bit(cpu->status_flags, 2);
            return; 
        }

        case (0x40): {
            
            uint8_t flags;
            uint16_t return_address = 0x0000;
            
            cpu->stack_pointer++;
            flags = cpu->memory[0x100 + cpu->stack_pointer];
            cpu->status_flags = (flags & 0xCF) | 0x20;

            cpu->stack_pointer++;
            return_address |= cpu->memory[0x100 + cpu->stack_pointer]; 
            
            cpu->stack_pointer++;
            return_address |= ((uint16_t)cpu->memory[0x100 + cpu->stack_pointer]) << 8;

            cpu->program_counter = return_address;
            return;
        }
   
        
        case (0x9A):
       
            cpu->stack_pointer = cpu->register_x;
            cpu->program_counter++;
            return;
        
        case (0xBA):
       
            cpu->register_x = cpu->stack_pointer;
            transfer_zero_negative(cpu, cpu->register_x);
            cpu->program_counter++;
            return;
        
        case (0x48):
       
            push_stack(cpu, cpu->register_a);
            cpu->program_counter++;
            return;
        
        case (0x68):
       
            cpu->register_a = pull_stack(cpu);
            transfer_zero_negative(cpu, cpu->register_a);
            cpu->program_counter++;
            return;
        
        case (0x08):
       
            push_stack(cpu, cpu->status_flags | (1 << 4) | (1 << 5));
            cpu->program_counter++;
            return;
       
        case (0x28):
       
            cpu->status_flags = pull_stack(cpu) & 0xEF;
            cpu->status_flags = set_bit(cpu->status_flags, 5);
            cpu->program_counter++;
            return;
        
        case (0x18):
       
            cpu->status_flags &= ~(1 << 0);
            cpu->program_counter++;
            return;
        
        case (0x38):
       
            cpu->status_flags = set_bit(cpu->status_flags, 0);
            cpu->program_counter++;
            return;
        
        case (0x58):
       
            cpu->status_flags &= ~(1 << 2);
            cpu->program_counter++;
            return;
       
        case (0x78):
       
            cpu->status_flags = set_bit(cpu->status_flags, 2);
            cpu->program_counter++;
            return;
       
        case (0xB8):
       
            cpu->status_flags &= ~(1 << 6);
            cpu->program_counter++;
            return;
        
        case (0xD8):
           
            cpu->status_flags &= ~(1 << 3);
            cpu->program_counter++;
            return;
        
        case (0xF8):
          
            cpu->status_flags = set_bit(cpu->status_flags, 3);
            cpu->program_counter++;
            return;

        case (0xEA):

            cpu->program_counter++;
            return;
    }
}

void cpu_status(CPU *cpu, int *a, int *x, int *y, int *pc, int *sp, int *flags) {

    *a = cpu->register_a;
    *x = cpu->register_x;
    *y = cpu->register_y;
    *pc = cpu->program_counter;
    *sp = cpu->stack_pointer;
    *flags = cpu->status_flags;
}

void cpu_free(CPU *cpu) {

    free(cpu->memory);
    free(cpu);
}
