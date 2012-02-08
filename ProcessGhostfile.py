#!/usr/bin/python2
"""
Geosmash Ghost script processing
    Goal: pare down the amount of data output by GhostAiRecorder
    Only considers myself as player 0
    
    algorithm:
        list start frames
        list end frames

    Look through the entire file
        if I changed frames from a valid start frame
            print both frames
            print the transition

    output:
        Player0 state
        Player1 state
        Action [prevFrame -> newFrame]

further work:
    - Be more intelligent about what constitutes an event or an 'action'


"""
import sys


if __name__ == "__main__":
 
    f = open(sys.argv[1])
    lines = [i.strip() for i in f.readlines()]

    # Read the frames that actions can begin from
    actionf = open("trainingdata/actionframes.txt")
    startFrames = [i.strip() for i in actionf.readlines()]

    i = 0
    prevl0 = ""
    prevl1 = ""
    prev0 = ""
    prev1 = ""
    while i < len(lines):
        l0 = lines[i]
        curr0 = l0[l0.find("FName:"):].split()[1]
        i += 1

        l1 = lines[i]
        curr1 = l1[l1.find("FName:"):].split()[1]
        i += 1

        # Frames we don't care about transitioning into
        ignoreFrames = ["AirStunned", "GroundNormal", "Grabbed"]

        #if (curr0 != prev0 or curr1 != prev1):
        if (curr0 != prev0 and (prev0 in startFrames) and curr0 not in ignoreFrames):
            print prevl0
            print prevl1
            print prev0, " -> ", curr0
            print

        prevl0 = l0
        prevl1 = l1
        prev0 = curr0
        prev1 = curr1
