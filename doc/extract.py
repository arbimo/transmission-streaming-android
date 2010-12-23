#!/usr/bin/python2
# -*- coding: utf-8 -*-
# coding:utf8

import re
import sys
import os

instru_dir = os.path.expanduser( "~/.config/transmission-daemon/instrumentation" )

def lastInstruFile():
    file_list = [ f for f in os.listdir( instru_dir )
                         if os.path.isfile( os.path.join( instru_dir, f ) ) ]
    file_list.sort()
    return instru_dir + "/" + file_list[-1]

class torrent:

    def __init__(self):
        self.blockSize = 16384
        self.requests = []
        self.requests2 = []
        self.requests3 = []
        self.requests4 = []
        self.blocks = []
        self.pieces = []
        self.rates = []
        
        self.numFirstRequests = 0.0
        self.numOtherRequests = 0.0

    def init( self, startTime, numPieces, pieceLength ):
        self.startTime = float( startTime )
        self.numPieces = numPieces 
        self.pieceLength = pieceLength
        self.numBlocks = numPieces * (pieceLength / blockSize)
        self.blocksPerPiece = (pieceLength / blockSize)
        for i in range(0, self.numBlocks):
            self.requests.append(0.0)
            self.requests2.append(0.0)
            self.requests3.append(0.0)
            self.requests4.append(0.0)
            self.blocks.append(0.0)
        for i in range(0, self.numPieces):
            self.pieces.append(0.0)

    def setId( self, id ):
        self.id = str( id )

    def setName( self, name ):
        self.name = name

    def addRequestSend( self, index, time ):
        if( self.requests[index] == 0 ):
            self.requests[index] = float( time ) - self.startTime
            self.numFirstRequests += 1.0
        else:
            self.numOtherRequests += 1.0
            if self.requests2[index] == 0:
                self.requests2[index] = float( time ) - self.startTime
            elif self.requests3[index] == 0:
                self.requests3[index] = float( time ) - self.startTime
            elif self.requests4[index] == 0:
                self.requests4[index] = float( time ) - self.startTime

    def addPieceReceived( self, index, time ):
        self.pieces[index] = float( time ) - self.startTime
        numgot = 0
        numorder = 0
        for i in range( len( self.pieces ) ):
            if( self.pieces[i] > 0 ):
                numgot += 1
                if( i == numorder + 1 ):
                    numorder += 1
        self.rates.append( [ float( time ) - self.startTime , numorder , numgot ] )

    def addBlockReceived( self, index, time ):
        self.blocks[index] = float( time ) - self.startTime

    def printStats( self ):
        print "Torrent (" +self.id+ ") name : " + self.name
        print "Pieces number : " + str( self.numPieces )
        print "Piece length : " + str( self.pieceLength )
        print "Blocks per piece : " + str( self.blocksPerPiece )
        if( self.numOtherRequests + self.numFirstRequests > 0 ):
            print "Re-requests : " + str( 100 * self.numOtherRequests 
                / ( self.numOtherRequests + self.numFirstRequests ) ) + "%"

tor = torrent()

# Parse arguments
if len( sys.argv ) > 1:
    tor.setId(  sys.argv[1] )
    if len( sys.argv ) > 2:
        if sys.argv[2] == "last":
            inputFile = lastInstruFile()
        else:
            inputFile = sys.argv[2]
    else:
        inputFile = "data.log"
else:
    tor.setId( "None" )


dataFile = open( inputFile, "r" )

line = dataFile.readline()

blockSize = 16384
startTime = 0

while line:
    d = re.split(' ', line)
    if d[1] == "#":
        # ignoring 
        doesNothing = True
    elif tor.id != "None" and d[2] != tor.id:
        # Not the good torrent
        doesNothing = True

    # Torrent description
    elif d[3] == "FN":
        startTime = float( d[0] )
        nbPieces = int( d[ len(d) -4 ] )
        pieceLength = int( d[ len(d) - 2] )
        name = ""
        tor.init( startTime, nbPieces, pieceLength )
        for i in range( 4, len(d) -5 ):
            name += d[i] + " "
        tor.setName( name )

        
    elif len(d) == 13:
        # Send request
        if d[1] == "TR" and d[3] == "S" and d[4] == "R":
            blockNum = int(d[7]) * (pieceLength / blockSize) + int(d[9]) / blockSize 
            tor.addRequestSend( blockNum, d[0] )
    elif len(d) == 11:
      
	    # Receive block
        if d[1] == "TR" and d[3] == "R" and d[4] == "P":
            blockNum = int(d[7]) * (pieceLength / blockSize) + int(d[9]) / blockSize 
            tor.addBlockReceived( blockNum, d[0] )


    elif len(d) ==9:

	    # Receive Have
        if d[1] == "TR" and d[3] == "R" and d[4] == "H":
            doesNothing = True

    elif len(d) == 7:
        
        # Receive Piece
        if d[1] == "TR" and d[3] == "R" and d[4] == "PI":
            tor.addPieceReceived( int( d[5] ), d[0] )

    elif len(d) >= 6:

	    # Receive bitfield
        if d[1] == "TR" and d[3] == "R" and d[4] == "BF":
            doesNothing = True

    line = dataFile.readline()

#for i in range(1, len(arrival)):
#    arrival[i] = max( arrival[i], arrival[i-1] ) 
    
#for i in range(len(block)):
  #block[i] = block[i] - request[i]

blocksRange = len( tor.blocks )

blockArrival = open("blocks.dat", "w")
for i in range( blocksRange ):
    blockArrival.write( str(i) + " " + str(tor.blocks[i]) +"\n" )    
blockArrival.close()

requestSending = open("requests.dat", "w")
for i in range( blocksRange ):
    requestSending.write( str(i) + " " + str(tor.requests[i]) +"\n" )
requestSending.close()

requestSending = open("requests2.dat", "w")
for i in range( blocksRange ):
    requestSending.write( str(i) + " " + str(tor.requests2[i]) +"\n" )
requestSending.close()

requestSending = open("requests3.dat", "w")
for i in range( blocksRange ):
    requestSending.write( str(i) + " " + str(tor.requests3[i]) +"\n" )
requestSending.close()

requestSending = open("requests4.dat", "w")
for i in range( blocksRange ):
    requestSending.write( str(i) + " " + str(tor.requests4[i]) +"\n" )
requestSending.close()

pieceArrival = open("pieces.dat", "w")
for i in range(len( tor.pieces )):
    pieceArrival.write( str(i) +" "+ str(tor.pieces[i]) +"\n" )
pieceArrival.close()

ratesFile = open("rates.dat", "w")
for i in range(len( tor.rates)):
    ratesFile.write( str( tor.rates[i][0] ) +" "+ str( tor.rates[i][1] ) +"\n" )
ratesFile.close()

tor.printStats()

