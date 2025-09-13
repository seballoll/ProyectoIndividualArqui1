# ProyectoIndividualArqui1
Proyecto individual del curso Arquitectura de Computadores 1 Segundo semestre 2025

## Implementación del Algoritmo TEA (Tiny Encryption Algorithm) en RISC-V

Este proyecto implementa el algoritmo de cifrado TEA usando una combinación de C y ensamblador RISC-V, ejecutándose en un entorno bare-metal emulado con QEMU.

## Tabla de Contenidos

1. [Descripción de la Arquitectura del Software](#1-descripción-de-la-arquitectura-del-software)
2. [Explicación de las Funcionalidades Implementadas](#2-explicación-de-las-funcionalidades-implementadas)
3. [Documentación de Evidencias de Ejecución de GDB y QEMU](#3-documentación-de-evidencias-de-ejecución-de-gdb-y-qemu)
4. [Breve Discusión de Resultados](#4-breve-discusión-de-resultados)
5. [Instrucciones para Compilar, Ejecutar y Utilizar el Sistema](#5-instrucciones-para-compilar-ejecutar-y-utilizar-el-sistema)

---

## 1. Descripción de la Arquitectura del Software

### Separación entre Capas C y Ensamblador

La arquitectura del proyecto está diseñada siguiendo una separación clara de responsabilidades entre el código C y el ensamblador RISC-V:

#### **Capa C (`TEA.c`)**
- **Responsabilidades principales:**
  - Gestión de entrada/salida (UART simulation)
  - Manejo de padding para bloques de 64 bits
  - División de datos en bloques apropiados para TEA
  - Interfaz de usuario y presentación de resultados
  - Orquestación del proceso de cifrado/descifrado

- **Funciones clave:**
  - `pad_and_copy()`: Aplica padding PKCS#7 para completar bloques de 64 bits
  - `print_*()`: Familia de funciones para output formateado
  - `read_option()`: Manejo de entrada de usuario vía UART
  - `main()`: Coordinación del flujo completo del programa

#### **Capa Ensamblador (`TEA.s`)**
- **Responsabilidades principales:**
  - Implementación pura del algoritmo TEA según especificación
  - Operaciones criptográficas de bajo nivel
  - Manejo directo de registros RISC-V
  - Optimización de rendimiento en operaciones registro-registro

- **Funciones exportadas:**
  - `tea_encrypt_asm()`: Cifrado de un bloque de 64 bits
  - `tea_decrypt_asm()`: Descifrado de un bloque de 64 bits

### Interfaces Utilizadas

#### **Interfaz C ↔ Assembly**
```c
// Prototipos de funciones en ensamblador
extern void tea_encrypt_asm(unsigned int *v, const unsigned int *k);
extern void tea_decrypt_asm(unsigned int *v, const unsigned int *k);
```

**Convención de llamada RISC-V:**
- `a0`: Puntero al bloque de datos (2 palabras de 32 bits)
- `a1`: Puntero a la clave (4 palabras de 32 bits)
- Preservación de registros `s0-s11` y `ra`
- Uso de `t0-t6` para cálculos temporales

#### **Interfaz Hardware Simulado**
- **UART Base Address:** `0x10000000`
- **UART Status Register:** `0x10000005`
- Simulación de I/O de caracteres para debugging

### Justificación de las Decisiones de Diseño

#### **1. Separación C/Assembly**
- **Ventaja:** El C maneja la complejidad de manejo de datos variables, mientras assembly optimiza el núcleo criptográfico
- **Motivación:** TEA requiere 32 rondas de operaciones bit-wise intensivas que se benefician de control directo de registros

#### **2. Padding Strategy**
- **Implementación:** Padding a nivel de bloque (64 bits) con ceros
- **Justificación:** TEA opera sobre bloques fijos de 64 bits; el padding garantiza alineación correcta

#### **3. Manejo de Múltiples Bloques**
- **Estrategia:** Procesamiento secuencial bloque por bloque
- **Beneficio:** Simplifica debugging y permite visualización del proceso

#### **4. Bare-metal Approach**
- **Decisión:** Sin dependencias de OS ni librerías
- **Ventaja:** Control total sobre recursos y timing, ideal para sistemas embebidos

---

## 2. Explicación de las Funcionalidades Implementadas

### Algoritmo TEA (Tiny Encryption Algorithm)

#### **Especificaciones del Algoritmo**
- **Tamaño de bloque:** 64 bits (2 palabras de 32 bits)
- **Tamaño de clave:** 128 bits (4 palabras de 32 bits)
- **Número de rondas:** 32
- **Constante DELTA:** `0x9E3779B9` (proporción áurea)

#### **Implementación de Cifrado**

**Pseudocódigo del algoritmo:**
```
sum = 0
for i = 0 to 31:
    sum += DELTA
    v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1)
    v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3)
```

**Implementación en Assembly (cifrado):**
```assembly
tea_encrypt_asm:
    # Cargar v0, v1 desde memoria
    lw t0, 0(a0)        # v0 = v[0]
    lw t1, 4(a0)        # v1 = v[1]
    
    # Cargar claves
    lw t2, 0(a1)        # key[0]
    lw t3, 4(a1)        # key[1]
    lw t4, 8(a1)        # key[2]
    lw t5, 12(a1)       # key[3]
    
    li t6, 0            # sum = 0
    li a2, 32           # rounds = 32

encrypt_loop:
    # sum += DELTA
    li a3, 0x9E3779B9
    add t6, t6, a3
    
    # Calcular término para v0
    sll a3, t1, 4       # v1 << 4
    add a3, a3, t2      # + key0
    add a4, t1, t6      # v1 + sum
    srl a5, t1, 5       # v1 >> 5
    add a5, a5, t3      # + key1
    xor a3, a3, a4      # XOR operaciones
    xor a3, a3, a5
    add t0, t0, a3      # v0 += resultado
    
    # Calcular término para v1 (similar)
    # ... [código análogo para v1]
    
    addi a2, a2, -1     # decrementar contador
    bnez a2, encrypt_loop
```

#### **Implementación de Descifrado**

El descifrado invierte el proceso:
- **sum inicial:** `0xC6EF3720` (DELTA × 32 mod 2³²)
- **Orden invertido:** Primero v1, después v0
- **Operación:** Resta en lugar de suma

### Funcionalidades de Soporte

#### **1. Sistema de Padding**
```c
int pad_and_copy(const char* src, unsigned char* dst, int maxlen) {
    int len = 0;
    // Copiar datos originales
    while (src[len] && len < maxlen) {
        dst[len] = (unsigned char)src[len];
        len++;
    }
    
    // Aplicar padding con ceros
    int remainder = len % 8;
    if (remainder != 0) {
        int pad_needed = 8 - remainder;
        for (int i = 0; i < pad_needed; i++) {
            dst[len++] = 0x00;
        }
    }
    
    return len;
}
```

#### **2. Visualización de Datos**
- **Representación hexadecimal:** Muestra cada byte en formato hex
- **Separación en palabras:** Visualiza bloques como dos palabras de 32 bits
- **Texto legible:** Convierte output a representación de caracteres cuando es posible

#### **3. Interfaz de Usuario**
- **Menú interactivo:** 4 opciones predefinidas con diferentes casos de prueba
- **Entrada por UART:** Simulación de input de consola
- **Feedback inmediato:** Confirmación de selecciones

### Casos de Prueba Implementados

1. **Opción 1:** `"HOLA1234"` (8 bytes exactos - 1 bloque sin padding)
2. **Opción 2:** `"Mensaje de prueba para TEA"` (26 bytes - múltiples bloques)
3. **Opción 3:** `"Este es un mensaje de prueba muy largo"` (>32 bytes - múltiples bloques)
4. **Opción 4:** `"Texto"` (5 bytes - 1 bloque con padding)

---

## 3. Documentación de Evidencias de Ejecución de GDB y QEMU

### Configuración del Entorno de Debugging

#### **Archivo de configuración GDB (`debug_test.gdb`)**
```gdb
target remote :1234
break _start
break main
break tea_encrypt_asm
break tea_decrypt_asm
layout asm
layout regs
continue
```


#### **Inicialización:**
```bash
# Terminal 1: Iniciar QEMU
./run-qemu.sh

# Terminal 2: Conectar GDB
docker exec -it rvqemu /bin/bash
cd /home/rvqemu-dev/workspace/examples/c-asm
gdb-multiarch TEA.elf
```

#### **Comandos GDB para análisis:**
```gdb
# Conexión y breakpoints
target remote :1234
break main
break tea_encrypt_asm
break tea_decrypt_asm

# Visualización
layout asm
layout regs
info registers

# Ejecución controlada
continue
step
stepi
next

# Análisis de memoria
x/8x $a0        # Ver contenido del bloque
x/4x $a1        # Ver claves
x/32i $pc       # Ver próximas instrucciones

# Monitoreo de variables
print $t0       # v0
print $t1       # v1
print $t6       # sum
```

---

## 4. Breve Discusión de Resultados

### Análisis de Correctitud

#### **Verificación del Algoritmo**
- **Test Vector 1:** `"HOLA1234"` se cifra y descifra correctamente
- **Integridad:** El texto original se recupera exactamente después del ciclo cifrado→descifrado
- **Padding:** Se aplica correctamente para mantener bloques de 64 bits

#### **Análisis de Rendimiento**
- **Rondas TEA:** Las 32 rondas se ejecutan correctamente en assembly
- **Manejo de registros:** Uso eficiente de registros temporales `t0-t6`
- **Stack management:** Preservación correcta de registros en llamadas

### Observaciones Importantes

#### **1. Fortalezas de la Implementación**
- **Modularidad:** Separación clara entre lógica de control (C) y operaciones críticas (Assembly)
- **Debugging:** Abundante información de estado durante ejecución
- **Flexibilidad:** Manejo de entradas de longitud variable
- **Estándares:** Cumple convenciones de llamada RISC-V

#### **2. Limitaciones Identificadas**
- **Tamaño máximo:** Limitado a 128 bytes de entrada (`MAX_INPUT_LEN`)
- **Padding simple:** Usa ceros en lugar de PKCS#7 estándar
- **Sin validación:** No verifica integridad de datos cifrados
- **Bare-metal:** Sin manejo de errores avanzado

#### **3. Casos Edge Manejados**
- **Entrada vacía:** Se maneja con padding completo
- **Múltiples bloques:** Procesamiento secuencial correcto
- **Alineación:** Garantiza bloques de 64 bits exactos

### Validación Criptográfica

#### **Propiedades Verificadas**
- **Reversibilidad:** `decrypt(encrypt(data)) == data`
- **Determinismo:** Misma entrada produce misma salida
- **Sensibilidad:** Cambios mínimos en entrada causan cambios significativos en salida



---

## 5. Instrucciones para Compilar, Ejecutar y Utilizar el Sistema

### Prerrequisitos del Sistema

#### **Herramientas Requeridas:**
- Docker (para entorno RISC-V)
- QEMU RISC-V
- GCC toolchain para RISC-V
- GDB con soporte multiarch

#### **Configuración del Entorno:**
```bash
# Clonar el repositorio
git clone https://github.com/seballoll/ProyectoIndividualArqui1.git
cd ProyectoIndividualArqui1/TEA

# Verificar archivos necesarios
ls -la
# Debe mostrar: TEA.c, TEA.s, startup.s, linker.ld, build.sh, run-qemu.sh
```

### Proceso de Compilación

#### **Script de Build (`build.sh`):**
```bash
#!/bin/bash
# Compilación automática del proyecto TEA

# Compilar código C
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -c -g TEA.c -o TEA.o

# Ensamblar código assembly
riscv64-unknown-elf-as -march=rv32i -mabi=ilp32 -g TEA.s -o TEAA.o
riscv64-unknown-elf-as -march=rv32i -mabi=ilp32 -g startup.s -o startup.o

# Enlazar todos los objetos
riscv64-unknown-elf-ld -T linker.ld startup.o TEA.o TEAA.o -o TEA.elf

echo "Compilación completada: TEA.elf"
```

#### **Ejecución de la Compilación:**
```bash
# Hacer ejecutable el script
chmod +x build.sh

# Compilar el proyecto
./build.sh
```

### Ejecución del Sistema

#### **Método 1: Ejecución Simple**
```bash
# Ejecutar con QEMU
chmod +x run-qemu.sh
./run-qemu.sh
```

#### **Método 2: Ejecución con Debugging**

**Terminal 1 - QEMU:**
```bash
./run-qemu.sh
# El sistema queda esperando conexión GDB en puerto 1234
```

**Terminal 2 - GDB:**
```bash
# Entrar al contenedor Docker
docker exec -it rvqemu /bin/bash

# Navegar al directorio del proyecto
cd /home/rvqemu-dev/workspace/examples/c-asm

# Iniciar sesión GDB
gdb-multiarch TEA.elf

# Dentro de GDB:
target remote :1234
break main
continue
```

### Manual de Usuario

#### **Interfaz del Programa**

Al ejecutar el programa, verás:
```
=== CIFRADOR TEA (cada bloque = 2 palabras de 32 bits) ===
Ingrese opcion (1-4): 
```

#### **Opciones Disponibles:**

1. **Opción 1 - Caso Básico:**
   - Entrada: `"HOLA1234"` (8 bytes exactos)
   - Resultado: 1 bloque sin padding

2. **Opción 2 - Mensaje Mediano:**
   - Entrada: `"Mensaje de prueba para TEA"`
   - Resultado: Múltiples bloques con padding

3. **Opción 3 - Mensaje Largo:**
   - Entrada: `"Este es un mensaje de prueba muy largo"`
   - Resultado: Múltiples bloques, demostración de escalabilidad

4. **Opción 4 - Caso con Padding:**
   - Entrada: `"Texto"` (5 bytes)
   - Resultado: 1 bloque con 3 bytes de padding

#### **Interpretación de la Salida**

**Formato de Output:**
```
Longitud original: X bytes
Longitud con padding: Y bytes (Z bits)
Bloques de 64 bits: N

=== BLOQUES ORIGINALES ===
Bloque 1 (64 bits): 48 4F 4C 41 31 32 33 34
  Palabra 1 (32 bits): 0x41434F48
  Palabra 2 (32 bits): 0x34333231

=== CIFRANDO ===
[Proceso de cifrado bloque por bloque]

=== BLOQUES CIFRADOS ===
[Datos cifrados en hexadecimal]

=== DESCIFRANDO ===
[Proceso de descifrado]

=== BLOQUES DESCIFRADOS ===
[Verificación de integridad]

Texto final descifrado: "HOLA1234"
```

### Ejemplos de Uso

#### **Ejemplo 1: Debugging Paso a Paso**
```bash
# 1. Iniciar QEMU con debugging
./run-qemu.sh

# 2. En otra terminal, conectar GDB
gdb-multiarch TEA.elf
(gdb) target remote :1234
(gdb) break tea_encrypt_asm
(gdb) continue
(gdb) layout asm
(gdb) layout regs

# 3. Interactuar con el programa
# Seleccionar opción 1 en QEMU

# 4. Cuando llegue al breakpoint
(gdb) info registers
(gdb) x/2x $a0  # Ver bloque a cifrar
(gdb) x/4x $a1  # Ver clave
(gdb) step      # Ejecutar paso a paso
```

#### **Ejemplo 2: Análisis de Performance**
```bash
# Usar opción 3 (mensaje largo) y observar:
# - Número de bloques procesados
# - Tiempo de procesamiento por bloque
# - Uso de memoria

(gdb) break tea_encrypt_asm
(gdb) commands
> info registers t0 t1 t6
> continue
> end
```

### Solución de Problemas Comunes

#### **Error: "Permission denied" en scripts**
```bash
chmod +x build.sh run-qemu.sh
```

#### **Error: "No such file or directory" en GDB**
```bash
# Verificar que TEA.elf existe
ls -la TEA.elf

# Recompilar si es necesario
./build.sh
```

#### **Error: "Connection refused" en GDB**
```bash
# Verificar que QEMU está ejecutándose
ps aux | grep qemu

# Reiniciar QEMU
./run-qemu.sh
```

#### **Error: "Breakpoint not hit"**
```bash
# Verificar que los símbolos están cargados
(gdb) info functions
(gdb) info variables

# Usar direcciones absolutas si es necesario
(gdb) break *0x80000000
```

---

## Conclusiones

Este proyecto demuestra exitosamente la implementación del algoritmo TEA en un entorno bare-metal RISC-V, combinando eficientemente código C para manejo de datos y ensamblador para operaciones criptográficas críticas. La arquitectura modular permite debugging detallado y fácil extensión para futuros desarrollos.

La implementación cumple con los estándares de las convenciones de llamada RISC-V y proporciona una base sólida para sistemas de cifrado en entornos embebidos de recursos limitados.
