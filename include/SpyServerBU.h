#ifndef SpyServerBU_H
#define SpyServerBU_H

#include <bitset>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <TObject.h>
#include <TSocket.h>
#include <TServerSocket.h>
#include <TMessage.h>
#include <TH1F.h>
#include <TGraph.h>

#include <boost/thread.hpp>

#include "XDAQSpy.h"

#include "DataFrames.h"
#include "Utils.h"

// This class has double functionality:
// 1) It connect to XDAQSpy to get the data from current acquistion
//    and store them so WaveWorker and HistoWorker can retrieve it.
// 2) It creates an another socket so the data can be obtained from
//    other PCs (this part is still not fully implemented).
// The class creates a new thread in order to obtain and unpack the buffer.
// The connection with the XDAQ spy is handled by XDAQSpy class!

class SpyServerBU {

public:
  
  SpyServerBU( std::vector<std::string> names, std::vector<int> channels, int nRun, std::string host = "localhost" );
  ~SpyServerBU( );

  void InitializeROOT( );

  bool Connect( );                                         // Triggers the connection for XDAQSpy
  void Disconnect( );                                      // Triggers the disconnection for XDAQSpy

  void Reset( );                                           // Resets the histograms
  void Save( );                                            // Saves the histograms

  void UnpackPHA( char* buffer, uint32_t& offset, uint32_t& boardId, uint32_t& chan, uint32_t& evtNum );
  void UnpackPSD( char* buffer, uint32_t& offset, uint32_t& boardId, uint32_t& chan, uint32_t& evtNum );

  // Functions used by the boost::thread
  
  void Start( );
  void Stop( );

  void GetNextEvent( );

  void rootServer( );

  int startCall         = 0;
  int stopCall          = 1;
  
private:

  std::vector<std::string> fNames; // Container for the board names
  std::vector<int> fChannels; // Container for the channel numbers

  // Structures to read the data
  
  dataKey            fKey;
  subDataPSD_t   fDataPSD;
  subDataPHA_t   fDataPHA;
  dataMergedKey fMergeKey;

  std::vector< std::vector< TH1F* > >   fEnergy;         // Container for histograms
  std::vector< std::vector< TH1F* > >  fQshort;         // Container for histograms
  std::vector< std::vector< TH1F* > > fQlong;          // Container for histograms

  std::vector< TH1F* >   fEnergySum;      // Container for histograms
  std::vector< TH1F* >   fQshortSum;      // Container for histograms
  std::vector< TH1F* >   fQlongSum;       // Container for histograms

  std::vector< TH1F* >   fEnergySum2;     // Container for histograms
  std::vector< TH1F* >   fQshortSum2;     // Container for histograms
  std::vector< TH1F* >   fQlongSum2;      // Container for histograms

  std::vector< TH1F* >   fEnergySum3;     // Container for histograms
  std::vector< TH1F* >   fQshortSum3;     // Container for histograms
  std::vector< TH1F* >   fQlongSum3;      // Container for histograms

  std::vector< TH1F* >   fEnergySum4;     // Container for histograms
  std::vector< TH1F* >   fQshortSum4;     // Container for histograms
  std::vector< TH1F* >   fQlongSum4;      // Container for histograms

  std::vector< std::vector< TH1F* > >   fEnergyAnti;     // Container for histograms
  std::vector< std::vector< TH1F* > >   fQshortAnti;     // Container for histograms
  std::vector< std::vector< TH1F* > >   fQlongAnti;      // Container for histograms

  std::vector<uint16_t> fEnergyBuffer;                // Buffer to store the data
  std::vector<uint16_t> fQshortBuffer;                // Buffer to store the data
  std::vector<uint16_t> fQlongBuffer;                 // Buffer to store the data

  uint16_t energy;
  uint16_t qshort;
  uint16_t qlong;

  // Connects to the XDAQ spy
  
  XDAQSpy* xdaq;

  // Thread variables
  boost::thread*  spyThread;
  
  int fVerbose   = 0;
  
  std::string hostname;
  int run;

  // ROOT socket variables
  TSocket* fSocket;
  TMessage* fMessage;
  TServerSocket* serverSocket;
  TSocket* clientSocket;
  boost::thread* rootThread;
  
};

#endif // SpyServerBU_H
