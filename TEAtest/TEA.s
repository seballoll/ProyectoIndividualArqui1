    .section .text
    .global tea_encrypt_asm
    .type tea_encrypt_asm, @function


tea_encrypt_asm:

    # a0 = ptr v (v[0], v[1])
    # a1 = ptr key (key[0..3])
    # Registers usage:
    # t0 = v0, t1 = v1
    # t2 = key0, t3 = key1, t4 = key2, t5 = key3
    # t6 = sum
    # a2 = loop counter (i)
    # a3,a4,a5 = temporaries

    # Save return address
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    sw s1, 4(sp)
    sw s2, 0(sp)

    # load v0, v1
    lw    t0, 0(a0)        # v0 = v[0]
    lw    t1, 4(a0)        # v1 = v[1]

    # load keys
    lw    t2, 0(a1)        # key[0]
    lw    t3, 4(a1)        # key[1]
    lw    t4, 8(a1)        # key[2]
    lw    t5, 12(a1)       # key[3]

    li    t6, 0            # sum = 0
    li    a2, 32           # rounds = 32

encrypt_loop:
    # sum += DELTA
    # load DELTA into a3 and add to sum.
    # DELTA = 0x9E3779B9

    li     a3 , 0x9E3779B9
    add   t6, t6, a3

    # compute term for v0: ((v1 << 4) + key0) ^ (v1 + sum) ^ ((v1 >> 5) + key1)
    sll   a3, t1, 4            # a3 = v1 << 4
    add   a3, a3, t2           # a3 = (v1<<4) + key0

    add   a4, t1, t6           # a4 = v1 + sum

    srl   a5, t1, 5            # a5 = v1 >> 5 (logical)
    add   a5, a5, t3           # a5 = (v1>>5) + key1

    xor   a3, a3, a4           # a3 = ((v1<<4)+k0) ^ (v1+sum)
    xor   a3, a3, a5           # a3 = ... ^ ((v1>>5)+k1)
    add   t0, t0, a3           # v0 += a3

    # compute term for v1: ((v0 << 4) + key2) ^ (v0 + sum) ^ ((v0 >> 5) + key3)
    sll   a3, t0, 4            # a3 = v0 << 4
    add   a3, a3, t4           # a3 = (v0<<4) + key2

    add   a4, t0, t6           # a4 = v0 + sum

    srl   a5, t0, 5            # a5 = v0 >> 5
    add   a5, a5, t5           # a5 = (v0>>5) + key3

    xor   a3, a3, a4
    xor   a3, a3, a5
    add   t1, t1, a3           # v1 += a3

    addi a2, a2, -1           # rounds--
    bnez  a2, encrypt_loop

end_encrypt:
    # store back v0, v1
    sw    t0, 0(a0)
    sw    t1, 4(a0)

    # Restore registers and stack
    lw ra, 12(sp)
    lw s0, 8(sp)
    lw s1, 4(sp)
    lw s2, 0(sp)
    addi sp, sp, 16

    ret


    .global tea_decrypt_asm
    .type tea_decrypt_asm, @function
tea_decrypt_asm:
    # a0 = ptr v, a1 = ptr key
    # same register usage as encrypt

    # Save return address
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    sw s1, 4(sp)
    sw s2, 0(sp)

    lw    t0, 0(a0)        # v0
    lw    t1, 4(a0)        # v1

    lw    t2, 0(a1)        # key0
    lw    t3, 4(a1)        # key1
    lw    t4, 8(a1)        # key2
    lw    t5, 12(a1)       # key3

    # sum = DELTA * 32 mod 2^32 = 0xC6EF3720
    li    t6, 0xC6EF3720    # t6 = 0xC6EF3720

    li    a2, 32           # rounds

decrypt_loop:
    # v1 -= ((v0 << 4) + key2) ^ (v0 + sum) ^ ((v0 >> 5) + key3)
    sll   a3, t0, 4
    add   a3, a3, t4
    add   a4, t0, t6
    srl   a5, t0, 5
    add   a5, a5, t5
    xor   a3, a3, a4
    xor   a3, a3, a5
    sub   t1, t1, a3

    # v0 -= ((v1 << 4) + key0) ^ (v1 + sum) ^ ((v1 >> 5) + key1)
    sll   a3, t1, 4
    add   a3, a3, t2
    add   a4, t1, t6
    srl   a5, t1, 5
    add   a5, a5, t3
    xor   a3, a3, a4
    xor   a3, a3, a5
    sub   t0, t0, a3

    # sum -= DELTA
    # DELTA = 0x9E3779B9; subtract by adding its two's complement
    li     a3 , 0x9E3779B9
    sub   t6, t6, a3

    addi a2, a2, -1
    bnez  a2, decrypt_loop

    sw    t0, 0(a0)
    sw    t1, 4(a0)

    # Restore registers and stack
    lw ra, 12(sp)
    lw s0, 8(sp)
    lw s1, 4(sp)
    lw s2, 0(sp)
    addi sp, sp, 16

    ret

