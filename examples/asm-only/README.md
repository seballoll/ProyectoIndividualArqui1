# Ejemplo: Assembly puro

Este ejemplo demuestra un programa simple escrito completamente en ensamblador RISC-V.

## Archivos

- `test.s`: Código fuente en ensamblador
- `linker.ld`: Script de enlazado
- `build.sh`: Script de compilación
- `run-qemu.sh`: Script para ejecutar QEMU

## Funcionalidad

El programa calcula la suma de números del 1 al 10 utilizando un bucle en ensamblador:
- Inicializa registros con valores
- Usa un bucle para sumar números consecutivos
- Termina en un bucle infinito

## Compilación y ejecución

```bash
# Compilar
./build.sh

# Ejecutar con QEMU (en una terminal)
./run-qemu.sh

# En otra terminal, conectar GDB
docker exec -it rvqemu /bin/bash
cd /home/rvqemu-dev/workspace/examples/asm-only
gdb-multiarch test.elf
```

## Depuración con GDB

```gdb
target remote :1234
break _start
continue
layout asm
layout regs
step  # o 's' para ejecutar paso a paso
monitor quit # para terminar la aplicación de qemu
quit # para salir
```

## Registros importantes

- `t0`: Límite superior (10)
- `t1`: Contador
- `t2`: Acumulador de suma