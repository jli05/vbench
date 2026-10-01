.global get_vl

.text
get_vl:
    cntb  x0
    lsl   x0, x0, #3
