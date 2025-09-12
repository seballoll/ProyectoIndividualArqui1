

// constants
#define DELTA 0x9E3779B9
#define ROUNDS 32



extern void tea_encrypt_asm(unsigned int *v, const unsigned int *k);
extern void tea_decrypt_asm(unsigned int *v, const unsigned int *k);

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

// --- Padding y manejo de bloques ---
#define MAX_INPUT_LEN 128

// Procesa la entrada dividiéndola en bloques de exactamente 64 bits (8 bytes)
// Para inputs > 64 bits: divide en múltiples bloques, aplica padding al último si necesario
// Para inputs ≤ 64 bits: un solo bloque con padding si necesario
// Cada bloque resultante = 2 palabras de 32 bits para la función de ensamblador
int pad_and_copy(const char* src, unsigned char* dst, int maxlen) {
    int len = 0;
    // Copiar caracteres de entrada
    while (src[len] && len < maxlen) {
        dst[len] = (unsigned char)src[len];
        len++;
    }
    
    // Aplicar padding para completar el último bloque a 8 bytes (64 bits)
    // Ejemplos:
    // - 5 bytes → padding de 3 bytes → 8 bytes (1 bloque)
    // - 12 bytes → padding de 4 bytes → 16 bytes (2 bloques)
    // - 16 bytes → sin padding → 16 bytes (2 bloques)
    int remainder = len % 8;
    if (remainder != 0) {
        int pad_needed = 8 - remainder;
        for (int i = 0; i < pad_needed; i++) {
            dst[len++] = 0x00;
        }
    }
    
    // Si la longitud original era 0, crear un bloque mínimo de 8 bytes
    if (len == 0) {
        for (int i = 0; i < 8; i++) {
            dst[len++] = 0x00;
        }
    }
    
    return len;
}

void print_block(const unsigned char* block) {
    for (int i = 0; i < 8; i++) {
        if (i > 0) print_char(' ');
        unsigned int b = block[i];
        if (b < 16) print_char('0');
        if (b < 10) {
            print_char('0' + b);
        } else {
            print_char('A' + b - 10);
        }
    }
}

// Función para leer un carácter desde UART (bare-metal)
char read_char_uart() {
    volatile char *uart_status = (volatile char*)0x10000005;  // Status register
    volatile char *uart_data = (volatile char*)0x10000000;    // Data register
    
    // Esperar hasta que haya un carácter disponible
    while ((*uart_status & 0x01) == 0) {
        // Busy wait - en bare-metal no hay interrupciones
        __asm__ volatile ("nop");
    }
    
    return *uart_data;
}

// Función para leer un número (1-4) desde consola
int read_option() {
    char c;
    int option = 1; // Por defecto
    
    print_string("Ingrese opcion (1-4): ");
    
    // Leer caracteres hasta obtener un número válido
    while (1) {
        c = read_char_uart();
        
        // Echo del carácter (mostrarlo en pantalla)
        print_char(c);
        
        if (c >= '1' && c <= '4') {
            option = c - '0';  // Convertir de ASCII a número
            print_string(" - Opcion seleccionada!\n\n");
            break;
        } else if (c == '\r' || c == '\n') {
            // Enter presionado, usar opción por defecto
            print_string(" - Usando opcion por defecto (1)\n\n");
            break;
        } else {
            // Carácter inválido
            print_string(" <- Invalido. Use 1-4: ");
        }
    }
    
    return option;
}


void print_text_representation(const unsigned char* data, int len) {
    print_string("Texto: \"");
    for (int i = 0; i < len; i++) {
        char c = (char)data[i];
        if (c >= 32 && c <= 126) { // Caracteres imprimibles
            print_char(c);
        } else if (c == 0) {
            print_string("\\0");
        } else {
            print_char('?');
        }
    }
    print_string("\"\n");
}

// Función para mostrar la separación en 2 palabras de 32 bits
void print_32bit_words(const unsigned char* block) {
    // Extraer las dos palabras de 32 bits del bloque de 64 bits
    unsigned int word1 = ((unsigned int)block[0]) | 
                        ((unsigned int)block[1] << 8) | 
                        ((unsigned int)block[2] << 16) | 
                        ((unsigned int)block[3] << 24);
    
    unsigned int word2 = ((unsigned int)block[4]) | 
                        ((unsigned int)block[5] << 8) | 
                        ((unsigned int)block[6] << 16) | 
                        ((unsigned int)block[7] << 24);
    
    print_string("  Palabra 1 (32 bits): ");
    print_hex(word1);
    print_string("\n  Palabra 2 (32 bits): ");
    print_hex(word2);
    print_string("\n");
}

void main() {
    // Claves predefinidas
    unsigned int key[4]  = {0x12345678, 0x9ABCDEF0, 0xFEDCBA98, 0x76543210};
    
    // Cadenas predefinidas para demostrar diferentes casos de padding y bloques
    const char* option1 = "HOLA1234";                    // 8 bytes = 1 bloque exacto (64 bits)
    const char* option2 = "Mensaje de prueba para TEA";   
    const char* option3 = "Este es un mensaje de prueba muy largo"; // >8 bytes → múltiples bloques
    const char* option4 = "Texto"; // Cadena vacía (0 bytes → padding → 1 bloque)
    
    unsigned char padded[MAX_INPUT_LEN+8];
    int input_len = 0;
    int opcion = 1; // Valor por defecto

    // Interfaz simple: elegir opción
    print_string("=== CIFRADOR TEA (cada bloque = 2 palabras de 32 bits) ===\n");
   

    opcion = read_option() ; // Cambiar a 1, 2 o 3 para probar diferentes casos

    print_number(opcion);
    print_string("\n\n");

    // Seleccionar entrada según opción
    const char* selected_input;
    if (opcion == 1) {
        selected_input = option1;
        print_string("Seleccionado: \"HOLA1234\" \n");
    } else if (opcion == 2) {
        selected_input = option2;
        print_string("Seleccionado: \"Mensaje de prueba para TEA\" \n");

    } else if (opcion == 3) {
        selected_input = option3;
        print_string("Seleccionado: Mensaje largo \n");
    } else {
        selected_input = option4;
        print_string("Seleccionado: Cadena pequeña \n");
    }

    // Procesar entrada: aplicar padding para completar bloques de 64 bits
    input_len = pad_and_copy(selected_input, padded, MAX_INPUT_LEN);
    
    int original_len = 0;
    while (selected_input[original_len]) original_len++; // Longitud original
    
    print_string("Longitud original: ");
    print_number(original_len);
    print_string(" bytes\n");
    print_string("Longitud con padding: ");
    print_number(input_len);
    print_string(" bytes (");
    print_number(input_len * 8);
    print_string(" bits)\n");
    print_string("Bloques de 64 bits: ");
    print_number(input_len / 8);
    print_string("\n");
    if (input_len > original_len) {
        print_string("Padding aplicado: ");
        print_number(input_len - original_len);
        print_string(" bytes (al ultimo bloque)\n");
    }
    print_string("\n");

    print_string("=== BLOQUES ORIGINALES (cada uno = 64 bits) ===\n");
    for (int i = 0; i < input_len; i += 8) {
        print_string("Bloque ");
        print_number((i/8) + 1);
        print_string(" (64 bits): ");
        print_block(&padded[i]);
        print_string("\n");
        print_32bit_words(&padded[i]);
        print_string("\n");
    }

    // Cifrado: cada bloque de 8 bytes (64 bits) se cifra como 2 palabras de 32 bits
    print_string("\n=== CIFRANDO (cada bloque por separado) ===\n");
    for (int i = 0; i < input_len; i += 8) {
        print_string("Cifrando bloque ");
        print_number((i/8) + 1);
        print_string(" - Enviando al ensamblador:\n");
        print_32bit_words(&padded[i]);
        // Pasar 2 palabras de 32 bits (total 64 bits) al ensamblador
        tea_encrypt_asm((unsigned int*)&padded[i], key);
        print_string("  Bloque cifrado.\n\n");
    }

    print_string("=== BLOQUES CIFRADOS ===\n");
    for (int i = 0; i < input_len; i += 8) {
        print_string("Bloque ");
        print_number((i/8) + 1);
        print_string(" cifrado: ");
        print_block(&padded[i]);
        print_string("\n");
        print_32bit_words(&padded[i]);
        print_string("\n");
    }

    // Descifrado: restaurar cada bloque de 64 bits
    print_string("\n=== DESCIFRANDO ===\n");
    for (int i = 0; i < input_len; i += 8) {
        print_string("Descifrando bloque ");
        print_number((i/8) + 1);
        print_string(" - Enviando al ensamblador:\n");
        print_32bit_words(&padded[i]);
        // Pasar 2 palabras de 32 bits (total 64 bits) al ensamblador
        tea_decrypt_asm((unsigned int*)&padded[i], key);
        print_string("  Bloque descifrado.\n\n");
    }

    print_string("=== BLOQUES DESCIFRADOS ===\n");
    for (int i = 0; i < input_len; i += 8) {
        print_string("Bloque ");
        print_number((i/8) + 1);
        print_string(" descifrado: ");
        print_block(&padded[i]);
        print_string("\n");
        print_32bit_words(&padded[i]);
        print_string("\n");
    }
    
    print_string("\nTexto final descifrado: ");
    print_text_representation(padded, input_len);

    print_string("\n=== RESUMEN ===\n");
    print_string("- Cada bloque procesado: exactamente 64 bits\n");
    print_string("- Separado en 2 palabras de 32 bits cada una\n");
    print_string("- Funciones ASM reciben: v[0] y v[1] (32 bits c/u)\n");
    print_string("- Padding aplicado cuando necesario\n");
    print_string("=== PRUEBAS COMPLETADAS ===\n");
    
    while (1) { __asm__ volatile ("nop"); }
}

