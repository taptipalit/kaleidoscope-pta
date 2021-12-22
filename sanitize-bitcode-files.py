import os
import subprocess
import sys

coreBitcodeFiles = []
elfFiles = []
linuxModuleBitcodeFiles = []

def isLinuxModule(fileName):
    ret = os.system("llvm-dis "+fileName)
    if ret == 0:
        bcTextFile = open(fileName + ".ll")
        with open(fileName+".ll", 'r') as bcTextFile:
            data = bcTextFile.read().strip()
        if "init_module" in data:
            return True
        else:
            return False
    else:
        print("Error in disassembling llvm bitcode\n")

def getObjectFiles():
    os.system("find . -name \"*.o\" > bc.list")


def getBitcodeFiles():
    f = open("bc.list")
    for line in f:
        ret = os.system("readelf -h "+line)
        if ret != 0:
            line = line.strip()
            # It is a LLVM bitcode
            # If it's a Linux module, then process them differently
            if isLinuxModule(line):
                linuxModuleBitcodeFiles.append(line)
            else: 
                coreBitcodeFiles.append(line.strip())
        else:
            elfFiles.append(line.strip())

def buildLinkedBitcode():
    getObjectFiles()
    getBitcodeFiles()

    print ("Bitcode files:")
    print (coreBitcodeFiles)
    print (len(coreBitcodeFiles))
    
    print ("ELF files:")
    print (elfFiles)
    print (len(elfFiles))

    print ("Linux module files:")
    print (linuxModuleBitcodeFiles)
    print (len(linuxModuleBitcodeFiles))


    cmds = []
    cmds.append("llvm-link")
    for bc in coreBitcodeFiles:
        ret = os.system("llvm-dis "+bc)
        if ret == 0:
            cmds.append(bc)
    cmds.append("-o")
    cmds.append("linked.bc")

    subprocess.run(cmds)

if __name__ == "__main__":
    buildLinkedBitcode()
