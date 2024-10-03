#ifndef DATAFRAMEBUILDERPSD_H_INCLUDED
#define DATAFRAMEBUILDERPSD_H_INCLUDED

#include <map>
#include <string>

#include "DataFrameBuilder.h"

class DataFrameBuilderPSD : public DataFrameBuilder{

    public:
        DataFrameBuilderPSD( std::string dgtzName );
        ~DataFrameBuilderPSD( );
        
        void ProduceTimeStamp( )  override;
        void ProduceNumSamples( ) override;
        void ProduceFlags( )      override;
        void ProduceConfigs( )    override;
        void ProduceFormats( )    override;

        int                                         GetTimeStamp()  override { return fTimeStamp; };
        int                                         GetNumSamples() override { return fNumSamples; };
        std::map<std::string, int>&                 GetFlags( )     override { return fFlags; }
        std::map<std::string, std::pair<int,int> >& GetConfigs( )   override { return fConfigs; }
        std::map<std::string, std::pair<int,int> >& GetFormat( )    override { return fFormats; }
        
    private:

        std::string fDgtzName;
        
        uint32_t fTimeStamp;
        uint32_t fNumSamples;
        std::map<std::string, int>                 fFlags;
        std::map<std::string, std::pair<int,int> > fConfigs;
        std::map<std::string, std::pair<int,int> > fFormats;

};

#endif // DATAFRAMEBUILDERPSD_H_INCLUDED