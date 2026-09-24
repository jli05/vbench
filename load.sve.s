.global run_scalar, run_vector

.text
run_scalar:
	isb
	mrs	x5, cntvct_el0
	isb
	str	x5, [x2]
	cbz	x0, run_scalar_return
	lsl	x7, x0, #2
	mov	x0, #0x0                   	// #0

run_scalar_loop:
	ldr	w5, [x1, x0]
	add	x0, x0, #0x4
	cmp	x7, x0
	b.ne	run_scalar_loop

run_scalar_return:
	isb
	mrs	x0, cntvct_el0
	isb
	str	x0, [x2, #8]
	ret

run_vector:
	isb
	mrs	x5, cntvct_el0
	isb
	str	x5, [x2]
	cmp	x0, #0x0
	b.le	run_vector_return
	mov	x6, #0x0                   	// #0
	cntw	x8
        isb
        mrs     x5, cntvct_el0
        isb
        str     x5, [x2, #8]

run_vector_loop:
	whilelt	p7.s, x6, x0
	sbfiz	x5, x6, #2, #32
	add	x7, x1, x5
	ld1w	{z31.s}, p7/z, [x7]
	add	x6, x6, x8
	cmp	x0, x6
	b.gt	run_vector_loop

run_vector_return:
	isb
	mrs	x0, cntvct_el0
	isb
	str	x0, [x2, #16]
	ret
