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

static void show_help(void)
{
    print("\nAvailable commands:\n");
    print("  help    - List commands\n");
    print("  clear   - Clear terminal\n");
    print("  about   - About RaktSetu Mini OS\n");
    print("  status  - Kernel status\n");
    print("  blood   - Show blood inventory\n");
    print("  request - Request blood: request GROUP UNITS\n");
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
    } else if (equals(input, "blood")) {
        show_blood();
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
