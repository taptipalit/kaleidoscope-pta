import os
import subprocess
import sys



os.system("find . -name \"*.o\" > bc.list")

f = open("bc.list")

bc_files=[]
elf_files=[]

for line in f:
    ret = os.system("readelf -h "+line)
    if ret != 0:
        bc_files.append(line.strip())
    else:
        elf_files.append(line.strip())

print ("Bitcode files:")
print (bc_files)
print (len(bc_files))

print ("ELF files:")
print (elf_files)
print (len(elf_files))

cmds = []
cmds.append("llvm-link")
for bc in bc_files:
    cmds.append(bc)
cmds.append("-o")
cmds.append("linked.bc")

subprocess.run(cmds)

