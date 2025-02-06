#include "Stats.h"

Stats::Stats()
{
  stopCall = 0;
  lost  = ".lostRate" ;
  total = ".totalRate";
  satu  = ".satuRate" ;
  pile  = ".pileRate" ;
  dead  = ".deadTime" ;
  sockCarbon = -1;
  graphiteThread = new std::thread( &Stats::Thread, this ); 
}

Stats::~Stats( )
{
  stopCall = 1;
  graphiteThread->join();
}


int Stats::init()
{
  struct sockaddr_in serv_addr;

  struct hostent *server;                                                                                       
  sockCarbon = socket(AF_INET,SOCK_STREAM,0);                                                                 
                                                                                                                
  server = gethostbyname(hostName.c_str());                                                                          
  if(server == nullptr){                                                                                        
    std::cerr << "Carbon host not found" << std::endl;
    sockCarbon = -1;
    return 0;                                                          
  }                                                                                                             
  bzero((char*)&serv_addr,sizeof(serv_addr));                                                                   
  serv_addr.sin_family = AF_INET;                                                                               
  bcopy((char*)server->h_addr,                                                                                  
        (char *)&serv_addr.sin_addr.s_addr,                                                                     
        server->h_length);                                                                                      
  serv_addr.sin_port = htons(hostPort);                                                                             
  if (connect(sockCarbon,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)                               
    std::cout << "ERROR connecting" << std::endl;   
  return 0;

}

void Stats::Thread( )
{

  double deltaT;
  double deadTime;
  double OCR;
  double ICR;

  std::ostringstream graphiteMessage;

  unsigned int ch;
  
  int nBytes;
  while( stopCall == 0 ){
    graphiteMessage.str("");
    epochTime = std::time(nullptr);

    for( auto ch = values.begin(); ch != values.end() ; ++ch ){
      deltaT = ch->second.time - ch->second.timePrev;
      deltaT *= 1e-9;
      ch->second.timePrev = ch->second.time;

      ch->second.lostRate  = ch->second.lostEvents  / deltaT;
      ch->second.totalRate = ch->second.totalEvents / deltaT;
      ch->second.satuRate  = ch->second.satuEvents  / deltaT;
      ch->second.pileRate  = ch->second.pileEvents  / deltaT;

      ch->second.lostEvents  = 0;
      ch->second.totalEvents = 0;
      ch->second.pileEvents  = 0;
      ch->second.satuEvents  = 0;
          
      ICR = ch->second.totalRate + ch->second.lostRate;
      OCR = ch->second.totalRate - ch->second.satuRate - ch->second.pileRate;
      
      if( ch->second.totalRate != 0 ) deadTime = 1 - ( OCR )/( ICR );
      else deadTime = 0;
      
      if( deadTime < 0 ) deadTime = 0;
      // Total rate
      if(ch->second.totalRate>=0)
	graphiteMessage << "ancillary.rates." << board << ".ch_" << ch->first << total 
			<< " " << ch->second.totalRate << " "   <<epochTime << std::endl;
      // Lost rate
      if(ch->second.lostRate>=0)
	graphiteMessage << "ancillary.rates." << board << ".ch_" << ch->first << lost 
			<< " " << ch->second.lostRate << " "   <<epochTime << std::endl;
      // Saturation rate
      if(ch->second.satuRate>=0)
	graphiteMessage << "ancillary.rates." << board << ".ch_" << ch->first << satu 
			<< " " << ch->second.satuRate << " "   <<epochTime << std::endl;
      // Pile-up rate
      if(ch->second.pileRate>=0)
	graphiteMessage << "ancillary.rates." << board << ".ch_" << ch->first << pile
			<< " " << ch->second.pileRate << " "   <<epochTime << std::endl;
      // Dead time
      if(deadTime>=0)
	graphiteMessage << "ancillary.rates." << board << ".ch_" << ch->first << dead
			<< " " << deadTime << " "   <<epochTime << std::endl;
    }
    if( graphiteMessage.str().length()>0 ){
      //      std::cout << graphiteMessage.str() << std::endl;
      if(sockCarbon!=-1){
	nBytes = write(sockCarbon, graphiteMessage.str().c_str(), strlen(graphiteMessage.str().c_str()));
	if (nBytes < 0) {
	  perror("write");
	  //	exit(1); //Maybe not exit for just a TCP error
	}
      }
    }
    std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
  }
  if(sockCarbon!=-1){
    close(sockCarbon);
    sockCarbon = -1;
  }  
}
