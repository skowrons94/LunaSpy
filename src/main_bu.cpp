#include <iostream>
#include <vector>
#include <string>

#include <unistd.h>

#include "SpyServerBU.h"

int main(int argc, char* argv[]) {

    // Check the arguments
    // These should be "-d" that follows digitizer name, firmware and number of channels and "-n" that follows the run number
    if( argc < 2 ){
        std::cout << "Usage: " << argv[0] << " -d <digitizer name> <firmware> <channels> -n <run number>" << std::endl;
        return 1;
    }

    std::vector<std::string> names;
    std::vector<int> channels;
    std::vector<int> firmware;

    int run = 0;

    for( int i = 1; i < argc; i++ ){
        if( strcmp( argv[i], "-d" ) == 0 ){
            names.push_back( argv[i+1] );
            firmware.push_back( std::stoi( argv[i+2] ) );
            channels.push_back( std::stoi( argv[i+3] ) );
        }
        else if( strcmp( argv[i], "-n" ) == 0 ){
            run = std::stoi( argv[i+1] );
        }
    }

    SpyServerBU* serverBU = new SpyServerBU(names, channels, run, "localhost");

    serverBU->Start( );

    while( serverBU->startCall ){
        sleep(1);
        serverBU->Save( );
    }

    delete serverBU;

    return 0;
}