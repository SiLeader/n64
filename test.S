abc:
    inc rs3
    add rs0, rs3, r0
    not rt0, rt31
    call jump
    asgn rt16, 0xabcd
    asgn rt17, 0123
    asgn rt18, 123
    nop
    raise
    mov rs1, rs3
    hlt

jump:
    asgn rs16, 0xabcd
    asgn rs17, 0123
    asgn rs18, 123
    ret
