#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <sstream>
#include <stdio.h>

#include "SpyServerBU.h"

SpyServerBU::SpyServerBU( std::vector<std::string> names, std::vector<int> channels, int nRun, std::string host ) : run(nRun), hostname(host)
{

  fNames = names;
  fChannels = channels;

  // Initializing the histograms  
  InitializeROOT( );

  // Initializing XDAQSpy
  xdaq = new XDAQSpy( );  
}

SpyServerBU::~SpyServerBU(){

  // Clean up

  if( startCall ){ 
    startCall = 0;
    stopCall  = 0;
    spyThread->join();
    rootThread->join();
  }
  
  delete xdaq;

}

void SpyServerBU::InitializeROOT( ){

  // Initialize the vector containers for the histograms
  for( int i = 0; i < fChannels.size( ); i++ ){

    fEnergy.push_back( std::vector< TH1F* >( fChannels[i] ) );
    fQshort.push_back( std::vector< TH1F* >( fChannels[i] ) );
    fQlong.push_back( std::vector< TH1F* >( fChannels[i] ) );

    fEnergyAnti.push_back( std::vector< TH1F* >( fChannels[i] ) );
    fQshortAnti.push_back( std::vector< TH1F* >( fChannels[i] ) );
    fQlongAnti.push_back( std::vector< TH1F* >( fChannels[i] ) );

    // Initialize ROOT Objects
    for( int chan = 0; chan < fChannels[i]; chan++ ){
      fEnergyAnti[i][chan] = new TH1F( ( fNames[i] + "_" + std::to_string( i ) + "_EnergyAnti_Channel" + std::to_string( chan ) ).c_str( ), ( fNames[i] + "_" + std::to_string( i ) + "_EnergyAnti_Channel" + std::to_string( chan ) ).c_str( ), 32768, 0, 32768 );
      fQshortAnti[i][chan] = new TH1F( ( fNames[i] + "_" + std::to_string( i ) + "_QshortAnti_Channel" + std::to_string( chan ) ).c_str( ), ( fNames[i] + "_" + std::to_string( i ) + "_QshortAnti_Channel" + std::to_string( chan ) ).c_str( ), 32768, 0, 32768 );
      fQlongAnti[i][chan] = new TH1F( ( fNames[i] + "_" + std::to_string( i ) + "_QlongAnti_Channel" + std::to_string( chan ) ).c_str( ), ( fNames[i] + "_" + std::to_string( i ) + "_QlongAnti_Channel" + std::to_string( chan ) ).c_str( ), 32768, 0, 32768 );
    }

    fEnergySum.push_back( new TH1F( ( fNames[i] + "_EnergySum" ).c_str( ), ( fNames[i] + "_EnergySum" ).c_str( ), 32768, 0, 32768 ) );
    fQshortSum.push_back( new TH1F( ( fNames[i] + "_QshortSum" ).c_str( ), ( fNames[i] + "_QshortSum" ).c_str( ), 32768, 0, 32768 ) );
    fQlongSum.push_back( new TH1F( ( fNames[i] + "_QlongSum" ).c_str( ), ( fNames[i] + "_QlongSum" ).c_str( ), 32768, 0, 32768 ) );

    fEnergySum2.push_back( new TH1F( ( fNames[i] + "_EnergySum2" ).c_str( ), ( fNames[i] + "_EnergySum2" ).c_str( ), 32768, 0, 32768 ) );
    fQshortSum2.push_back( new TH1F( ( fNames[i] + "_QshortSum2" ).c_str( ), ( fNames[i] + "_QshortSum2" ).c_str( ), 32768, 0, 32768 ) );
    fQlongSum2.push_back( new TH1F( ( fNames[i] + "_QlongSum2" ).c_str( ), ( fNames[i] + "_QlongSum2" ).c_str( ), 32768, 0, 32768 ) );

    fEnergySum3.push_back( new TH1F( ( fNames[i] + "_EnergySum3" ).c_str( ), ( fNames[i] + "_EnergySum3" ).c_str( ), 32768, 0, 32768 ) );
    fQshortSum3.push_back( new TH1F( ( fNames[i] + "_QshortSum3" ).c_str( ), ( fNames[i] + "_QshortSum3" ).c_str( ), 32768, 0, 32768 ) );
    fQlongSum3.push_back( new TH1F( ( fNames[i] + "_QlongSum3" ).c_str( ), ( fNames[i] + "_QlongSum3" ).c_str( ), 32768, 0, 32768 ) );

    fEnergySum4.push_back( new TH1F( ( fNames[i] + "_EnergySum4" ).c_str( ), ( fNames[i] + "_EnergySum4" ).c_str( ), 32768, 0, 32768 ) );
    fQshortSum4.push_back( new TH1F( ( fNames[i] + "_QshortSum4" ).c_str( ), ( fNames[i] + "_QshortSum4" ).c_str( ), 32768, 0, 32768 ) );
    fQlongSum4.push_back( new TH1F( ( fNames[i] + "_QlongSum4" ).c_str( ), ( fNames[i] + "_QlongSum4" ).c_str( ), 32768, 0, 32768 ) );
  }

}

void SpyServerBU::InitializeCalibration( ){

  // Read the file in calib/{board_name}_{board_id}.cal file
  // In there columns of a and b parameters for each channel are stored
  // If file does not exists, set a = 0 and b = 1

  std::ifstream file;
  std::string line;
  std::string name;
  std::string board;
  std::string path = "calib/";

  for( int i = 0; i < fNames.size( ); i++ ){
    name = fNames[i];
    board = std::to_string( i );
    file.open( path + name + "_" + board + ".cal" );
    if( file.is_open( ) ){
      for( int chan = 0; chan < fChannels[i]; chan++ ){
        std::getline( file, line );
        std::istringstream iss( line );
        iss >> fCalibrationA[i][chan][0] >> fCalibrationB[i][chan][0];
      }
    }
    else{
      for( int chan = 0; chan < fChannels[i]; chan++ ){
        fCalibrationA[i][chan][0] = 0;
        fCalibrationB[i][chan][0] = 1;
      }
    }
    file.close( );
  }
}

void SpyServerBU::Reset( ){

}

void SpyServerBU::Save( ){

  // Save the histograms to a file
  std::ofstream file;

  for( int i = 0; i < fChannels.size( ); i++ ){

    file.open( "data/run" + std::to_string(run) + "/" + fNames[i] + "_HistoSum.dat" );
    for( int j = 0; j < fEnergySum[i]->GetNbinsX( ); j++ ){
      file << i << fEnergySum[i]->GetBinContent( j ) << std::endl;
    }
    file.close( );

  }

}

bool SpyServerBU::Connect( ){

  // Connects to the XDAQ spy port
  if( xdaq->Connect( hostname, 10003 ) ) return true;
  else return false;

}

void SpyServerBU::Disconnect( ){

  xdaq->Disconnect( );

}

void SpyServerBU::Start( ){

  if( this->Connect( ) ){ 

    startCall = 1;
    stopCall  = 0;
    
    spyThread = new boost::thread( &SpyServerBU::GetNextEvent, this );
    rootThread = new boost::thread( &SpyServerBU::rootServer, this );

  }

}

void SpyServerBU::Stop( ){

  // It is called just before the acquistion stops.

    if( startCall ){

    startCall = 0;
    stopCall  = 1;
    xdaq->Disconnect( );
    spyThread->join();
    xdaq->Close( ); 

  }

}

void SpyServerBU::UnpackPHA( char* buffer, uint32_t& offset, uint32_t& boardId, uint32_t& chan, uint32_t& evtNum ){
  memcpy( &fDataPHA, buffer+offset, sizeof(fDataPHA) );

  fDataPHA.energy = fCalibrationA[boardId][chan][0] + fCalibrationB[boardId][chan][0] * fDataPHA.energy;

  if( evtNum == 1 ) fEnergyAnti[boardId][chan]->Fill( fDataPHA.energy );
  else{
    fEnergyBuffer.push_back( fDataPHA.energy );
  }

  if( fVerbose )
  {
    std::cout << "Energy: " << fDataPHA.energy << std::endl;
  }
}

void SpyServerBU::UnpackPSD( char* buffer, uint32_t& offset, uint32_t& boardId, uint32_t& chan, uint32_t& evtNum ){
  memcpy( &fDataPSD, buffer+offset, sizeof(fDataPSD) );

  if( evtNum == 1 ) fEnergyAnti[boardId][chan]->Fill( fDataPHA.energy );
  else{
    fQlongBuffer.push_back( fDataPSD.qlong );
    fQshortBuffer.push_back( fDataPSD.qshort );
  }
}

void SpyServerBU::GetNextEvent( ){

  // Ask for the next buffer and unpacks it.

  char* buffer = new char [512*512];
  
  uint32_t length, aggSize, evtSize, evtNum, evt, boardId, chan;
  while( startCall ){
    
    length = xdaq->GetNextBuffer( buffer );

    if( fVerbose )
    {
      std::cout << "Buffer Size: "   << length << std::endl;
    }

    uint32_t offset = 0;
    while( offset < length && startCall ){

      memcpy( &fMergeKey, buffer+offset, sizeof( fMergeKey ) );
      
      aggSize = fMergeKey.GetBytes( );
      evtNum  = fMergeKey.GetEvnum( );      
      
      if( fMergeKey.IsIdle( ) ){
        offset += aggSize;
        continue;
      }

      if( fVerbose )
      {
        std::cout << "Aggregate Size: "   << aggSize << std::endl;
        std::cout << "Number of Events: " << evtNum << std::endl;
      }

      fEnergyBuffer.clear( );
      fQshortBuffer.clear( );
      fQlongBuffer.clear( );
      offset += sizeof( fMergeKey );
      for( evt = 0; evt < evtNum; ++evt ){
      
        memcpy( &fKey, buffer+offset, sizeof( fKey ) );

        boardId = fKey.GetBoard( );
        evtSize = fKey.GetBytes( );
        chan    = fKey.GetChannel( ) - 16 * boardId;

        if( fVerbose )
        {
//          std::cout << "Board ID: "   << boardId << std::endl;
//          std::cout << "Event Size: " << evtSize << std::endl;
//          std::cout << "Channel: "    << chan    << std::endl;
        }
      
        offset += sizeof( dataKey );
        if( evtSize == sizeof(subDataPHA_t) + sizeof(dataKey) ){
	        UnpackPHA( buffer, offset, boardId, chan, evtNum );
          offset += sizeof( subDataPHA_t );
          if( evtNum == 2 ){
            energy = 0;
            for( int i = 0; i < fEnergyBuffer.size( ); i++ ){
              energy += fEnergyBuffer[i];
            }
            fEnergySum2[boardId]->Fill( energy );
          }
          if( evtNum == 3 ){
            energy = 0;
            for( int i = 0; i < fEnergyBuffer.size( ); i++ ){
              energy += fEnergyBuffer[i];
            }
            fEnergySum3[boardId]->Fill( energy );
          }
          if( evtNum == 4 ){
            energy = 0;
            for( int i = 0; i < fEnergyBuffer.size( ); i++ ){
              energy += fEnergyBuffer[i];
            }
            fEnergySum4[boardId]->Fill( energy );
          }
          energy = 0;
          for( int i = 0; i < fEnergyBuffer.size( ); i++ ){
            energy += fEnergyBuffer[i];
          }
          fEnergySum[boardId]->Fill( energy );
        }
        if( evtSize == sizeof(subDataPSD_t) + sizeof(dataKey) ){
          UnpackPSD( buffer, offset, boardId, chan, evtNum  );
          offset += sizeof( subDataPSD_t );
          if( evtNum == 2 ){
            qshort = 0;
            qlong = 0;
            for( int i = 0; i < fQshortBuffer.size( ); i++ ){
              qshort += fQshortBuffer[i];
              qlong += fQlongBuffer[i];
            }
            fQshortSum2[boardId]->Fill( qshort );
            fQlongSum2[boardId]->Fill( qlong );
          }
          if( evtNum == 3 ){
            qshort = 0;
            qlong = 0;
            for( int i = 0; i < fQshortBuffer.size( ); i++ ){
              qshort += fQshortBuffer[i];
              qlong += fQlongBuffer[i];
            }
            fQshortSum3[boardId]->Fill( qshort );
            fQlongSum3[boardId]->Fill( qlong );
          }
          if( evtNum == 4 ){
            qshort = 0;
            qlong = 0;
            for( int i = 0; i < fQshortBuffer.size( ); i++ ){
              qshort += fQshortBuffer[i];
              qlong += fQlongBuffer[i];
            }
            fQshortSum4[boardId]->Fill( qshort );
            fQlongSum4[boardId]->Fill( qlong );
          }
          qshort = 0;
          qlong = 0;
          for( int i = 0; i < fQshortBuffer.size( ); i++ ){
            qshort += fQshortBuffer[i];
            qlong += fQlongBuffer[i];
          }
          fQshortSum[boardId]->Fill( qshort );
          fQlongSum[boardId]->Fill( qlong );
        }
    
      }

    }

    bzero(buffer,512*512);

  }

}

void SpyServerBU::rootServer( ){

  // This is a simple ROOT server
  serverSocket = new TServerSocket( 7070, kTRUE );

  while( startCall ){

    clientSocket = serverSocket->Accept( );

    if( clientSocket ){

      // First receive a message, if it is stop, then close the connection
      char str[32];
      clientSocket->Recv(str,32);
      std::string messageType(str);

      if( messageType == "stop"){
        clientSocket->Close( );
        clientSocket = nullptr;
        serverSocket->Close( );
        serverSocket = nullptr;
        startCall = 0;
        xdaq->Disconnect( );
        spyThread->join();
        xdaq->Close( ); 
        break;
      }

      // Send PHA histograms
      std::vector<TH1F*> histograms;
      for( auto it = fEnergyAnti.begin(); it != fEnergyAnti.end(); it++ ){
        for( auto jt = it->begin(); jt != it->end(); jt++ ){
          histograms.push_back( (TH1F*)(*jt) );
        }
      }
      
      for (const auto& hist : histograms) {
          TMessage message(kMESS_OBJECT);
          message.WriteObject(hist);
          clientSocket->Send(message);
        }

      // Send Qshort histograms
      histograms.clear();
      for( auto it = fQshortAnti.begin(); it != fQshortAnti.end(); it++ ){
        for( auto jt = it->begin(); jt != it->end(); jt++ ){
          histograms.push_back( (TH1F*)(*jt) );
        }
      }

      for (const auto& hist : histograms) {
          TMessage message(kMESS_OBJECT);
          message.WriteObject(hist);
          clientSocket->Send(message);
        }

      // Send Qlong histograms
      histograms.clear();
      for( auto it = fQlongAnti.begin(); it != fQlongAnti.end(); it++ ){
        for( auto jt = it->begin(); jt != it->end(); jt++ ){
          histograms.push_back( (TH1F*)(*jt) );
        }
      }

      for (const auto& hist : histograms) {
          TMessage message(kMESS_OBJECT);
          message.WriteObject(hist);
          clientSocket->Send(message);
        }

      // Send Sum histograms
      histograms.clear();
      for( auto it = fEnergySum.begin(); it != fEnergySum.end(); it++ ){
        histograms.push_back( (TH1F*)(*it) );
      }

      for (const auto& hist : histograms) {
          TMessage message(kMESS_OBJECT);
          message.WriteObject(hist);
          clientSocket->Send(message);
        }

    }

    clientSocket->Close();
    clientSocket = nullptr;

  }

}