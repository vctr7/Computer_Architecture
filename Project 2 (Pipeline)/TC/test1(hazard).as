	lw	0	1	data1
	noop
	noop
	noop
	beq	0	0	3	branch taken
	add	1	1	2	change to noop
	add	1	1	3	change to noop
	add	1	1	4 	change to noop
	halt				
data1	.fill	1
