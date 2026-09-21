.global run_vector

.text
run_vector:
	isb
	mrs	x5, cntvct_el0
	isb
	str	x5, [x4]
	cmp	x0, #0x0
	b.le	run_vector_return
	mov	x6, #0x0                   	// #0
	cntw	x8
        isb
        mrs     x5, cntvct_el0
        isb
        str     x5, [x4, #8]

run_vector_loop:
	whilelt	p7.s, x6, x0
	sbfiz	x5, x6, #2, #32
	add	x7, x1, x5
	ld1w	{z31.s}, p7/z, [x7]
	add	x7, x2, x5
	ld1w	{z30.s}, p7/z, [x7]
	mul	z31.s, p7/m, z31.s, z30.s
	add	x5, x3, x5
	st1w	{z31.s}, p7, [x5]
	add	x6, x6, x8
	cmp	x0, x6
	b.gt	run_vector_loop

run_vector_return:
	isb
	mrs	x0, cntvct_el0
	isb
	str	x0, [x4, #16]
	ret
