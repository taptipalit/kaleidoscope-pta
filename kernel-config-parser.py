import os
import subprocess
import sys
import logging


dependencyMapMap = {} # config --> { dependent-config1: val, dependent-config2: val ...}
revDependencyMap = {} # required-config (T) --> {config1, config2, ...}

currSetOptions = []

def extractDependency(line):
    tokens = line.split()
    depMap = {}
    for token in tokens:
        if token == "depends":
            continue
        if token == "&&" or token == "||" or token == "on":
            continue
        if token[0] == "!":
            depConfig = token[1:]
            depMap[depConfig] = False
        else:
            depConfig = token
            depMap[depConfig] = True
    logging.debug(depMap)
    return depMap

def parseKConfig(path):
    f = open(path)
    currConfig = ""
    for line in f:
        if len(line.strip()) == 0:
            currConfig = ""
        elif line.startswith("config") and "CHOICE" not in line:
            # starting a new config entry
            tokens = line.split()
            if len(tokens) > 1:
                currConfig = line.split()[1]
        elif "depends" in line:
            if len(currConfig) > 0 and len(currConfig) > 2:
                if ">" not in line and "<" not in line and "=" not in line:
                    dependencyMap = extractDependency(line)
                    dependencyMapMap[currConfig] = dependencyMap

def buildRevMap():
    for config in dependencyMapMap:
        for depConfig in dependencyMapMap[config]:
            if depConfig not in revDependencyMap:
                revDependencyMap[depConfig] = []
            if dependencyMapMap[config][depConfig] == True: 
               revDependencyMap[depConfig].append(config)
            #TODO: handle false

def findTransitiveDepCount(depConfig):
    count = len(revDependencyMap[depConfig]);
    workList = []
    workList.append(depConfig)
    while len(workList) > 0:
        work = workList.pop()
        if work not in revDependencyMap:
            count = count + 1
        else:
            count = count + len(revDependencyMap[work]);
            for subWork in revDependencyMap[work]:
                workList.append(subWork)
    return count


def parseCurrentConfig(filename):
    f = open(filename)
    for line in f:
        if len(line) == 0 or line.startswith("#"):
            continue
        tokens = line.split("=")
        if len(tokens) == 2:
            confOption = tokens[0].strip()[7:]
            #print(confOption)
            val = tokens[1].strip()
            if val == "y":
                currSetOptions.append(confOption)


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    parseKConfig("all.kconfig")
    buildRevMap();
    for config in dependencyMapMap:
        logging.debug('%s depends on %s', config, dependencyMapMap[config])

    sortedList = []
    for depConfig in revDependencyMap:
        depCount = findTransitiveDepCount(depConfig)
        logging.debug("Dep count found %d", depCount);
        sortedList.append((depConfig, depCount))
        logging.debug('%s decides %s', depConfig, revDependencyMap[depConfig])

    sortedList.sort(key = lambda x:x[1], reverse=True)
    top100 = []
    i = 0
    for sortedItem in sortedList:
        logging.debug("--- dependency --> %s", sortedItem)
        if i < 100:
            top100.append(sortedItem[0])
            i = i+1

    parseCurrentConfig(".config")
    logging.debug("Currently set options: %s", currSetOptions)
    candidates = []
    for conf in top100:
        #print(conf)
        if conf not in currSetOptions:
            candidates.append(conf)

    logging.info("Total simple config options (y/n): %d", len(sortedList))
    logging.info("Total config options set to y: %d", len(currSetOptions))
    logging.info("From top-100, candidates: %d", len(candidates))


    os.system("cp .config .config.bkup")
    count = 0
    for candidate in candidates:
        print(candidate)
        #os.system("./scripts/config --enable "+candidate)
        #os.system("cp .config .config."+str(count))

#
#    for curOption in currSetOptions:
#        print("Curr option: ", curOption)
#    for top in top100:
#        print("Top: ", top)
#
