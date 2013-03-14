//
//  ofxSerialLink.h
//
//  Created by Cyril Diagne on 15/05/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef mirroirDeFonds_Simulation_SerialLink_h
#define mirroirDeFonds_Simulation_SerialLink_h

#include "ofMain.h"

#define BUFF_lENGTH 4

enum {
    SERIALLINK_READY,
    VAL_CHANGED
};

struct SerialLinkCommand {
    string command;
    string arguments;
};

class SerialLinkEventArgs : public ofEventArgs {
public:
    int event;
    vector<string> args;
};

class SerialLink {
    
public:
    
    SerialLink();
    ~SerialLink();
    
    bool setup(string serial);
    void update();
    void addCommand(string command, string args="");
    void sendCommand(string command, string args="");
    void resetCommandQueue();
    
    ofEvent<SerialLinkEventArgs> SerialLinkEvent;
    const vector<SerialLinkCommand>& getCmdBuffer() { return cmdBuffer;}
    
    bool bLog;
    int cmdTimeToLive;
    
protected:
    
    void lock();
    void unlock();
    
    void process(unsigned char * buffer, int num);
    void handleCommandFromSerial(string commandLine);
    void handleValUpdatedFromSerial(string commandLine);
    
    vector<string> getCommandAsVector(string cmdLine);
    
    unsigned char * buffer;
    vector<SerialLinkCommand> cmdBuffer;
    
    ofSerial serial;
    
    bool bLock;
    
    bool bCommandStarted;
    bool bLogStarted;
    bool bValUpdateStarted;
    bool bAcknowledged;
    int lastCmdSentTime;
    string currLine;
};

#endif