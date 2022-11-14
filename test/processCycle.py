#!/usr/bin/python3

import os
import sys
from datetime import datetime

class Cycle:
    def __init__(self, nodes, isPWC):
        self.nodes = nodes
        self.isPWC = isPWC
        
class Node:
    def __init__(self, fnPtdTup):
        self.fnPtdTup = fnPtdTup

class UniquePtdSetCycle:
    def __init__(self, uniqSet):
        self.uniqSet = uniqSet
       
def parseCycleRecord(cycleRecords):
    beginNodeRecord = False
    cycles = []
    for cycleRecord in cycleRecords:
        nodes = []
        fnPtdList = []
        for record in cycleRecord:
            if "Node Id:" in record:
                if beginNodeRecord:
                    beginNodeRecord = False
                    fnPtdList = []
                else: 
                    beginNodeRecord = True
                    node = Node(tuple(fnPtdList))
                    nodes.append(node)
                    fnPtdList = []
            else:
                if "Cycle ptd" in record:
                    ptd = record.split()[2].strip()
                    fnPtdList.append(ptd)
                elif "###" in record: 
                    # Last record
                    # End the last Node Record
                    # And collect the PWC info
                    beginNodeRecord = False
                    if record.split()[1] == '0':
                        isPWC = False
                        node = Node(tuple(fnPtdList))
                    else:
                        isPWC = True
                        node = Node(tuple(fnPtdList))
                    nodes.append(node)
        cycle = Cycle(nodes, isPWC)
        cycles.append(cycle)
    print("Number of cycle objects: " + str(len(cycles)))

    pwcCycles = [cycle for cycle in cycles if cycle.isPWC == True]
    nonPWCCycles = [cycle for cycle in cycles if cycle.isPWC == False]

    print("Number of PWC cycles: " + str(len(pwcCycles)))
    print("Number of non-SCC cycles: " + str(len(nonPWCCycles)))

    for cycle in cycles:
        ptdSet = set(tuple([node.fnPtdTup for node in cycle.nodes]))
        print ("Cycle has " + str(len(ptdSet)) + " unique sets, and isPWC = " + str(cycle.isPWC))
    return cycles

def process(filename):
    file = open(filename, 'r')
    lines = file.readlines()
    cycleRecords = []
    beginRecord = False
    for line in lines:
        if "Dumping cycle" in line:
            beginRecord = True
            record = []
        elif "####" in line and beginRecord:
            beginRecord = False
            record.append(line)
            cycleRecords.append(record)
        elif beginRecord:
            record.append(line)
    print ("Number of cycle records: " + str(len(cycleRecords)))
    return parseCycleRecord(cycleRecords)

if __name__ == "__main__":
    process(sys.argv[1])
