	lw	0	1	A
	lw	0	2	B
	lw	0	3	C
	lw	0	4	D
	add	1	2	1
	add	1	3	1	double data hazard
	add 	1	4	1	reg1 should be 10
	halt
A	.fill	1
B	.fill	2
C	.fill	3
D	.fill	4
