#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <bitset>
#include <sstream>

#include <TList.h>

#include "SpyServerRU.h"

SpyServerRU::SpyServerRU( std::vector<std::string> names, std::vector<int> channels, std::vector<int> firmware, int nRun, std::string host ) : run( nRun ), hostname( host )
{

  startCall = 0;
  stopCall  = 1;
  fVerbose = 0;

  // Creating the DataFrames
  for( int i = 0; i < names.size( ); i++ ){
    fDataFrame[i] = DataFrame( firmware[i], names[i] );
    fDataFrame[i].Build( );
    fRO[i] = 0;
    for( int chan = 0; chan < channels[i]; chan++ ){
      fWaveUpdateMap[i][chan] = true;
    }
  }

  fNames = names;
  fChannels = channels;
  fFirmware = firmware;

  // Initializing the histograms  
  Initialize( );

  // Initialize calibration
  InitializeCalibration( );

  // Initializing XDAQSpy
  xdaq = new XDAQSpy( );

  // Initialize rates
  ratesMonitor.setName("BGO");
  ratesMonitor.setHost("localhost",2003);
  
}

SpyServerRU::~SpyServerRU(){

  // Clean up
  if( startCall ){ 
    startCall = 0;
    stopCall  = 1;
    spyThread->join();
    rootThread->join();
  }

  delete xdaq;
  
}

void SpyServerRU::Initialize( ){

  // Initialize the vector containers for the histograms
  for( int i = 0; i < fChannels.size( ); i++ ){
    fEnergy.push_back( std::vector< std::vector<int> >( fChannels[i], std::vector<int>( 32768, 0 ) ) );
    fQshort.push_back( std::vector< std::vector<int> >( fChannels[i], std::vector<int>( 32768, 0 ) ) );
    fQlong.push_back( std::vector< std::vector<int> >( fChannels[i], std::vector<int>( 32768, 0 ) ) );
    fWave1.push_back( std::vector< std::vector<int> >( fChannels[i], std::vector<int>( 10000, 0 ) ) );
    fWave2.push_back( std::vector< std::vector<int> >( fChannels[i], std::vector<int>( 10000, 0 ) ) );

    fEnergyHist.push_back( std::vector< TH1F* >( fChannels[i] ) );
    fQshortHist.push_back( std::vector< TH1F* >( fChannels[i] ) );
    fQlongHist.push_back( std::vector< TH1F* >( fChannels[i] ) );
    fWave1Hist.push_back( std::vector< TGraph* >( fChannels[i] ) );
    fWave2Hist.push_back( std::vector< TGraph* >( fChannels[i] ) );

    fWave1HistT.push_back( std::vector< TH1F* >( fChannels[i] ) );
    fWave2HistT.push_back( std::vector< TH1F* >( fChannels[i] ) );

    // Initialize ROOT Objects
    for( int chan = 0; chan < fChannels[i]; chan++ ){
      fEnergyHist[i][chan] = new TH1F( ( fNames[i] + "_" + std::to_string( i ) + "_Energy_Channel" + std::to_string( chan ) ).c_str( ), ( fNames[i] + "_" + std::to_string( i ) + "_Energy_Channel" + std::to_string( chan ) ).c_str( ), 32768, 0, 32768 );
      fQshortHist[i][chan] = new TH1F( ( fNames[i] + "_" + std::to_string( i ) + "_Qshort_Channel" + std::to_string( chan ) ).c_str( ), ( fNames[i] + "_" + std::to_string( i ) + "_Qshort_Channel" + std::to_string( chan ) ).c_str( ), 32768, 0, 32768 );
      fQlongHist[i][chan] = new TH1F( ( fNames[i] + "_" + std::to_string( i ) + "_Qlong_Channel" + std::to_string( chan ) ).c_str( ), ( fNames[i] + "_" + std::to_string( i ) + "_Qlong_Channel" + std::to_string( chan ) ).c_str( ), 32768, 0, 32768 );
      fWave1Hist[i][chan] = new TGraph( 10000 );
      fWave2Hist[i][chan] = new TGraph( 10000 );

      fWave1HistT[i][chan] = new TH1F( ( fNames[i] + "_" + std::to_string( i ) + "_Wave1_Channel" + std::to_string( chan ) ).c_str( ), ( fNames[i] + "_" + std::to_string( i ) + "_Wave1_Channel" + std::to_string( chan ) ).c_str( ), 10000, 0, 10000 );
      fWave2HistT[i][chan] = new TH1F( ( fNames[i] + "_" + std::to_string( i ) + "_Wave2_Channel" + std::to_string( chan ) ).c_str( ), ( fNames[i] + "_" + std::to_string( i ) + "_Wave2_Channel" + std::to_string( chan ) ).c_str( ), 10000, 0, 10000 );
    }
  }

}

void SpyServerRU::InitializeCalibration( ){

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
    fCalibrationA.push_back( std::vector<float> ( fChannels[i] ) );
    fCalibrationB.push_back( std::vector<float> ( fChannels[i] ) );
    if( file.is_open( ) ){
      for( int chan = 0; chan < fChannels[i]; chan++ ){
        std::getline( file, line );
        std::istringstream iss( line );
        iss >> fCalibrationA[i][chan] >> fCalibrationB[i][chan];
      }
    }
    else{
      for( int chan = 0; chan < fChannels[i]; chan++ ){
        fCalibrationA[i][chan] = 0;
        fCalibrationB[i][chan] = 1;
      }
    }
    file.close( );
  }
}

void SpyServerRU::Reset( ){

  // Set all the histograms to zero
  for( int i = 0; i < fChannels.size( ); i++ ){
    for( int chan = 0; chan < fChannels[i]; chan++ ){
      for( int j = 0; j < 32768; j++ ){
        fEnergy[i][chan][j] = 0;
        fQshort[i][chan][j] = 0;
        fQlong[i][chan][j] = 0;
      }
      for( int j = 0; j < 10000; j++ ){
        fWave1[i][chan][j] = 0;
        fWave2[i][chan][j] = 0;
      }
    }
  }

}

void SpyServerRU::Save( ){

  // Save the histograms to a file
  /*std::ofstream file;

  for( int i = 0; i < fChannels.size( ); i++ ){
    for( int chan = 0; chan < fChannels[i]; chan++ ){

      file.open( "data/run" + std::to_string(run) + "/" + fNames[i] + "_Histo_Channel" + std::to_string( chan ) + ".dat" );
      for( int j = 0; j < 32768; j++ ){
        file << j << " " << fEnergy[i][chan][j]  << std::endl;
      }

      file.close( );
    }
  }*/

  // Save the ROOT file
  //std::string filename = "data/run" + std::to_string(run) + "/run" + std::to_string(run) + ".root";
  //fFile = new TFile( filename.c_str( ), "RECREATE" );

  //for( int i = 0; i < fChannels.size( ); i++ ){
  //  for( int chan = 0; chan < fChannels[i]; chan++ ){
  //    fEnergyHist[i][chan]->Write( );
  //  }
  //}

  //fFile->Close( );

}

bool SpyServerRU::Connect( ){

  // Connects to the XDAQ spy port
  if( xdaq->Connect( hostname, 10002 ) ) return true;
  else return false;

}

void SpyServerRU::Disconnect( ){

  xdaq->Disconnect( );

}

void SpyServerRU::Start( ){

  if( this->Connect( ) ){ 

    startCall = 1;
    stopCall  = 0;
    
    spyThread = new boost::thread( &SpyServerRU::GetNextEvent, this );
    rootThread = new boost::thread( &SpyServerRU::rootServer, this );

  }

}

void SpyServerRU::Stop( ){

  if( startCall ){

    startCall = 0;
    stopCall  = 1;
    xdaq->Disconnect( );
    spyThread->join();
    xdaq->Close( ); 

  }

}

void SpyServerRU::UpdateWave( ){

  for( auto& it : fWaveUpdateMap ){
    for( auto& jt : it.second ){
      jt.second = true;
    }
  }

}

void SpyServerRU::UpdateGraphs( ){

}

void SpyServerRU::FillGraphs( uint32_t* inpBuffer, uint32_t board, uint32_t chan, DataFrame dataForm ){

  uint16_t numSamples = dataForm.GetConfig( "NumSamples" )/2 - 5;

  std::pair<int,int> formatSample = dataForm.GetFormat( "Sample" );
  std::pair<int,int> formatDP1    = dataForm.GetFormat( "DP1" );
  std::pair<int,int> formatDP2    = dataForm.GetFormat( "DP2" );

  if( fVerbose ){
    std::cout << "NumSamples: " << numSamples << std::endl;
    std::cout << "Sample: " << formatSample.first << " " << formatSample.second << std::endl;
    std::cout << "DP1: " << formatDP1.first << " " << formatDP1.second << std::endl;
    std::cout << "DP2: " << formatDP2.first << " " << formatDP2.second << std::endl;
  }

  short temp;
  for( uint16_t idx = 0; idx < numSamples; ++idx ){
    if( dataForm.CheckEnabled( "DT" ) ){
      if( fVerbose ) std::cout << "DT Enabled" << std::endl;
      temp = static_cast<short>(project_range( formatSample.first   , formatSample.second   , std::bitset<32>(inpBuffer[idx])).to_ulong());
      if( fVerbose ) std::cout << temp << std::endl;
      fWave1[board][chan][idx] = temp;
      if( fVerbose ) std::cout << "Setting point " << idx << std::endl;
      fWave1Hist[board][chan]->SetPoint( idx, idx, temp );
      if( fVerbose ) std::cout << "Setting bin content " << idx << std::endl;
      fWave1HistT[board][chan]->SetBinContent( idx, temp );
      temp = static_cast<short>(project_range( formatSample.first+16, formatSample.second+16, std::bitset<32>(inpBuffer[idx])).to_ulong());
      if( fVerbose ) std::cout << temp << std::endl;
      //fWave2[board][chan][idx] = temp;
      if( fVerbose ) std::cout << "Setting point " << idx << std::endl;
      fWave2Hist[board][chan]->SetPoint( idx, idx, temp );
      if( fVerbose ) std::cout << "Setting bin content " << idx << std::endl;
      fWave2HistT[board][chan]->SetBinContent( idx, temp );
      //temp = static_cast<bool>(project_range(  formatDP1.first      , formatDP1.second      , std::bitset<32>(inpBuffer[idx])).to_ulong());
      //fDigitalProbe1[board][chan]->SetPoint( idx, idx/2, temp );
      //temp = static_cast<bool>(project_range(  formatDP1.first+16   , formatDP1.second+16   , std::bitset<32>(inpBuffer[idx])).to_ulong());
      //fDigitalProbe1[board][chan]->SetPoint( idx, idx, temp );
      //temp = static_cast<bool>(project_range(  formatDP2.first      , formatDP2.second      , std::bitset<32>(inpBuffer[idx])).to_ulong());
      //fDigitalProbe2[board][chan]->SetPoint( idx, idx/2, temp );
      //temp = static_cast<bool>(project_range(  formatDP2.first+16   , formatDP2.second+16   , std::bitset<32>(inpBuffer[idx])).to_ulong());
      //fDigitalProbe2[board][chan]->SetPoint( idx, idx, temp );
    }
    else{
      temp = static_cast<short>(project_range( formatSample.first   , formatSample.second   , std::bitset<32>(inpBuffer[idx])).to_ulong());
      fWave1[board][chan][2*idx] = temp;
      fWave1Hist[board][chan]->SetPoint( 2*idx, 2*idx, temp );
      fWave1HistT[board][chan]->SetBinContent( 2*idx, temp );
      temp = static_cast<short>(project_range( formatSample.first+16, formatSample.second+16, std::bitset<32>(inpBuffer[idx])).to_ulong());
      fWave1[board][chan][2*idx+1] = temp;
      fWave1Hist[board][chan]->SetPoint( 2*idx+1, 2*idx+1, temp );
      fWave1HistT[board][chan]->SetBinContent( 2*idx+1, temp );
      //temp = static_cast<short>(project_range( formatDP1.first      , formatDP1.second      , std::bitset<32>(inpBuffer[idx])).to_ulong());
      //fDigitalProbe1[board][chan]->SetPoint( 2*idx, 2*idx, temp );
      //temp = static_cast<short>(project_range( formatDP1.first+16   , formatDP1.second+16   , std::bitset<32>(inpBuffer[idx])).to_ulong());
      //fDigitalProbe1[board][chan]->SetPoint( 2*idx+1, 2*idx+1, temp );
      //temp = static_cast<short>(project_range( formatDP2.first      , formatDP2.second      , std::bitset<32>(inpBuffer[idx])).to_ulong());
      //fDigitalProbe2[board][chan]->SetPoint( 2*idx, 2*idx, temp );
      //temp = static_cast<short>(project_range( formatDP2.first+16   , formatDP2.second+16   , std::bitset<32>(inpBuffer[idx])).to_ulong());
      //fDigitalProbe2[board][chan]->SetPoint( 2*idx+1, 2*idx+1, temp );
    }

  }

  UpdateGraphs( );

}

void SpyServerRU::UnpackHeader( uint32_t* inpBuffer, uint32_t& aggLength, uint32_t& board, std::bitset<8>& channelMask, uint32_t& offset ){

  // Aggregate buffer has a header containing different information
  aggLength                  = inpBuffer[0+offset]&0x0FFFFFFF;
  board                      = (inpBuffer[1+offset]&0xF8000000)>>27;
  uint32_t boardFailFlag     = (inpBuffer[1+offset]&0x4000000)<<5 ;
  channelMask                = inpBuffer[1+offset]&0xFF;
  uint32_t aggregateCounter  = inpBuffer[2+offset]&0x7FFFFF;
  uint32_t aggregateTimeTag  = inpBuffer[3+offset];

  if( this->fVerbose ){
    std::cout << "Aggregate Length: " << aggLength << std::endl;
    std::cout << "Board: " << board << std::endl;
    std::cout << "Board Fail Flag: " << boardFailFlag << std::endl;
    std::cout << "Channel Mask: " << channelMask << std::endl;
    std::cout << "Aggregate Counter: " << aggregateCounter << std::endl;
    std::cout << "Aggregate Time Tag: " << aggregateTimeTag << std::endl;
  }

}

void SpyServerRU::UnpackPHA( uint32_t* inpBuffer, uint32_t& board, std::bitset<8>& channelMask, uint32_t& offset ){

  // We need to reconstruct the channel number from the couple id for the 
  // 16 channels boards
  int index = 0;
  uint8_t couples[8]={0,0,0,0,0,0,0,0};
  for(uint16_t bit = 0 ; bit < 8 ; bit++){ 
    if(channelMask.test(bit)){
      couples[index]=bit;
      index++;
    }
  }

  // Getting the DataForm
  DataFrame dataForm = fDataFrame[board];
  std::pair<int,int> format;
  uint32_t pos, wavePos;

  // Variables to store the data
  uint32_t startingPos         = 4 + offset;
  uint32_t coupleAggregateSize = 0;
  uint32_t extras              = 0;
  uint32_t extras2             = 0;    
  uint16_t energy              = 0;
  uint64_t tstamp              = 0;
  uint16_t fineTS              = 0;
  uint32_t flags               = 0;
  uint16_t numSamples          = 0; 
  bool pur                     = false;
  bool satu                    = false;
  bool lost                    = false;

  // Here starts the real decoding of the data for the channels
  uint32_t chanNum = 0; 
  for(uint8_t cpl = 0 ; cpl < channelMask.count() ; cpl++){

    // Channel aggregate size is board dependant
    pos    = startingPos;
    format = dataForm.GetFormat( "Size" );
    coupleAggregateSize = project_range( format.first, format.second, std::bitset<32>( inpBuffer[pos] ) ).to_ulong( );

    dataForm.SetDataFormat(inpBuffer[startingPos+1]);
      
    for(uint16_t evt = 0 ; evt < (coupleAggregateSize-2)/dataForm.EvtSize();evt++){
	      
      // PU and Energy seems not to be dependant on the board
      pur    = static_cast<bool>(((inpBuffer[startingPos+dataForm.EvtSize()-1+dataForm.EvtSize()*evt+2])&0x8000)>>15);
      energy = static_cast<uint16_t>((inpBuffer[startingPos+dataForm.EvtSize()-1+dataForm.EvtSize()*evt+2])&0x7FFF);

      // Time stamp is board dependant
      pos    = startingPos+2+dataForm.EvtSize()*evt;
      format = dataForm.GetFormat( "TS" );
      tstamp = static_cast<uint64_t>(project_range( format.first, format.second, std::bitset<32>(inpBuffer[pos])).to_ulong());

      // Channel number is board dependant
      if( dataForm.CheckFormat( "CH" ) ){
        chanNum  = static_cast<uint32_t>(((inpBuffer[startingPos+2+dataForm.EvtSize()*evt]&0x80000000)>>31)+couples[cpl]*2);
      }
      else{
        chanNum = couples[cpl];
      }

      // Some boards uses the Roll Over flag
      if( dataForm.CheckFormat( "RO" ) ){
        if( ((inpBuffer[startingPos+2+dataForm.EvtSize()*evt]&0x80000000)>>31) ){
          ++fRO[board];
        }
      }

      // Extras size is board dependant
      pos     = startingPos+dataForm.EvtSize()-1+dataForm.EvtSize()*evt+2;
      format  = dataForm.GetFormat( "Extras" );
      extras  = static_cast<uint16_t>(project_range( format.first, format.second, std::bitset<32>( inpBuffer[pos] ) ).to_ulong( ));

      // If Roll Over flag is set, we need to add it
      if( pur && ( energy == 0 ) && ( tstamp == 0 ) && (extras&0002) ){
        ++fRO[board];
      }

      // Adding RO to the Time Stamp
      format = dataForm.GetFormat( "TS" );
      tstamp = ( fRO[board]<<(format.second+1) | tstamp );

      // The Lost and Satu flags seems not to be dependant on the board
	    lost  = (extras&0x20)>>5;
	    satu  = (extras&0x10)>>4;

	    if(dataForm.CheckEnabled( "Extras" )){
	      extras2 = inpBuffer[startingPos+dataForm.EvtSize()-2+dataForm.EvtSize()*evt+2];
	      switch(dataForm.GetConfig( "Extras" )) {
	        case 0: // Extended Time Stamp [31:16] ; baseline*4 [15:0]
	          tstamp = ((uint64_t)(extras2&0xFFFF0000))<<15 | (uint64_t)tstamp;
	          break;
	        case 1: // Extended Time stamp [31:16] ; flags [15:0]
	          tstamp = ((uint64_t)(extras2&0xFFFF0000))<<15 | (uint64_t)tstamp;
	          break;
	        case 2: // Extended Time stamp [31:16] ; Flags [15:10] ; Fine Time Stamp [9:0]
	          fineTS = static_cast<uint16_t>(extras2&0x3FF);
	          tstamp = ((uint64_t)(extras2&0xFFFF0000))<<15 | (uint64_t)tstamp;
	          break;
	        case 4: // Lost Trigger Counter [31:16] ; Total Trigger [15:0]
	          break;
	        case 5: // Positive zero crossing [31:16] ; Negative zero crossing [15:0]
	          break;
	        case 7: // Debug fixed value;
	          break;
	        default:
	          break;
	      }
	  
      }
      else extras2 = 0;

      if( this->fVerbose ){
        std::cout << "Board: " << board << std::endl;
        std::cout << "Channel: " << chanNum << std::endl;
        std::cout << "Energy: " << energy << std::endl;
        std::cout << "Time Stamp: " << tstamp << std::endl;
        std::cout << "Fine Time Stamp: " << fineTS << std::endl;
        std::cout << "Extras: " << extras << std::endl;
        std::cout << "Extras2: " << extras2 << std::endl;
      }

      if( dataForm.CheckEnabled( "Trace" ) && fWaveUpdateMap[board][chanNum] ){
        if( waveThread != nullptr ){
          if( !waveThread->timed_join(boost::posix_time::seconds(0)) ){
            waveThread->join( );
            delete waveThread;
            waveThread = nullptr;
            numSamples = dataForm.GetConfig( "NumSamples" )/2;
            delete waveBuffer;
            waveBuffer = nullptr;
            waveBuffer = new uint32_t[numSamples];
            wavePos = (startingPos+3+dataForm.EvtSize()*evt);
            memcpy(waveBuffer,inpBuffer+wavePos,numSamples*sizeof(uint32_t));
            waveThread = new boost::thread( boost::bind( &SpyServerRU::FillGraphs, this, waveBuffer, board, chanNum, dataForm ) );
            fWaveUpdateMap[board][chanNum] = false;
          }
        }
        else{
          numSamples = dataForm.GetConfig( "NumSamples" )/2;
          waveBuffer = new uint32_t[numSamples];
          wavePos = (startingPos+3+dataForm.EvtSize()*evt);
          memcpy(waveBuffer,inpBuffer+wavePos,numSamples*sizeof(uint32_t));
          waveThread = new boost::thread( boost::bind( &SpyServerRU::FillGraphs, this, waveBuffer, board, chanNum, dataForm ) );
          fWaveUpdateMap[board][chanNum] = false;
        }
      }

      // Filling ROOT files
      fEnergy[board][chanNum][energy]++;

      energy = fCalibrationA[board][chanNum] + fCalibrationB[board][chanNum] * energy;
      fEnergyHist[board][chanNum]->Fill( energy );

      if(ratesMonitor.values.find(chanNum) == ratesMonitor.values.end()) ratesMonitor.values[chanNum].Init();
      ratesMonitor.values[chanNum].time = tstamp;
	    ++ratesMonitor.values[chanNum].totalEvents ;
	    if( pur )  ++ratesMonitor.values[chanNum].pileEvents ;
	    if( satu ) ++ratesMonitor.values[chanNum].satuEvents ;
	    if( lost ) ++ratesMonitor.values[chanNum].lostEvents ;

    }

    startingPos+=coupleAggregateSize;

  }

}

void SpyServerRU::UnpackPSD( uint32_t* inpBuffer, uint32_t& board, std::bitset<8>& channelMask, uint32_t& offset ){
  
  // We need to reconstruct the channel number from the couple id for the 
  // 16 channels boards
  int index = 0 ;
  uint8_t couples[8]={0,0,0,0,0,0,0,0};
  for(uint16_t bit = 0 ; bit < 8 ; bit++){ 
    if(channelMask.test(bit)){
      couples[index]=bit;
      index++;
    }
  }

  // Getting the DataForm
  DataFrame dataForm = fDataFrame[board];
  std::pair<int,int> format;
  uint32_t pos, wavePos;

  // Variables to store the data
  uint32_t startingPos         = 4 + offset;
  uint32_t coupleAggregateSize = 0;
  uint64_t tstamp              = 0;
  uint32_t fineTS              = 0;
  uint32_t extras              = 0;
  uint32_t flags               = 0;
  uint16_t qshort              = 0;
  uint16_t qlong               = 0;
  uint16_t numSamples          = 0;
  bool pur                     = false;
  bool ovr                     = false;
  bool lost                    = false;
  
  // Here starts the real decoding of the data for the channels 
  uint32_t chanNum = 0;
  for(uint8_t cpl = 0 ; cpl < channelMask.count() ; cpl++){
    
    // Channel aggregate size is board dependant
    pos    = startingPos;
    format = dataForm.GetFormat( "Size" );
    coupleAggregateSize = project_range( format.first, format.second, std::bitset<32>( inpBuffer[pos] ) ).to_ulong( );

    dataForm.SetDataFormat(inpBuffer[startingPos+1]);

    for(uint16_t evt = 0 ; evt < (coupleAggregateSize-2)/dataForm.EvtSize();evt++){

      // PU and Charges seems not to be dependant on the board
      pur  = static_cast<bool>(((inpBuffer[startingPos+dataForm.EvtSize()-1+dataForm.EvtSize()*evt+2])&0x8000)>>15);
      qshort  = static_cast<uint16_t>((inpBuffer[startingPos+dataForm.EvtSize()-1+dataForm.EvtSize()*evt+2])&0x7FFF);
      qlong   = static_cast<uint16_t>((inpBuffer[startingPos+dataForm.EvtSize()-1+dataForm.EvtSize()*evt+2])>>16);

      // Time stamp is board dependant
      pos    = startingPos+2+dataForm.EvtSize()*evt;
      format = dataForm.GetFormat( "TS" );
      tstamp = static_cast<uint64_t>(project_range( format.first, format.second, std::bitset<32>(inpBuffer[pos])).to_ulong());

      // Channel number is board dependant
      if( dataForm.CheckFormat( "CH" ) ){
        chanNum  = static_cast<uint32_t>(((inpBuffer[startingPos+2+dataForm.EvtSize()*evt]&0x80000000)>>31)+couples[cpl]*2);
      }
      else{
        chanNum = couples[cpl];
      }
      
      if(dataForm.CheckEnabled( "Extras" )){
	      extras = inpBuffer[startingPos+dataForm.EvtSize()-2+dataForm.EvtSize()*evt+2];
        if(dataForm.CheckConfig( "Extras" )){
	        switch(dataForm.GetConfig( "Extras" )) {
	          case 0: // Extended Time Stamp [31:16] ; baseline*4 [15:0]
	            tstamp = (static_cast<uint64_t>((extras&0xFFFF0000))<<15) | tstamp;
	            break;
	          case 1: // Extended Time stamp [31:16] ; flags [15:0]
	            ovr    = static_cast<bool>((extras&0x00004000)>>14);
	            lost   = static_cast<bool>((extras&0x00001000)>>12);
	            flags  = static_cast<uint32_t>((extras&0x0000F000)<<15);
	            tstamp = (static_cast<uint64_t>((extras&0xFFFF0000))<<15) | tstamp;
	            break;
	          case 2: // Extended Time stamp [31:16] ; Flags [15:10] ; Fine Time Stamp [9:0]
	            flags  = static_cast<uint32_t>((extras&0x0000F000)<<15);
	            fineTS = static_cast<uint16_t>(extras&0x000003FF);
	            tstamp = ((uint64_t)(extras)&0xFFFF0000)<<15 | tstamp;
	            break;
	          case 4: // Lost Trigger Counter [31:16] ; Total Trigger [15:0]
	            break;
	          case 5: // Positive zero crossing [31:16] ; Negative zero crossing [15:0]
	            break;
	          case 7: // Debug fixed value;
	            break;
	          default:
	            break;
	        }

        }
        else tstamp = (static_cast<uint64_t>((extras&0x00007FFF))<<32) | tstamp;

      }

      if( dataForm.CheckEnabled( "Trace" ) && fWaveUpdateMap[board][chanNum] ){
        if( waveThread != nullptr ){
          if( !waveThread->timed_join(boost::posix_time::seconds(0)) ){
            waveThread->join( );
            delete waveThread;
            waveThread = nullptr;
            numSamples = dataForm.GetConfig( "NumSamples" ) / 2;
            delete waveBuffer;
            waveBuffer = nullptr;
            waveBuffer = new uint32_t[numSamples];
            wavePos = (startingPos+2+dataForm.EvtSize()*evt);
            memcpy(waveBuffer,inpBuffer+wavePos,numSamples*sizeof(uint32_t));
            waveThread = new boost::thread( boost::bind( &SpyServerRU::FillGraphs, this, waveBuffer, board, chanNum, dataForm ) );
            fWaveUpdateMap[board][chanNum] = false;
          }
        }
        else{
          numSamples = dataForm.GetConfig( "NumSamples" )/2;
          waveBuffer = new uint32_t[numSamples];
          wavePos = (startingPos+2+dataForm.EvtSize()*evt);
          memcpy(waveBuffer,inpBuffer+wavePos,numSamples*sizeof(uint32_t));
          waveThread = new boost::thread( boost::bind( &SpyServerRU::FillGraphs, this, waveBuffer, board, chanNum, dataForm ) );
          fWaveUpdateMap[board][chanNum] = false;
        }
      }

      // Filling ROOT files
      fQshort[board][chanNum][qshort]++;
      fQlong[board][chanNum][qlong]++;

      fQshortHist[board][chanNum]->Fill( qshort );
      fQlongHist[board][chanNum]->Fill( qlong );

    }
      
    startingPos+=coupleAggregateSize;

  }

}

uint32_t SpyServerRU::ReadHeader( char* buffer ){

  uint32_t hSize = 0;
  memcpy(&hSize,buffer,sizeof(uint32_t));

  // Here we can continue decrypt the header but it is not necessary
  // for the SpyServerRU

  return hSize;

}

void SpyServerRU::GetNextEvent( ){

  // Ask for the next buffer and unpacks it.
  // When thread is started, the startCall is up.
  // When stopped, it goes down and exits.
  
  int            length;
  uint32_t       aggLength, board;
  std::bitset<8> channelMask;

  char* buffer = new char [512*512];

  bool firstBuffer = true;
  while( startCall ){
    
    length = xdaq->GetNextBuffer( buffer );

    int      pos    = 0;
    uint32_t offset = 0;
    if( firstBuffer ){
      offset = ReadHeader( buffer );
      firstBuffer = false;
      continue; // FIXME: ReadHeader sometimes gives a wrong length and provokes a Segmentation Fault. 
                //        Thus we skip the first buffer.
    }

    int bIdx;
    while( pos < length && startCall ){

      UnpackHeader( (uint32_t*)buffer, aggLength, board, channelMask, offset );
      if( ( offset + aggLength )*sizeof(u_int32_t) > length )  // Just in case
        break;

      if( this->fFirmware[board] == 0 )
        UnpackPHA( (uint32_t*)buffer, board, channelMask, offset );
      else if( this->fFirmware[board] == 1 )
        UnpackPSD( (uint32_t*)buffer, board, channelMask, offset );
      else;

      offset += aggLength;
      pos    += offset*sizeof(uint32_t);
    
    }

    bzero(buffer,512*512);

  }

  delete []buffer;

}

void SpyServerRU::rootServer( ){

  // This is a simple ROOT server
  serverSocket = new TServerSocket( 6060, kTRUE );

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
      for( auto it = fEnergyHist.begin(); it != fEnergyHist.end(); it++ ){
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
      for( auto it = fQshortHist.begin(); it != fQshortHist.end(); it++ ){
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
      for( auto it = fQlongHist.begin(); it != fQlongHist.end(); it++ ){
        for( auto jt = it->begin(); jt != it->end(); jt++ ){
          histograms.push_back( (TH1F*)(*jt) );
        }
      }

      for (const auto& hist : histograms) {
          TMessage message(kMESS_OBJECT);
          message.WriteObject(hist);
          clientSocket->Send(message);
        }

      // Send Wave1 histograms
      std::vector<TH1F*> waveHist;
      for( auto it = fWave1HistT.begin(); it != fWave1HistT.end(); it++ ){
        for( auto jt = it->begin(); jt != it->end(); jt++ ){
          waveHist.push_back( (TH1F*)(*jt) );
        }
      }

      for (const auto& hist : waveHist) {
          TMessage message(kMESS_OBJECT);
          message.WriteObject(hist);
          clientSocket->Send(message);
        }

      // Send Wave2 histograms
      waveHist.clear();
      for( auto it = fWave2HistT.begin(); it != fWave2HistT.end(); it++ ){
        for( auto jt = it->begin(); jt != it->end(); jt++ ){
          waveHist.push_back( (TH1F*)(*jt) );
        }
      }

      for (const auto& hist : waveHist) {
          TMessage message(kMESS_OBJECT);
          message.WriteObject(hist);
          clientSocket->Send(message);
        }

      this->UpdateWave( );

    }

    clientSocket->Close();
    clientSocket = nullptr;

  }

}