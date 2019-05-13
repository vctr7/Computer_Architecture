      lw    0    0    zero
      lw    0    1    zero    
      lw    0    2    mcand
      lw    0    3    mplier
      lw    0    4    one    
      lw    0    5    max     
      lw    0    6    zero      
      lw    0    7    one       
loop  nor   3    3    3       
      nor   4    4    4       
      nor   3    4    6       
      nor   3    3    3       
      nor   4    4    4       
      beq   0    6    func    
      add   2    1    1       
func  add   4    4    4       
      add   2    2    2       
      beq   4    5    done   
      beq   0    0    loop
done  halt
mcand   .fill 32766
mplier  .fill 10383
max	.fill 32768
zero    .fill   0
one     .fill   1
