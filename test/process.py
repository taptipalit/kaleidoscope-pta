#!/usr/bin/python3

import os
import sys
import subprocess
import re
import pickle
from datetime import datetime

LLVM_ONLY=False
C_ONLY=False
LLVM_AND_C=True

#RED='\033[0;31m'
#GREEN='\033[0;32m'
#NC='\033[0m' # No Color

RED=''
NC=''
GREEN=''

class Value:
    def __init__(self, *args):
        if len(args) > 1:
            valName = args[0]
            ln = args[1]
            fl = args[2]
            cSourceLine = args[3]
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
        formatStr = "%s --(" + GREEN + "copy" + NC + ")--> %s { %s }"
        result = formatStr % (self.srcVal, self.dstVal, self.origVal) 
        for ptd in self.ptdSet:
            result = result# + ptd
        return result

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
        formatStr = "%s --(" + GREEN + "gep" + NC + ")--> %s"
        result = formatStr % (self.srcVal, self.dstVal) 
        for ptd in self.ptdSet:
            result = result# + ptd
        return result

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
    if len(tokens) < 2:
        return Value()
    matches = re.search('{(.*)}', line)
    if matches is not None:
        dloc = matches.group(1)
        (ln, fl) = parseDloc(dloc)
        result = subprocess.run(['../grok.sh', sourceDir, fl, ln], stdout = subprocess.PIPE, stderr=subprocess.DEVNULL)
        v = Value(tokens[1], ln, fl, result.stdout.decode('utf-8').strip())
    else:
        ln = "0"
        fl = "0"
        v = Value(tokens[1], ln, fl, "")
    return v

def parseVal2(line, sourceDir):
    tokens = line.split(":")
    if len(tokens) < 2:
        return Value()
    valStr = tokens[4]
    matches = re.search('{(.*)}', line)
    if matches is not None:
        dloc = matches.group(1)
        (ln, fl) = parseDloc(dloc)
        result = subprocess.run(['../grok.sh', sourceDir, fl, ln], stdout = subprocess.PIPE, stderr=subprocess.DEVNULL)
        v = Value(valStr, ln, fl, result.stdout.decode('utf-8').strip())
    else:
        ln = "0"
        fl = "0"
        v = Value(valStr, ln, fl, "")
    return v

def parseVal3(line, sourceDir):
    tokens = line.split(":")
    valStr = tokens[2]
    matches = re.search('{(.*)}', line)
    if matches is not None:
        dloc = re.search('{(.*)}', line).group(1)
        (ln, fl) = parseDloc(dloc)
        result = subprocess.run(['../grok.sh', sourceDir, fl, ln], stdout = subprocess.PIPE, stderr=subprocess.DEVNULL)
        v = Value(valStr, ln, fl, result.stdout.decode('utf-8').strip())
    else:
        ln = "0"
        fl = "0"
        v = Value(valStr, ln, fl, "")
    return v

def process(filename, tgt, sourceDir, outfile, copysPickleFile = None, gepsPickleFile = None):
    oFile = open(outfile, 'w')
    if (copysPickleFile is not None) and (gepsPickleFile is not None):
        with open(copysPickleFile, "rb") as pickleFile:
            copys = pickle.load(pickleFile)
        with open(gepsPickleFile, "rb") as pickleFile:
            geps = pickle.load(pickleFile)
        __process(copys, geps, oFile)
    else:
        print("Just collecting the records ... ")
        file = open(filename, 'r')
        lines = file.readlines()
        records = []
        beginRecord = False
        for line in lines:
            if beginRecord:
                record.append(line)
            if "$$ -----" in line:
                if beginRecord:
                    beginRecord = False
                    records.append(record)
                else:
                    beginRecord = True
                    record = []
        print ("Number of records: " + str(len(records)))
        """
        for record in records:
            print(record)
            print()
        """
        _process(records, tgt, sourceDir, oFile)

def _process(records, tgt, sourceDir, oFile):
    copys = []
    geps = []
    for idx, record in enumerate(records):
        printProgressBar(idx, len(records))
        #print (record[0])
        if "Solving copy edge" in record[0]:
            # Has 4 lines at least
            (src, dst) = record[0].split(":")[1].split("and")
            copy = Copy(src, dst)
            srcVal = parseVal(record[1], sourceDir)
            dstVal = parseVal(record[2], sourceDir)
            copy.setSrc(srcVal)
            copy.setDst(dstVal)
            if "DERIVED" in record[3] and ("NO SOURCE" not in record[3]):
                v = parseVal2(record[3], sourceDir)
                copy.setOrigVal(v)
            else:
                copy.setOrigVal(None)
            if len(record) > 4:
                # Has PTDs
                ptdSet = []
                for line in record[4:]:
                    if "PTD" in line:
                        ptdSet.append(parseVal3(line, sourceDir))
                copy.setPtdSet(ptdSet)
            else:
                copy.setPtdSet([])
            copys.append(copy)
        if "Solving gep edge" in record[0]:
            # Has 4 lines at least
            (src, dst) = record[0].split(":")[1].split("and")
            gep = Gep(src, dst)
            srcVal = parseVal(record[1], sourceDir)
            dstVal = parseVal(record[2], sourceDir)
            gep.setSrc(srcVal)
            gep.setDst(dstVal)
            if len(record) > 4:
                # Has PTDs
                ptdSet = []
                for line in record[4:]:
                    if "PTD" in line:
                        ptdSet.append(parseVal3(line, sourceDir))
                gep.setPtdSet(ptdSet)
            else:
                gep.setPtdSet([])
            geps.append(gep)

    now = datetime.now()
    day = now.strftime("%m_%d_%Y_%m_%h_%s")

    with open(day+'_copys.pickle', 'wb') as pickleFile:
        pickle.dump(copys, pickleFile)

    with open(day+'_geps.pickle', 'wb') as pickleFile:
        pickle.dump(geps, pickleFile)

    __process(copys, geps, tgt, oFile)


def __process(copys, geps, tgt, oFile):
    for copy in copys:
        for ptd in copy.ptdSet:
            if tgt in ptd.valName:
                oFile.write(str(copy) + "\n")
    for gep in geps:
        for ptd in gep.ptdSet:
            if tgt in ptd.valName:
                oFile.write(str(gep) + "\n")

# Print iterations progress
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
	percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
	filledLength = int(length * iteration // total)
	bar = fill * filledLength + '-' * (length - filledLength)
	print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
	# Print New Line on Complete
	if iteration == total: 
		print()

if __name__ == "__main__":
    process(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
