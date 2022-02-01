import sys

class Edge:
    """
    Class to represent an edge
    """

    def __init__(self, srcId, dstId):
        self.srcId = srcId
        self.dstId = dstId

    def __eq__(self, other):
        if (isinstance(other, Edge)):
            if other.srcId == self.srcId and other.dstId == self.dstId:
                return True
            else:
                return False
        else:
            return False

    def __str__(self):
        return (str(self.srcId) + " --> " + str(self.dstId))

    def __repr__(self):
        return (str(self.srcId) + " --> " + str(self.dstId))

class Cycle:
    """ 
    Class to represent a cycle
    """
    def __init__(self, cycleId, edges):
        self.cycleId = cycleId
        self.edges = edges

    def __str__(self):
        print("{", end = '')
        for edge in self.edges:
            print(edge, end = '')
        print("}")

class Intersection:
    """
    Class to represent an intersection between two cycles
    """
    def __init__(self, srcCycleId, tgtCycleId, edges):
        self.srcCycleId = srcCycleId
        self.tgtCycleId = tgtCycleId
        self.edges = edges 

def processCycles(succBrokenCycles, failBrokenCycles): 
    """
    Analyze the cycles that are processed

    Parameters:
    succBrokenCycles: list of lists, where each inner list represents a cycle that
    was broken.

    failBrokenCycles: list of lists, where each inner list represents a cycle that
    failed to be broken.
    """
    print ("Broke " + str(len(succBrokenCycles)) + " cycles")
    sys.stdout.flush()

    successBrokenCyclesMap = {cycle.cycleId: cycle for cycle in succBrokenCycles} 

    def intersectionLength(intersection):
        return len(intersection.edges)

    for cycleId, cycle in successBrokenCyclesMap.items():
        # For this cycle how many other similar cycles were found
        intersections = []
        for otherCycle in succBrokenCycles:
            if cycle == otherCycle:
                continue
            intersectionEdges = [edge for edge in cycle.edges if edge in otherCycle.edges]
            intersection = Intersection(cycle.cycleId, otherCycle.cycleId, intersectionEdges)
            intersections.append(intersection)

        intersections = sorted(intersections, key=intersectionLength, reverse=True)

        # Print the top intersection
        highestMatch = intersections[0]
        print("For cycle " + cycleId + " of size:" + str(len(cycle.edges)) + " found intersection with cycle " + highestMatch.tgtCycleId + " with maximum edges " + str(len(highestMatch.edges)) + " in another cycle")
        """
        for intersection in intersections[0: 5 if len(intersections) > 5 else len(intersections)]:
            print (intersection)
        """
        sys.stdout.flush()

def parseLog(filename):
    """
    Parse the log and retrieve the cycles broken and not-broken

    Parameters:
    filename: The log file name
    """
    succCycleList = []
    with open(filename) as f:
        lines = f.readlines()
    for line in lines:
        if line.startswith("Successful cycle collapse:"):
            edgeList = []
            cycleEdges = line.split(": ")
            idTokens = cycleEdges[1].strip().split("--")
            cycleId = idTokens[0].strip()
            tokens = idTokens[1].strip().split()
            # pick adjacent pairs
            index = 0
            lim = len(tokens) - 1
            while index < lim:
                #print("index = " + str(index) + " lim = " + str(lim))
                e = Edge(tokens[index], tokens[index+1])
                edgeList.append(e)
                index = index + 2
            c = Cycle(cycleId, edgeList)
            succCycleList.append(c)
    return succCycleList

if __name__ == "__main__":
    if len(sys.argv) == 1:
        e1 = Edge(10,20)
        e2 = Edge(20,30)
        e3 = Edge(30,40)
        c1 = Cycle([e1, e2, e3])
        c2 = Cycle([e1,e2])
        processCycles([c1, c2], [c1])
    else:
        fileName = sys.argv[1]
        succCycleList = parseLog(fileName)
        processCycles(succCycleList, succCycleList)
