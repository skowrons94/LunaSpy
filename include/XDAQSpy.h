#ifndef XDAQSPY_H
#define XDAQSPY_H

#include <string>

#include "i2o.h"

class XDAQSpy {

public:

  XDAQSpy( );
  ~XDAQSpy( );

  bool Connect( std::string address, int port );
  void Disconnect( );
  void Close( );

  int  GetNextBuffer( char* buff );

private:

  int ReadAll(int sockfd, char* buff, int sizeToRead);

  int                 port;
  int                 sockfd;
  std::string         address;
  struct sockaddr_in  server;
  
  int fVerbose = 0;
  
};

#endif // XDAQSPY_H
