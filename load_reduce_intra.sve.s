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
        mov     w10, #0

run_vector_loop:
	whilelo	p7.s, x6, x0
	lsl     x5, x6, #2
	add	x7, x1, x5
	ld1w	z31.s, p7/z, [x7]
        saddv   d11, p7, z31.s
        fmov    x11, d11
	add     w10, w10, w11
	add	x6, x6, x8
	cmp	x0, x6
	b.gt	run_vector_loop

run_vector_return:
        mov     w0, w10
	isb
	mrs	x5, cntvct_el0
	isb
	str	x5, [x2, #16]
	ret
