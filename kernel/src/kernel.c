#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_COLOR 0x0F
#define INPUT_SIZE 128

static size_t cursor = 0;
static char input[INPUT_SIZE];
static size_t input_length = 0;
static uint8_t shift_down = 0;

static void put_char(char c)
{
    if (c == '\n') {
        cursor = ((cursor / VGA_WIDTH) + 1) * VGA_WIDTH;
    } else if (c == '\r') {
        cursor = (cursor / VGA_WIDTH) * VGA_WIDTH;
    } else if (c == '\b') {
        if (cursor > 0) {
            cursor--;
            VGA_MEMORY[cursor] = (uint16_t)((VGA_COLOR << 8) | ' ');
        }
    } else {
        VGA_MEMORY[cursor++] = (uint16_t)((VGA_COLOR << 8) | (uint8_t)c);
    }

    if (cursor >= VGA_WIDTH * VGA_HEIGHT) {
        for (size_t i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
            VGA_MEMORY[i] = VGA_MEMORY[i + VGA_WIDTH];
        }

        for (size_t i = VGA_WIDTH * (VGA_HEIGHT - 1);
             i < VGA_WIDTH * VGA_HEIGHT; i++) {
            VGA_MEMORY[i] = (uint16_t)((VGA_COLOR << 8) | ' ');
        }

        cursor = VGA_WIDTH * (VGA_HEIGHT - 1);
    }
}

static void print(const char *text)
{
    while (*text) {
        put_char(*text++);
    }
}

static void clear_screen(void)
{
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = (uint16_t)((VGA_COLOR << 8) | ' ');
    }
    cursor = 0;
}

static uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int keyboard_ready(void)
{
    return (inb(0x64) & 1) != 0;
}

static char scancode_to_ascii(uint8_t code)
{
    static const char normal[] = {
        0, 27, '1','2','3','4','5','6','7','8','9','0','-','=',
        '\b','\t','q','w','e','r','t','y','u','i','o','p','[',']',
        '\n', 0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
        0, '\\','z','x','c','v','b','n','m',',','.','/','*',0,' '
    };

    static const char shifted[] = {
        0, 27, '!','@','#','$','%','^','&','*','(',')','_','+',
        '\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}',
        '\n', 0, 'A','S','D','F','G','H','J','K','L',':','"','~',
        0, '|','Z','X','C','V','B','N','M','<','>','?','*',0,' '
    };

    if (code >= sizeof(normal)) {
        return 0;
    }

    return shift_down ? shifted[code] : normal[code];
}

static int equals(const char *a, const char *b)
{
    size_t i = 0;

    while (a[i] && b[i] && a[i] == b[i]) {
        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}


/* Kernel-owned blood inventory. Stock is held in RAM. */
static const char *blood_groups[] = {
    "A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"
};

static int blood_stock[] = {10, 10, 10, 10, 10, 10, 10, 5};

static int find_blood_group(const char *group)
{
    for (int i = 0; i < 8; i++) {
        if (equals(group, blood_groups[i])) {
            return i;
        }
    }
    return -1;
}

static void show_blood(void)
{
    print("\nBlood inventory (units):\n");
    for (int i = 0; i < 8; i++) {
        print("  ");
        print(blood_groups[i]);
        print(": ");
        if (blood_stock[i] < 10) {
            put_char(' ');
        }
        put_char((char)('0' + blood_stock[i] / 10));
        put_char((char)('0' + blood_stock[i] % 10));
        print("\n");
    }
}

static void handle_blood_request(const char *group, int units)
{
    int index = find_blood_group(group);

    if (index < 0) {
        print("\nInvalid blood group. Use A+, A-, B+, B-, AB+, AB-, O+, or O-.\n");
        return;
    }

    if (units <= 0 || units > 99) {
        print("\nUnits must be between 1 and 99.\n");
        return;
    }

    if (blood_stock[index] < units) {
        print("\nRequest rejected: insufficient stock. Available: ");
        put_char((char)('0' + blood_stock[index] / 10));
        put_char((char)('0' + blood_stock[index] % 10));
        print("\n");
        return;
    }

    blood_stock[index] -= units;
    print("\nRequest accepted. Remaining ");
    print(blood_groups[index]);
    print(" stock: ");
    put_char((char)('0' + blood_stock[index] / 10));
    put_char((char)('0' + blood_stock[index] % 10));
    print("\n");
}

static void parse_request(void)
{
    /* Expected format: request GROUP UNITS, e.g. request O+ 2 */
    char group[4];
    size_t i = 0;
    size_t pos = 7;

    while (input[pos] == ' ') pos++;

    while (input[pos] && input[pos] != ' ' && i < sizeof(group) - 1) {
        group[i++] = input[pos++];
    }
    group[i] = '\0';

    if (i == 0 || (input[pos] && input[pos] != ' ')) {
        print("\nUsage: request GROUP UNITS\n");
        return;
    }

    while (input[pos] == ' ') pos++;

    if (input[pos] < '0' || input[pos] > '9') {
        print("\nUsage: request GROUP UNITS\n");
        return;
    }

    int units = 0;
    while (input[pos] >= '0' && input[pos] <= '9') {
        units = units * 10 + (input[pos] - '0');
        pos++;
    }

    while (input[pos] == ' ') pos++;
    if (input[pos] != '\0') {
        print("\nUsage: request GROUP UNITS\n");
        return;
    }

    handle_blood_request(group, units);
}


#define MAX_PROCESSES 16
#define PROCESS_NAME_SIZE 24

typedef enum {
    PROC_READY,
    PROC_DONE
} ProcessState;

typedef struct {
    int pid;
    char name[PROCESS_NAME_SIZE];
    ProcessState state;
} KernelProcess;

static KernelProcess processes[MAX_PROCESSES];
static int process_count = 0;
static int next_pid = 1;

static const char *process_state_name(ProcessState state)
{
    return state == PROC_READY ? "READY" : "DONE";
}

static int add_process(const char *name)
{
    if (process_count >= MAX_PROCESSES) {
        print("\nProcess table full.\n");
        return 0;
    }

    KernelProcess *p = &processes[process_count];
    p->pid = next_pid++;
    p->state = PROC_READY;

    size_t i = 0;
    while (name[i] && i < PROCESS_NAME_SIZE - 1) {
        p->name[i] = name[i];
        i++;
    }
    p->name[i] = '\0';
    process_count++;

    print("\nProcess created: PID ");
    if (p->pid >= 10)
        put_char((char)('0' + p->pid / 10));
    put_char((char)('0' + p->pid % 10));
    print("\n");
    return 1;
}

static void show_processes(void)
{
    print("\nPID  NAME                    STATE\n");

    if (process_count == 0) {
        print("No processes registered.\n");
        return;
    }

    for (int i = 0; i < process_count; i++) {
        KernelProcess *p = &processes[i];

        if (p->pid >= 10)
            put_char((char)('0' + p->pid / 10));
        put_char((char)('0' + p->pid % 10));

        print("    ");
        print(p->name);

        size_t n = 0;
        while (p->name[n]) n++;
        while (n++ < PROCESS_NAME_SIZE - 1) put_char(' ');

        print(" ");
        print(process_state_name(p->state));
        print("\n");
    }
}

static void schedule_fcfs(void)
{
    int found = 0;
    print("\nFCFS scheduling order:\n");

    for (int i = 0; i < process_count; i++) {
        if (processes[i].state != PROC_READY) continue;

        print(" -> ");
        print(processes[i].name);
        processes[i].state = PROC_DONE;
        found = 1;
    }

    if (!found) print("No READY processes.");
    print("\nScheduling demonstration complete.\n");
}

static void parse_proc_add(void)
{
    size_t pos = 9; /* After "proc add " */
    char name[PROCESS_NAME_SIZE];
    size_t i = 0;

    while (input[pos] == ' ') pos++;

    while (input[pos] && input[pos] != ' ' &&
           i < PROCESS_NAME_SIZE - 1) {
        name[i++] = input[pos++];
    }

    if (i == 0 || input[pos] != '\0') {
        print("\nUsage: proc add NAME (one-word name)\n");
        return;
    }

    name[i] = '\0';
    add_process(name);
}


/* First-fit allocator demonstration using simulated memory blocks. */
#define MEMORY_BLOCK_COUNT 5

static const int memory_block_sizes[MEMORY_BLOCK_COUNT] = {
    100, 500, 200, 300, 600
};

static int memory_block_used[MEMORY_BLOCK_COUNT] = {0};

static void print_number(int value)
{
    char digits[10];
    int count = 0;

    if (value == 0) {
        put_char('0');
        return;
    }

    while (value > 0 && count < 10) {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    }

    while (count > 0) {
        put_char(digits[--count]);
    }
}

static void show_memory(void)
{
    print("\nFirst-Fit Memory Blocks\n");
    print("BLOCK   SIZE   STATUS\n");

    for (int i = 0; i < MEMORY_BLOCK_COUNT; i++) {
        print("  ");
        print_number(i + 1);
        print("      ");
        print_number(memory_block_sizes[i]);
        print("    ");
        print(memory_block_used[i] ? "ALLOCATED" : "FREE");
        print("\n");
    }
}

static void allocate_memory(int requested_size)
{
    if (requested_size <= 0) {
        print("\nSize must be greater than zero.\n");
        return;
    }

    for (int i = 0; i < MEMORY_BLOCK_COUNT; i++) {
        if (!memory_block_used[i] &&
            memory_block_sizes[i] >= requested_size) {
            memory_block_used[i] = 1;

            print("\nFirst-fit allocation successful.\n");
            print("Block: ");
            print_number(i + 1);
            print("\nBlock size: ");
            print_number(memory_block_sizes[i]);
            print("\nRequested size: ");
            print_number(requested_size);
            print("\nUnused space in block: ");
            print_number(memory_block_sizes[i] - requested_size);
            print("\n");
            return;
        }
    }

    print("\nAllocation failed: no free block is large enough.\n");
}

static void reset_memory(void)
{
    for (int i = 0; i < MEMORY_BLOCK_COUNT; i++) {
        memory_block_used[i] = 0;
    }

    print("\nMemory demonstration reset. All blocks are FREE.\n");
}

static void parse_memory_command(void)
{
    /* Expected: mem alloc SIZE */
    const char *prefix = "mem alloc ";
    (void)prefix;

    size_t pos = 10;

    if (input_length <= pos) {
        print("\nUsage: mem | mem alloc SIZE | mem reset\n");
        return;
    }

    int size = 0;

    while (input[pos] >= '0' && input[pos] <= '9') {
        size = size * 10 + input[pos] - '0';
        if (size > 100000) {
            print("\nRequested size is too large.\n");
            return;
        }
        pos++;
    }

    while (input[pos] == ' ') pos++;

    if (input[pos] != '\0' || size <= 0) {
        print("\nUsage: mem alloc SIZE\n");
        return;
    }

    allocate_memory(size);
}


/* FCFS disk scheduling demonstration using cylinder requests. */
#define DISK_REQUEST_COUNT 5

static const int disk_requests[DISK_REQUEST_COUNT] = {
    98, 183, 37, 122, 14
};

static void run_disk_fcfs(void)
{
    int head = 53;
    int total_movement = 0;

    print("\nFCFS Disk Scheduling\n");
    print("Initial head position: ");
    print_number(head);
    print("\nRequest order: ");

    for (int i = 0; i < DISK_REQUEST_COUNT; i++) {
        int target = disk_requests[i];
        int movement = target - head;

        if (movement < 0) movement = -movement;
        total_movement += movement;

        print_number(target);
        if (i < DISK_REQUEST_COUNT - 1) print(" -> ");

        head = target;
    }

    print("\nTotal head movement: ");
    print_number(total_movement);
    print(" cylinders\n");
}


static void show_help(void)
{
    print("\n");
    print("============================================================\n");
    print("               RAKTSETU MINI OS - COMMANDS\n");
    print("============================================================\n");
    print(" SYSTEM                 BLOOD BANK\n");
    print(" help   - Show help     blood - Show inventory\n");
    print(" clear  - Clear screen  request GROUP UNITS\n");
    print(" about  - About OS      - Request blood units\n");
    print(" status - Kernel status\n");
    print("------------------------------------------------------------\n");
    print(" PROCESS MANAGEMENT    MEMORY MANAGEMENT\n");
    print(" ps       - List procs  mem - Show memory blocks\n");
    print(" proc add NAME         mem alloc SIZE\n");
    print(" - Register process    - First-Fit allocation\n");
    print(" schedule - FCFS demo  mem reset - Reset memory\n");
    print("------------------------------------------------------------\n");
    print(" DISK SCHEDULING\n");
    print(" disk - Run FCFS disk scheduling\n");
    print("------------------------------------------------------------\n");
    print(" NOTE: Scheduling, memory and disk are simulations.\n");
    print("============================================================\n");
}

static void execute_command(void)
{
    input[input_length] = '\0';



    if (equals(input, "help")) {
        show_help();
    } else if (equals(input, "clear")) {
        clear_screen();
    } else if (equals(input, "about")) {
        print("\nRaktSetu Mini OS");
        print("\nA blood-bank resource management OS prototype.");
        print("\nRunning as a freestanding kernel in QEMU.\n");
    } else if (equals(input, "status")) {
        print("\nKernel: RUNNING");
        print("\nTerminal: ACTIVE");
        print("\nKeyboard: PS/2 polling enabled");
        print("\nBlood inventory: INITIALIZED");
        print("\nProcess scheduler: NOT INITIALIZED\n");
    } else if (equals(input, "mem")) {
        show_memory();
    } else if (equals(input, "mem reset")) {
        reset_memory();
    } else if (input_length > 10 &&
               input[0] == 'm' && input[1] == 'e' &&
               input[2] == 'm' && input[3] == ' ' &&
               input[4] == 'a' && input[5] == 'l' &&
               input[6] == 'l' && input[7] == 'o' &&
               input[8] == 'c' && input[9] == ' ') {
        parse_memory_command();
    } else if (equals(input, "disk")) {
        run_disk_fcfs();
    } else if (equals(input, "blood")) {
        show_blood();
    } else if (equals(input, "ps")) {
        show_processes();
    } else if (equals(input, "schedule")) {
        schedule_fcfs();
    } else if (input_length >= 9 &&
               input[0] == 'p' && input[1] == 'r' &&
               input[2] == 'o' && input[3] == 'c' &&
               input[4] == ' ' && input[5] == 'a' &&
               input[6] == 'd' && input[7] == 'd' &&
               input[8] == ' ') {
        parse_proc_add();
    } else if (input_length >= 8 &&
               input[0] == 'r' && input[1] == 'e' &&
               input[2] == 'q' && input[3] == 'u' &&
               input[4] == 'e' && input[5] == 's' &&
               input[6] == 't' && input[7] == ' ') {
        parse_request();
    } else if (input_length == 0) {
        /* Ignore empty commands. */
    } else {
        print("\nUnknown command. Type 'help'.\n");
    }

    input_length = 0;
    print("\nraktsetu> ");
}

void kernel_main(void)
{
    clear_screen();

    print("========================================\n");
    print("       RAKTSETU MINI OS KERNEL\n");
    print("========================================\n");
    print("Booted successfully through GRUB.\n");
    print("Type 'help' to list available commands.\n");
    print("\nraktsetu> ");

    for (;;) {
        if (!keyboard_ready()) {
            __asm__ volatile ("pause");
            continue;
        }

        uint8_t code = inb(0x60);

        /* Ignore key releases, while tracking Shift releases. */
        if (code == 0xAA || code == 0xB6) {
            shift_down = 0;
            continue;
        }

        if (code == 0x2A || code == 0x36) {
            shift_down = 1;
            continue;
        }

        if (code & 0x80) {
            continue;
        }

        /* Handle spacebar explicitly and store it in the command. */
        if (code == 0x39) {
            if (input_length < INPUT_SIZE - 1) {
                input[input_length++] = ' ';
                put_char(' ');
            }
            continue;
        }

        char c = scancode_to_ascii(code);
        if (c == 0) {
            continue;
        }

        if (c == '\n') {
            put_char('\n');
            execute_command();
        } else if (c == '\b') {
            if (input_length > 0) {
                input_length--;
                put_char('\b');
            }
        } else if (c >= 32 && c <= 126 && input_length < INPUT_SIZE - 1) {
            input[input_length++] = c;
            put_char(c);
        }
    }
}
