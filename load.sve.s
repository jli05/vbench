.global run_scalar, run_vector

.text
run_scalar:
	isb
	mrs	x4, cntvct_el0
	isb
	str	x4, [x2]
	cbz	x0, run_scalar_return
	mov	x4, #0x0                   	// #0

run_scalar_loop:
	ldr	w5, [x1, x4, lsl #2]
	add	x4, x4, #0x1
	cmp	x0, x4
	b.ne    run_scalar_loop

run_scalar_return:
	isb
	mrs	x4, cntvct_el0
	isb
	str	x4, [x2, #8]
	ret

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

run_vector_loop:
	whilelo	p7.s, x6, x0
	lsl	x5, x6, #2
	add	x7, x1, x5
	ld1w	{z31.s}, p7/z, [x7]
	add	x6, x6, x8
	cmp	x0, x6
	b.gt	run_vector_loop

run_vector_return:
	isb
	mrs	x5, cntvct_el0
	isb
	str	x5, [x2, #16]
	ret
