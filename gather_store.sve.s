.global run_vector

.text
run_vector:
	isb
	mrs	x9, cntvct_el0
	isb
	str	x9, [x4]
	cntd	x8
        isb
        mrs     x9, cntvct_el0
        isb
        str     x9, [x4, #8]
	cmp	x0, #0x0
	b.le	run_vector_return
	mov	x6, #0x0                   	// #0

run_vector_loop:
	whilelo	p7.d, x6, x0
        lsl     x9, x6, #3
        add     x7, x2, x9
	ld1d	{z31.d}, p7/z, [x7]
        add     x7, x5, x9
        st1d    {z31.d}, p7, [x7]
        ld1w    {z30.d}, p7/z, [x1, z31.d, lsl #2]
        lsl     x9, x6, #2
        add     x7, x3, x9
        st1w    {z30.d}, p7, [x7]
	add	x6, x6, x8
	cmp	x0, x6
	b.gt	run_vector_loop

run_vector_return:
	isb
	mrs	x9, cntvct_el0
	isb
	str	x9, [x4, #16]
	ret
