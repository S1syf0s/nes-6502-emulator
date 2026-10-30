#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

#define GRAY "\033[90m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"
#define CLEAR_PREVIOUS_LINE "\033[1A\033[2K\r"

static const char *current_test = NULL;

void testing(const char *name) {
    
    current_test = name;
    printf(GRAY "[" YELLOW "testing" GRAY "]" RESET " %s\n", name);
    fflush(stdout);
}

void test_ok(void) {
    
    printf(CLEAR_PREVIOUS_LINE GRAY "[" GREEN "ok" GRAY "]" RESET " %s\n",
           current_test);
}

void test_failed(void) {
    
    printf(CLEAR_PREVIOUS_LINE GRAY "[" RED "failed" GRAY "]" RESET " %s\n",
           current_test);
}

void test_assert_impl(char *condition, int result) {
    
    if (result) return;

    test_failed();
    printf("Wrong answer: %s\n", condition);
    exit(0);
}

#define test_assert(condition) test_assert_impl(#condition, condition)

void check_file(char *filename) {
   
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("File %s not found, check the instructions\n", filename);
        exit(1);
    }
    fclose(file);
}

void test(void) {
    
    check_file("nestest.nes");
    check_file("nestest.log");

    CPU *cpu = cpu_init("nestest.nes");
    FILE *log = fopen("nestest.log", "r");

    char line[100];
    int count = 0;
    
    for (int i = 0; i < 5003; i++) {

        count++;
        fgets(line, 100, log);

        int log_a, log_x, log_y, log_pc, log_sp, log_flags;
        sscanf(line, "%x", &log_pc);
        sscanf(line + 50, "%x", &log_a);
        sscanf(line + 55, "%x", &log_x);
        sscanf(line + 60, "%x", &log_y);
        sscanf(line + 65, "%x", &log_flags);
        sscanf(line + 71, "%x", &log_sp);

        int cpu_a, cpu_x, cpu_y, cpu_pc, cpu_sp, cpu_flags;
        cpu_status(cpu, &cpu_a, &cpu_x, &cpu_y, &cpu_pc, &cpu_sp, &cpu_flags);

        int error = 0;

        if (log_a != cpu_a) error = 1;
        if (log_x != cpu_x) error = 1;
        if (log_y != cpu_y) error = 1;
        if (log_pc != cpu_pc) error = 1;
        if (log_sp != cpu_sp) error = 1;
        if (log_flags != cpu_flags) error = 1;

        if (error) {
            printf("Error in emulation (step %d)\n", count);
            printf("Result:   PC=%04X, A=%02X, X=%02X, Y=%02X, SP=%02X, P=%02X\n",
                   cpu_pc, cpu_a, cpu_x, cpu_y, cpu_sp, cpu_flags);
            printf("Expected: PC=%04X, A=%02X, X=%02X, Y=%02X, SP=%02X, P=%02X\n",
                   log_pc, log_a, log_x, log_y, log_sp, log_flags);
            exit(1);
        }

        cpu_step(cpu);
    }

    cpu_free(cpu);
}

int main(void) {
    
    testing("nestest.nes");
    test();
    test_ok();

    return 0;
}
