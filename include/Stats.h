#ifndef STATS_H
#define STATS_H

#include <thread>
#include <string>
#include <map>
#include <ctime>
#include <chrono>
#include <thread>
#include <string.h>
#include <iostream>
#include <unistd.h> 
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>   

class Stats
{
public:
    
    Stats();
    ~Stats( ) ;  

    void Thread( void );
    void setName(std::string name) { board = name;}
    void setHost(std::string name, int port){
        hostName = name;
        hostPort = port;
        init();
    };
    int init();
    
    struct rates{
        uint32_t lostEvents;
        uint32_t pileEvents;
        uint32_t satuEvents;
        uint32_t totalEvents;

        float    lostRate;
        float    pileRate;
        float    satuRate;
        float    totalRate;
        unsigned long long time;
        unsigned long long timePrev;
        void Init(){
            lostEvents   = 0;
            pileEvents   = 0;
            satuEvents   = 0;
            totalEvents  = 0;
        }
    };

    std::map<int,rates> values;

private:
    
    int maxCh;
    int sockCarbon;
    std::string hostName;
    int hostPort;
    bool stopCall = 0;
    std::time_t epochTime;
    std::thread *graphiteThread;
    std::string host;
    std::string board;
    std::string lost;
    std::string total;
    std::string satu;
    std::string pile;
    std::string dead;

};

#endif
