start	lw	0	0	zero
	lw	1	1	one
	lw	2	2	two
	lw	3	3	three
	add	0	4	4
	beq	0	1	start
	sw	0	0	A4
	lw	0	7	four
	beq	0	2	start
	sw	0	1	A3
	add	0	1	5
	beq	0	1	start
	sw	0	2	A2
	add	1	2	6
	sw	0	3	A1
	beq	0	7	start
	nor	4	5	1
	sw	0	7	A0
	beq	0	6	start
	halt
zero	.fill	0
one	.fill	1
two	.fill	2
three	.fill	3
four	.fill	4
A0	.fill	zero
A1	.fill	one
A2	.fill	two
A3	.fill	three
A4	.fill	four
