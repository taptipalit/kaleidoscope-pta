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

    allSeen = set()
    notSeen = set()
    i = 0
    for cycle in cycles:
        cycleNodePtdSets = set(tuple([(node, node.fnPtdTup) for node in cycle.nodes]))
        if isEmpty(cycleNodePtdSets):
            continue # No need to dump
        i = i + 1
        print (str(i) + ". Cycle has " + str(len(cycleNodePtdSets)) + " unique sets, of sizes " + str(getIndividualSizes(cycleNodePtdSets)) + " and isPWC = " + str(cycle.isPWC))
        seenEarlier = set()
        notSeenEarlierList = []
        ptds = set()
        fullSetSeenCount = 0
        for node, ptdSet in cycleNodePtdSets:
            fullSetSeen = True
            notSeenEarlier = set()
            for ptd in ptdSet:
                ptds.add(ptd)
                if ptd in allSeen:
                    seenEarlier.add(ptd)
                else:
                    fullSetSeen = False
                    notSeenEarlier.add(ptd)
            notSeenEarlierList.append(notSeenEarlier)
            if fullSetSeen:
                fullSetSeenCount = fullSetSeenCount + 1
        print ("All seen earlier: " + str(len(seenEarlier)) + "/" + str(len(ptds)) + " full-set-seen-count: " + str(fullSetSeenCount))
        for ptdSet in notSeenEarlierList:
            if len(ptdSet) > 0:
                print ("\t\t\t >>> SET >> ")
            for ptd in ptdSet:
                print("\t\t\t\t\t >>> " + str(ptd))
                notSeen.add(ptd)
        allSeen.update(ptds)

    print("all seen size: " + str(len(allSeen)))

    notSeenList = [fn for fn in notSeen]
    notSeenList.sort()
    print("Not seen:")
    for fn in notSeenList:
        print(fn)

    return cycles


def isEmpty(cycleNodePtdSet):
    for (node, ptdSet) in cycleNodePtdSet:
        if len(ptdSet) > 0:
            return False
    return True

def getIndividualSizes(cycleNodePtdSets) :
    sizeList = []
    for node, ptd in cycleNodePtdSets:
        sizeList.append(len(ptd))
    return tuple(sizeList)

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
