#!/usr/bin/python
#
import sys
import math

INTERVAL_LENGTH=3.141592
N_ENTRIES=int(sys.argv[1])

print("unsigned char sintab[] = {")
for x in range(0,N_ENTRIES):
    val = int(math.floor(math.sin((INTERVAL_LENGTH*x)/N_ENTRIES)*256))
    if (x % 8 == 0):
        sys.stdout.write("        ")
    
    sys.stdout.write("0x%02x," % val)

    if ((x % 8) == 7):
        sys.stdout.write("\n")
    else:
        sys.stdout.write(" ")
print("};")
