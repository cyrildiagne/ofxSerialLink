//
//
//  ofxSerialLink.cpp
//
//  Created by Cyril Diagne on 15/05/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "ofxSerialLink.h"

#include <iterator>

SerialLink::SerialLink()
{
    buffer = new unsigned char [BUFF_lENGTH];
    bLock = false;
    
    bCommandStarted = false;
    bLogStarted = false;
    bValUpdateStarted = false;
    bAcknowledged = false;
    
    bLog = false;
    
    cmdTimeToLive = 200;
    
    lastCmdSentTime = 0;
}

SerialLink::~SerialLink()
{
    delete buffer;
}

bool SerialLink::setup(string ser)
{
    currLine = "";
    serial.listDevices();
    return serial.setup(ser, 9600);
}

void SerialLink::lock()
{
    bLock = true;
}

void SerialLink::unlock()
{
    bLock = false;
}

void SerialLink::update()
{
    // first we listen to what the arduino is saying
    if(serial.available())  {
        
        // read chunks of 4 bytes
        while( serial.available() > BUFF_lENGTH ){
            serial.readBytes( buffer, BUFF_lENGTH);
            process(buffer, BUFF_lENGTH);
        };
        
        // read remaining bytes
        int rmb = serial.available();
        if(rmb<=0) return;
        
        unsigned char* tmpBuff = new unsigned char [rmb];
        serial.readBytes( tmpBuff, rmb);
        
        process(tmpBuff, rmb);
        
        delete tmpBuff;
    }
    
    // then we send 1 command at a time
    if(cmdBuffer.size() && !bLock) {
        
        lock();
        
        string cmd = cmdBuffer[0].command +" "+ cmdBuffer[0].arguments;
        
        if(bLog) ofLog() << "> " << cmd;
        
        cmd += "\n";
        std::vector<unsigned char> tempV(cmd.begin(), cmd.end());
        serial.writeBytes(&tempV[0], tempV.size());
        lastCmdSentTime = ofGetElapsedTimeMillis();
    }
    else if(bLock && lastCmdSentTime - ofGetElapsedTimeMillis() > cmdTimeToLive) {
        unlock();
    }
}

void SerialLink::process(unsigned char * buffer, int num)
{
    unsigned char c;
    for (int i=0; i<num; i++)
    {
        c = buffer[i];
        
        if(currLine.compare("")==0) {
            
            // start acknowledge
            if(c=='$') {
                unlock();
                currLine += c;
                bAcknowledged = true;
                if(cmdBuffer.size()) cmdBuffer.erase(cmdBuffer.begin(), cmdBuffer.begin()+1);
            }
            // start log
            else if(c=='#') {
                bLogStarted = true;
                currLine += c;
            }
            // start value report
            else if(c=='@') {
                bValUpdateStarted = true;
                currLine += c;
            }
            else if(c!='\n' && c!='\r') {
                bCommandStarted = true;
                currLine += c;
            }
        }
        // end of command
        else if(c=='\n' || c=='\r') {
            
            if(bAcknowledged) {
                if(bLog) ofLog() << ">> Serial Link : OK.";
                bAcknowledged = false;
            }
            else if(bLogStarted) {
                if(bLog) ofLog() << "Serial Link LOG : " + currLine;
                bLogStarted = false;
            }
            else if(bValUpdateStarted) {
                if(bLog) ofLog() << "Serial Link VAL UPDATED : " + currLine;
                handleValUpdatedFromSerial(currLine);
                bValUpdateStarted = false;
            }
            else if(bCommandStarted) {
                if(bLog) ofLog() << "<< Serial Link CMD: " + currLine;
                handleCommandFromSerial(currLine);
                bCommandStarted = false;
            } else {
                ofLog(OF_LOG_WARNING) << "Serial Link LINE SKIPPED (?) : " << currLine;
            }
            currLine = "";
        }
        else {
            currLine += c;
        }
    }
}

void SerialLink::resetCommandQueue() {
    unlock();
    bAcknowledged = true;
    currLine = "";
    cmdBuffer.clear();
}

void SerialLink::handleCommandFromSerial(string commandLine)
{
    if(commandLine.compare("READY")==0) {
        SerialLinkEventArgs evArgs;
        evArgs.event = SERIALLINK_READY;
        evArgs.args = getCommandAsVector(commandLine);
        ofNotifyEvent(SerialLinkEvent, evArgs);
    }
}

void SerialLink::handleValUpdatedFromSerial(string commandLine)
{
    SerialLinkEventArgs evArgs;
    evArgs.event = VAL_CHANGED;
    evArgs.args = getCommandAsVector(commandLine);
    
    // remove the "@" from the first argument
    evArgs.args[0].erase(evArgs.args[0].begin(),evArgs.args[0].begin()+1);
    
    ofNotifyEvent(SerialLinkEvent, evArgs);
}

void SerialLink::addCommand(string command, string args)
{
    // Check if we already have a that command in the stack
    bool bPrevCommandUpdated = false;
    for (int i=0; i<cmdBuffer.size(); i++) {
        
        // update the params of the previously called command with new args
        if(command.compare(cmdBuffer[i].command)==0) {
            cmdBuffer[i].arguments = args;
            bPrevCommandUpdated = true;
        }
    }
    
    if(!bPrevCommandUpdated) cmdBuffer.push_back((SerialLinkCommand){command, args});
}

void SerialLink::sendCommand(string command, string args)
{
    string cmd = cmdBuffer[0].command +" "+ cmdBuffer[0].arguments + "\n";
    std::vector<unsigned char> tempV(cmd.begin(), cmd.end());
    serial.writeBytes(&tempV[0], tempV.size());
}

vector<string> SerialLink::getCommandAsVector(string cmdLine)
{
    istringstream iss(cmdLine);
    vector<string> tokens;
    do {
        string sub;
        iss >> sub;
        tokens.push_back(sub);
    } while (iss);
    
    return tokens;
}