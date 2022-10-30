#!/usr/bin/python3

import os
import sys
import subprocess
import re

# p = precise (context and flow sensitive
# k = non-precise (but with kaleidoscope)

def process(filename1, filename2):
    pFile = open(filename1, 'r')
    kFile = open(filename2, 'r')

    pLines = pFile.readlines()
    kLines = kFile.readlines()

    pMap = {}
    kMap = {}

    for pLine in pLines:
        if "DUMPING IND CALL" in pLine:
            tokens = pLine.split(":")
            caller = tokens[1]
            callees = tokens[2].split(",")
            calleeSet = set()
            for callee in callees:
                calleeSet.add(callee.strip())
            pMap[caller] = calleeSet

    for kLine in kLines:
        if "DUMPING IND CALL" in kLine:
            tokens = kLine.split(":")
            caller = tokens[1]
            callees = tokens[2].split(",")
            calleeSet = set()
            for callee in callees:
                calleeSet.add(callee.strip())
            kMap[caller] = calleeSet

    for caller in pMap:
        pList = pMap[caller]
        if caller in kMap:
            kList = kMap[caller]

            """
    
            print("Printing the lists")
            print (pList)
            print (kList)
            """
    
            diff1 = set()
            for callee in pList:
                if callee not in kList:
                    diff1.add(callee)
    
            diff2 = set()
            for callee in kList:
                if callee not in pList:
                    diff2.add(callee)
            print("Printing the differences for function" + caller)
            print("precise - kaleidoscope = " + str(diff1))
            print("kaleidoscope - precise = " + str(diff2))

if __name__ == "__main__":
    process(sys.argv[1], sys.argv[2])
