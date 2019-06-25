        lw      0   1   input           
        lw      0   2   cnt              
        add     0   0   3               
        lw      0   4   neg1            
        lw      0   5   addr           
        jalr    5   6                  
        sw      0   3   100             
        halt
func    add     3   1   3               
        add     2   4   2               
        beq     0   2   done            
        beq     0   0   func
done    jalr    6   7
addr   .fill    func
cnt    .fill    10
input  .fill    3
neg1   .fill    -2
