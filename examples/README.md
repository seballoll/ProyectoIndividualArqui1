# Ejemplos de RISC-V con QEMU

Este directorio contiene ejemplos para desarrollo en RISC-V utilizando QEMU y GDB.

## Ejemplos disponibles

### 1. Assembly puro (`asm-only/`)
- **Descripción**: Programa simple escrito completamente en ensamblador RISC-V
- **Archivo principal**: `test.s`
- **Funcionalidad**: Calcula la suma de números del 1 al 10

### 2. C + Assembly (`c-asm/`)
- **Descripción**: Programa en C que llama funciones escritas en ensamblador
- **Archivos principales**: `example.c`, `math_asm.s`
- **Funcionalidad**: Programa en C que llama una función en ensamblador para calcular sumas

## Uso general

Cada ejemplo tiene su propio directorio con:
- Código fuente
- Script de compilación (`build.sh`)
- Script para ejecutar QEMU (`run-qemu.sh`)
- Linker script (`linker.ld`)

Para usar cualquier ejemplo:

1. Navegar al directorio del ejemplo
2. Ejecutar `./build.sh` para compilar
3. Ejecutar `./run-qemu.sh` para iniciar QEMU con servidor GDB
4. En otra terminal, conectar GDB para depuración

## Comandos útiles de GDB

```gdb
target remote :1234    # Conectar al servidor GDB
break _start           # Punto de ruptura en el inicio
continue               # Continuar ejecución
step                   # Ejecutar siguiente instrucción
info registers         # Mostrar registros
layout asm             # Vista de ensamblador
layout regs            # Vista de registros
monitor quit           # Finalizar aplicación remota (qemu)
```