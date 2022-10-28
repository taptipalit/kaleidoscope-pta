#!/usr/bin/python3

import os
import sys
import subprocess
import re

LLVM_ONLY=False
C_ONLY=False
LLVM_AND_C=True

#RED='\033[0;31m'
#NC='\033[0m' # No Color

RED=''
NC=''

class Value:
    def __init__(self, valName, ln, fl, cSourceLine):
        self.valName = valName.strip()
        self.ln = ln.strip()
        self.fl = fl.strip()
        self.cSourceLine = cSourceLine.strip()

    def __str__(self):
        if LLVM_AND_C:
            formatStr = "%s [ %s %s ] " + RED + "(%s)" + NC
            return formatStr % (self.valName, self.fl, self.ln, self.cSourceLine)
        elif C_ONLY:
            return "%s [%s %s]" % (self.cSourceLine, self.fl, self.ln)
        else:
            return "%s [ %s %s ]" % (self.valName, self.fl, self.ln)


class Copy:
    def __init__(self, srcID, dstID):
       self.srcID = srcID
       self.dstID = dstID

    def setSrc(self, srcVal):
        self.srcVal = srcVal

    def setDst(self, dstVal):
        self.dstVal = dstVal

    def setDerived(self, derived):
        self.derived = derived

    def setPtdSet(self, ptdSet):
        self.ptdSet = ptdSet

    def setOrigVal(self, v):
        self.origVal = v

    def __str__(self): 
        return "%s --(copy)--> %s { %s }" % (self.srcVal, self.dstVal, self.origVal) 

class Gep:
    def __init__(self, srcID, dstID):
       self.srcID = srcID
       self.dstID = dstID

    def setSrc(self, srcVal):
        self.srcVal = srcVal

    def setDst(self, dstVal):
        self.dstVal = dstVal

    def setDerived(self, derived):
        self.derived = derived

    def setPtdSet(self, ptdSet):
        self.ptdSet = ptdSet

    def setFlxIdx(self, fldIdx):
        self.fldIdx = fldIdx

    def __str__(self): 
        return "%s --(gep)--> %s" % (self.srcVal, self.dstVal) 

def parseDloc(dloc):
    tokens = dloc.split()
    # print(tokens)
    if len(tokens) < 3:
        return ("0", "0")
    if "Glob" in dloc:
        ln = tokens[2]
        fl = tokens[4]
    else:
        if "cl" in dloc:
            ln = tokens[1]
            fl = tokens[5]
        else:
            ln = tokens[1]
            fl = tokens[3]
    return (ln, fl)

def parseVal(line, sourceDir):
    # print (line)
    tokens = line.split(":")
    dloc = re.search('{(.*)}', line).group(1)
    (ln, fl) = parseDloc(dloc)
    result = subprocess.run(['../grok.sh', sourceDir, fl, ln], stdout = subprocess.PIPE)
    v = Value(tokens[1], ln, fl, result.stdout.decode('utf-8').strip())
    return v

def parseVal2(line, sourceDir):
    tokens = line.split(":")
    valStr = tokens[4]
    dloc = re.search('{(.*)}', line).group(1)
    (ln, fl) = parseDloc(dloc)
    result = subprocess.run(['../grok.sh', sourceDir, fl, ln], stdout = subprocess.PIPE)
    v = Value(valStr, ln, fl, result.stdout.decode('utf-8').strip())
    return v

def process(filename, tgt, sourceDir):
    file = open(filename, 'r')
    lines = file.readlines()
    copys = []
    geps = []
    for i in range(len(lines)): # We want to modify i inside the loop
        line = lines[i]
        if line.startswith("$$"):
            line = line[3:]
            if "Solving copy edge" in line:
                (src, dst) = line.split(":")[1].split("and")
                copy = Copy(src, dst)
                # Src value: ...
                i = i + 1
                srcVal = parseVal(lines[i], sourceDir)
                # Dst value: ...
                i = i + 1
                dstVal = parseVal(lines[i], sourceDir)
                copy.setSrc(srcVal)
                copy.setDst(dstVal)
                # Processing cpoy edge: [PRIMARY] / [DERIVED]
                i = i + 1
                if "DERIVED" in lines[i] and ("NO SOURCE" not in lines[i]):
                    v = parseVal2(lines[i], sourceDir)
                    copy.setOrigVal(v)
                else:
                    copy.setOrigVal(None)
                # PTD
                temp = i + 1
                ptdSet = []
                while "PTD" in lines[temp]:
                    ptdSet.append(lines[temp])
                    temp = temp + 1
                i = temp - 1
                copy.setPtdSet(ptdSet)
                copys.append(copy)
            if "Solving gep edge" in line:
                (src, dst) = line.split(":")[1].split("and")
                gep = Gep(src, dst)
                i = i + 1
                # print("here>>" + lines[i])
                srcVal = parseVal(lines[i], sourceDir)
                i = i + 1
                # print("here2>>" + lines[i])
                dstVal = parseVal(lines[i], sourceDir)
                gep.setSrc(srcVal)
                gep.setDst(dstVal)
                temp = i + 2
                ptdSet = []
                while "PTD" in lines[temp]:
                    ptdSet.append(lines[temp])
                    temp = temp + 1
                i = temp - 1
                gep.setPtdSet(ptdSet)
                geps.append(gep)
    for copy in copys:
        for ptd in copy.ptdSet:
            if tgt in ptd:
                print (copy)

    for gep in geps:
        for ptd in gep.ptdSet:
            if tgt in ptd:
                print (gep)

if __name__ == "__main__":
    process(sys.argv[1], sys.argv[2], sys.argv[3])
