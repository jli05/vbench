.global run_vector

.text
run_vector:
	isb
	mrs	x5, cntvct_el0
	isb
	str	x5, [x2]
	cntw	x8
        isb
        mrs     x5, cntvct_el0
        isb
        str     x5, [x2, #8]
	cmp	x0, #0x0
	b.le	run_vector_return
	mov	x6, #0x0                   	// #0
        dup     z30.s, #0

run_vector_loop:
	whilelo	p7.s, x6, x0
        lsl     x5, x6, #2
	add	x7, x1, x5
	ld1w	z31.s, p7/z, [x7]
        add     z30.s, p7/m, z30.s, z31.s
	add	x6, x6, x8
	cmp	x0, x6
	b.gt	run_vector_loop

run_vector_return:
        ptrue   p7.s
        saddv   d11, p7, z30.s
        fmov    x0, d11
	isb
	mrs	x5, cntvct_el0
	isb
	str	x5, [x2, #16]
	ret
