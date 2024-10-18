#ifndef LFFRAMES_H
#define LFFRAMES_H

#include <stdint.h>

const uint32_t keyDomainMask    = 0x0000FFFF;   // mask applied to agataKey.evnum to get the domain number
const uint32_t keyBoardMask     = 0x0000FF00;   // mask applied to agataKey.evnum to get the board number
const uint32_t keyChannelMask   = 0x000000FF;   // mask applied to agataKey.evnum to get the channel number
const uint32_t keyIdleMask      = 0x80000000;   // mask applied to agataKey.evnum to gwt thw idle flag
const uint32_t keyPu            = 0x00010000;   // mask applied to agataKey.evnum to gwt thw pu flag
const uint32_t keySatu          = 0x00040000;   // mask applied to agataKey.evnum to gwt thw satu flag
const uint32_t keyLost          = 0x00080000;   // mask applied to agataKey.evnum to gwt thw lost flag

struct dataMergedKey {
  uint32_t size;
  uint32_t key;
  uint32_t evnum;
  uint32_t ts_0;
  uint32_t ts_1;

  uint32_t GetBytes ()   const { return size; }
  uint32_t GetEvnum ()   const { return evnum; }
  bool     IsIdle()      const { return (evnum & keyIdleMask)   != 0; }
};

struct dataKey {

  uint32_t size  = 0;   //
  uint32_t key   = 0;   //
  uint32_t evnum = 0;   //
  uint32_t ts_0  = 0;   //
  uint32_t ts_1  = 0;   //

  uint32_t GetBytes ()   const { return size; }
  uint32_t GetEvnum ()   const { return evnum; }
  uint64_t GetTstamp()   const { return (uint64_t(ts_1 )<<32) | (uint64_t(ts_0));}
  uint32_t GetDomain()   const { return (evnum & keyDomainMask); }
  uint32_t GetBoard()    const { return (evnum & keyBoardMask) >> 8; }
  uint32_t GetChannel()  const { return (evnum & keyChannelMask); }
  uint32_t GetPu()       const { return (evnum & keyPu) >> 16; }
  uint32_t GetLost()     const { return (evnum & keyLost) >> 19; }
  uint32_t GetSatu()     const { return (evnum & keySatu) >> 18; }
  bool     IsIdle   ()   const { return (evnum & keyIdleMask)   != 0; }

};


struct subDataPHA_t
{
  uint16_t energy = 0;
  uint16_t cfd    = 0;
};

struct subDataPSD_t
{
  uint16_t qshort = 0;
  uint16_t qlong  = 0;
  float cfd       = 0;
};

struct caenEventPHA_t
{
  dataKey      theKey;
  subDataPHA_t theData;
};

struct caenEventPSD_t
{
  dataKey      theKey;
  subDataPSD_t theData;
};

#endif // LFFRAMES_H
