typedef struct CPU CPU;

CPU* cpu_init(char *filename);
void cpu_step(CPU *cpu);
void cpu_status(CPU *cpu, int *a, int *x, int *y, int *pc, int *sp, int *flags);
void cpu_free(CPU *cpu);
