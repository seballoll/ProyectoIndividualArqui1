

// constants
#define DELTA 0x9E3779B9
#define ROUNDS 32



extern void tea_encrypt(unsigned int *v, const unsigned int *k);
extern void tea_decrypt(unsigned int *v, const unsigned int *k);

void print_char(char c) {
    // In a real bare-metal environment, this would write to UART
    // For now, this is just a placeholder
    volatile char *uart = (volatile char*)0x10000000;
    *uart = c;
}

void print_number(int num) {
    if (num == 0) {
        print_char('0');
        return;
    }
    
    if (num < 0) {
        print_char('-');
        num = -num;
    }
    
    char buffer[10];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // Print digits in reverse order
    while (i > 0) {
        print_char(buffer[--i]);
    }
}

void print_string(const char* str) {
    while (*str) {
        print_char(*str++);
    }
}

void print_hex(unsigned int num) {
    print_string("0x");
    
    // Print 8 hex digits (32-bit number)
    for (int i = 7; i >= 0; i--) {
        int digit = (num >> (i * 4)) & 0xF;
        if (digit < 10) {
            print_char('0' + digit);
        } else {
            print_char('A' + digit - 10);
        }
    }
}

void main() {
    // Example data and key
    // Data is two 32-bit integers (v[0], v[1])
    // Key is four 32-bit integers (k[0], k[1], k[2], k[3])
    // The data will be encrypted and then decrypted to verify correctness
    // Initial values are chosen for demonstration purposes
    
    unsigned int data[2] = {0x484f4c41, 0x31323334};
    unsigned int key[4]  = {0x12345678, 0x9ABCDEF0, 0xFEDCBA98, 0x76543210};

    print_string("Original: ");
    print_hex(data[0]);
    print_char(' ');
    print_hex(data[1]);
    print_char('\n');

    // Encrypt the data

    tea_encrypt(data, key);
    print_string("Encrypted: ");
    print_hex(data[0]);
    print_char(' ');
    print_hex(data[1]);
    print_char('\n');

    tea_decrypt(data, key);
    print_string("Decrypted: ");
    print_hex(data[0]);
    print_char(' ');
    print_hex(data[1]);
    print_char('\n');



    print_string("Tests completed.\n");
    
    // Infinite loop to keep program running
    while (1) {
        __asm__ volatile ("nop");
    }

}

