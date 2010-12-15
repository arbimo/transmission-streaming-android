#!/usr/bin/python2
# -*- coding: utf-8 -*-
# coding:utf8

import re
import sys


if len( sys.argv ) > 1:
    TR = sys.argv[1]
    if len( sys.argv ) > 2:
        inputFile = sys.argv[2]
    else:
        inputFile = "data.log"
else:
    TR = "None"


dataFile = open( inputFile, "r" )

line = dataFile.readline()

blockSize = 16384
startTime = 0
arrival = []
request = []
block = []
bitfields = []

while line:
    d = re.split(' ', line)
    #print len(d)
    if d[1] == "#":
        # ignoring 
        doesNothing = True
    elif TR != "None" and d[2] != TR:
        # Not the good torrent
        doesNothing = True

    # Torrent description
    elif d[3] == "FN":
        startTime = float( d[0] )
        nbPieces = int( d[ len(d) -4 ] )
        pieceLength = int( d[ len(d) - 2] )
        nbBlocks = nbPieces * (pieceLength / blockSize)
        blocksPerPiece = (pieceLength / blockSize)
        print blocksPerPiece
        for i in range(0, nbBlocks):
            request.append(0.0)
            block.append(0.0)
        for i in range(0, nbPieces):
            arrival.append(0.0)
        
    elif len(d) == 13:
        # Send request
        if d[1] == "TR" and d[3] == "S" and d[4] == "R":
            block_num = int(d[7]) * (pieceLength / blockSize) + int(d[9]) / blockSize 
            request[block_num] = float( d[0] ) -startTime
            
    elif len(d) == 11:
        if d[1] == "TR" and d[3] == "R" and d[4] == "P":
            block_num = int(d[7]) * (pieceLength / blockSize) + int(d[9]) / blockSize 
            block[block_num] = float( d[0] )- startTime

    elif len(d) ==9:
        if d[1] == "TR" and d[3] == "R" and d[4] == "H":
            isInList = False
            for peer in bitfields:
                if peer == d[5]:
                    isInList = True

            if not isInList:
                print "Have without bitfield : " + d[5]

            
    elif len(d) == 7:
        
        # Received Piece
        if d[1] == "TR" and d[3] == "R" and d[4] == "PI":
            #print d[5] + " " + str( float(d[0]) - startTime )
            arrival[ int(d[5]) ] = float(d[0]) - startTime

    elif len(d) >= 6:

        if d[1] == "TR" and d[3] == "R" and d[4] == "BF":
            bitfields.append( d[5] )

    line = dataFile.readline()

#for i in range(1, len(arrival)):
    #arrival[i] = max( arrival[i], arrival[i-1] ) 

blockArrival = open("blocks.dat", "w")
for i in range( 70*blocksPerPiece ): #len(block)*3/4):
    blockArrival.write( str(i) + " " + str(block[i]) +"\n" )    
blockArrival.close()

requestSending = open("requests.dat", "w")
for i in range( 70*blocksPerPiece ): #len(block)*3/4
    requestSending.write( str(i) + " " + str(request[i]) +"\n" )
requestSending.close()

pieceArrival = open("piece.dat", "w")
for i in range(len(arrival)):
    pieceArrival.write( str(i) +" "+ str(arrival[i]) +"\n" )
pieceArrival.close()
