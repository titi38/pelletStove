//------------------------------------------------------------------------------
//
// LcdReader.hh : Press button of pellet stove controller
//
//------------------------------------------------------------------------------
//
// AUTHOR      : T.DESCOMBES (thierry.descombes@gmail.com)
// PROJECT     : pelletStove
//
//------------------------------------------------------------------------------
//           Versions and Editions  historic
//------------------------------------------------------------------------------
// Ver      | Ed  |   date   | resp.       | comment
//----------+-----+----------+-------------+------------------------------------
// 1        | 1   | 01/01/19 | T.DESCOMBES | source creation
//----------+-----+----------+-------------+------------------------------------
//
//           Detailed modification operations on this source
//------------------------------------------------------------------------------
//  Date  :                            Author :
//  REL   :
//  Description :
//
//------------------------------------------------------------------------------

#ifndef LCDREADER_HH_
#define LCDREADER_HH_

#define MAX_SCREEN_SIZE 32
#define NB_LINES 15

  /**
  * LcdReader - generic class to handle the relay
  */
  class LcdReader
  {

    enum class LcdPins : int { d0=0, d1=1, d2=6, d3=7, d4=10, d5=12, d6=13, d7=14, en=21, rs=22 /*Arduino: d0=2, d1=3, d2=8, d3=9, d4=10, d5=11, d6=12, d7=13, en=A1, rs=A2*/ };

    bool rearmedData=true, rearmedCommand=true;
    int nbCls=0, nbChome=0;

    char bufText[NB_LINES][MAX_SCREEN_SIZE+1];
    int posCurr =0;
    int posLine=0;
 
    char lcdMessage[MAX_SCREEN_SIZE+1]="";

    void initPinMode();
    char readLcdData();
    void loop();
    void setLcdMessage(const char* msg);

    public:
      void startThread();
      void stopThread();
      const char *getLcdMessage() const { return lcdMessage; };
  };
  
#endif

