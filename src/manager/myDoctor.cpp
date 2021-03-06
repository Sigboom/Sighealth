/*************************************************************************
	> File Name: myDoctor.cpp
	> Author: Doni Daniel
	> Mail: sigboom@163.com
	> Created Time: 四 11/ 7 17:01:56 2019
 ************************************************************************/

#include <boost/regex.hpp>
#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::string;
using boost::regex;
using boost::regex_match;
using std::stringstream;

class myDoctor:public doctor {
public:
    void execute(string sym) {
        if (sym == "myDoctor") cout << "execute myDoctor!" << endl;
        else getNextDoctor()->execute(sym);
        return ;
    }
};

class netDoctor: public doctor {
public:
    void execute(string sym) {
        /*
        sy.checknet();    
        int len = sendMsg(server.connfd, sy.getSym());
        if (len <= 0) return;
        return ;
        */
        if (sym == "net") cout << "execute netDoctor" << endl;
        else getNextDoctor()->execute(sym);
        return ;
    }
};

class fileDoctor: public doctor {
private:
    shared_ptr<mediCentre> mc;
    string logPath;

public:
    fileDoctor(shared_ptr<mediCentre> mc): mc(mc) {
        logPath = mc->getLogPath();
    }
    
    void execute(string sym) {
        cout << "sym:" << sym << endl;
        int id = stoi(sym);
        string sendBuffer = mc->getSendBuffer(id);
        string recvBuffer = mc->getRecvBuffer(id);
        regex sendExpr("^[0-9]*$"), recvExpr("^ans_[0-9]*");
        if (regex_match(sendBuffer, sendExpr)) {
            int connfd = mc->getConnfd(id);
            mc->sendMsg(connfd, sendBuffer);
        } 
        if (regex_match(recvBuffer, recvExpr)) { 
            string temp = recvBuffer.substr(7);
            cout << "port(String): " << temp << endl;
            int filePort = stoi(temp);
            string filePath = logPath + mc->getHostName(id) + "/";
            cout << "log filePath: " << filePath << endl;
            mc->recvFile(filePort, mc->getHostIp(id), filePath);
        } else {
            getNextDoctor()->execute(sym);
        }
        return ;
    }
};

class serverDoctor: public doctor {
private:
    list<shared_ptr<doctor> > dList;     
    shared_ptr<mediCentre> mc;

public:
    serverDoctor(shared_ptr<mediCentre> mc): mc(mc) {
        dList.push_front(make_shared<fileDoctor>(mc)); 
    }

    void addDoctor(shared_ptr<doctor> newDoctor) {
        newDoctor->setNextDoctor(dList.front());
        dList.push_front(newDoctor);
        return ;
    }
    
    void execute(string sym) {
        int pos = 0;
        
        //send or recv msg or file
        if ((pos = sym.find(":")) != string::npos) {
            cout << "exec serverDoctor" << endl;
            string server = sym.substr(0, pos - 1);
            bool send = (sym.substr(pos - 1, pos) == "<");
            bool recv = (sym.substr(pos - 1, pos) == ">");
            string msg = sym.substr(pos + 1);
            trim(server);
            trim(msg);
            try {
                int id = mc->catchStation(server);
                if (send) mc->setSendBuffer(id, msg); 
                if (recv) mc->setRecvBuffer(id, msg);
                stringstream ssTemp;
                ssTemp << id;
                dList.front()->execute(ssTemp.str()); 
            } catch (serverException e) {
                e.show();
            }
            return ;
        }
        
        //long connect
        if (sym.find("connect ", 0, 8) != string::npos) {
            string station = sym.substr(8);
            connect(station);
            return ;
        }
        getNextDoctor()->execute(sym);
        return ;
    }

private:
    void connect(string station) {
        //cout << "sigDoctor catch sym station: " << station << endl;
        if (station == "all") {
            //cout << "all OK!" << endl;
            int counter = mc->getCounter();
            for (int i = 0; i < counter; ++i) {
                string hostIp = mc->getHostIp(i);
                int port = mc->getPort(i);
                string checkMsg = "SYN";
                int connfd = -1;

                if ((connfd = mc->socket_connect(port, hostIp.c_str())) == -1) {
                    cout << "try" << i + 1 << endl;
                    mc->setErrBuffer(i, "connect_server");
                    mc->setState(i, -1);
                    continue;
                }
                //cout << "ready for send " << checkMsg << endl;
                int n = 0;
                if ((n = mc->sendMsg(connfd, checkMsg)) < 0) cout << "send < 0" << endl;
                cout << "send over " << n << endl;
                mc->setConnfd(i, connfd);
                mc->setState(i, 1);
            }
        }
    }
};

