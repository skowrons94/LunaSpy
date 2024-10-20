#ifndef SPYSERVERRU_H
#define SPYSERVERRU_H

#include <bitset>
#include <string>

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
#include "DataFrame.h"
#include "Utils.h"

class SpyServerRU {

public:
  
  SpyServerRU( std::vector<std::string> names, std::vector<int> channels, std::vector<int> firmware, int nRun, std::string host = "localhost" );
  ~SpyServerRU( );

  void Initialize( );
  void InitializeCalibration( );
  void Reset( );
  void Save( );

  bool Connect( );       // Triggers the connection for XDAQSpy
  void Disconnect( );    // Triggers the disconnection for XDAQSpy

  uint32_t ReadHeader( char* buffer );  // Reads the header in the first buffer

  void UnpackHeader( uint32_t* inpBuffer, uint32_t& aggLength, uint32_t& board, std::bitset<8>& channelMask, uint32_t& offset );
  void UnpackPHA( uint32_t* inpBuffer, uint32_t& board, std::bitset<8>& channelMask, uint32_t& offset );
  void UnpackPSD( uint32_t* inpBuffer, uint32_t& board, std::bitset<8>& channelMask, uint32_t& offset );

  // Functions used by the boost::thread
  void Start( );
  void Stop( );

  void ReadNextEvent( );
  void GetNextEvent( );

  void FillGraphs( uint32_t* inpBuffer, uint32_t board, uint32_t chan, DataFrame dataForm );  // Fills the TGraphs for the waves

  // Public functions for the access from HistoWorker and WaveWorker

  void UpdateWave( );                      // Changes the fWaveUpdate
  void UpdateGraphs( );                    // Updates the style of the TGraphs

  // ROOT Socket
  void rootServer( );

  // Thread variables
  int startCall;
  int stopCall;
  
private:

  std::vector<std::string> fNames; // Container for the board names
  std::vector<int> fChannels; // Container for the channel numbers
  std::vector<int> fFirmware; // Container for the number of samples

  std::vector< std::vector< std::vector<int> > > fEnergy; // Container for the energy histograms
  std::vector< std::vector< std::vector<int> > > fQshort; // Container for the Qshort histograms
  std::vector< std::vector< std::vector<int> > > fQlong; // Container for the Qlong histograms
  std::vector< std::vector< std::vector<int> > > fWave1; // Container for the wave1 histograms
  std::vector< std::vector< std::vector<int> > > fWave2; // Container for the wave1 histograms

  std::vector< std::vector< TH1F* > > fEnergyHist; // Container for the energy histograms
  std::vector< std::vector< TH1F* > > fQshortHist; // Container for the Qshort histograms
  std::vector< std::vector< TH1F* > > fQlongHist; // Container for the Qlong histograms
  std::vector< std::vector< TGraph* > > fWave1Hist; // Container for the wave1 histograms
  std::vector< std::vector< TGraph* > > fWave2Hist; // Container for the wave2 histograms

  std::vector< std::vector< TH1F* > > fWave1HistT; // Container for the wave1 histograms
  std::vector< std::vector< TH1F* > > fWave2HistT; // Container for the wave2 histograms

  std::map<int,DataFrame> fDataFrame; // Container for the data frames
  std::map<int,int> fRO; // Container for Roll Over flags

  XDAQSpy* xdaq; // Connects to the XDAQ spy

  boost::thread* spyThread;

  // Wave thread variables
  uint32_t* waveBuffer;
  boost::thread*  waveThread;
  std::map<int,std::map<int,bool>> fWaveUpdateMap;

  std::string hostname;
  int fVerbose;

  int run;

  // ROOT socket variables
  TSocket* fSocket;
  TMessage* fMessage;
  TServerSocket* serverSocket;
  TSocket* clientSocket;
  boost::thread* rootThread;

  // Calibration
  std::vector< std::vector<float> > fCalibrationA;
  std::vector< std::vector<float> > fCalibrationB;
  
};

#endif // SPYSERVERRU_H
