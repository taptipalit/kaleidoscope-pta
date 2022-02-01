import os
import sys

def postProcess(fileName):
    f = open(fileName)
    fout = open(fileName+".post.dot", "w")
    includeNodes = set()
    for line in f:
        if "digraph" in line:
            fout.write(line)
        elif "->" in line:
            if "penwidth=4" in line or "penwidth=5" in line:
                # Get the source and destination
                tokens = line.split()
                src = tokens[0]
                dst = tokens[2].split("[")[0]
                includeNodes.add(src)
                includeNodes.add(dst)
                fout.write(line)
    f.seek(0) # rewind
    for line in f:
        if "shape" in line:
            tokens = line.split()
            if tokens[0] in includeNodes:
                fout.write(line)
    fout.write("}")
    print ("Size of included nodes:"+str(len(includeNodes)))

if __name__ == "__main__":
    postProcess("consCG_final.dot")
