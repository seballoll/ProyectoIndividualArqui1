.section .text
.globl _start
_start:
    li t0, 10
    li t1, 1
    li t2, 0

loop_sum:
    add t2, t2, t1
    addi t1, t1, 1
    ble t1, t0, loop_sum
end:
    j end
    # The program ends here, but we loop infinitely to keep the QEMU session alive.
    