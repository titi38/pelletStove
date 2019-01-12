//------------------------------------------------------------------------------
//
// LcdReader.cc : exec command
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

#include <unistd.h>
#include <wiringPi.h>
#include <ctype.h>


#include "libnavajo/libnavajo.hh"
#include "LcdReader.hh"

#define	LCD_DGRAM	0x80


  /***********************************************************************/

  LcdReader::LcdReader()
  {
    exiting = false;

    thread_loop=new thread(&LcdReader::loop, this);
    NVJ_LOG->append(NVJ_INFO, "LcdReader is starting" );
  }

  LcdReader::~LcdReader()
  {
    exiting = true;
    thread_loop->join();
  }

  /***********************************************************************/

  void LcdReader::initPinMode()
  {
    pinMode(static_cast<int>(LcdPins::d0), INPUT);
    pinMode(static_cast<int>(LcdPins::d1), INPUT);
    pinMode(static_cast<int>(LcdPins::d2), INPUT);
    pinMode(static_cast<int>(LcdPins::d3), INPUT);
    pinMode(static_cast<int>(LcdPins::d4), INPUT);
    pinMode(static_cast<int>(LcdPins::d5), INPUT);
    pinMode(static_cast<int>(LcdPins::d6), INPUT);
    pinMode(static_cast<int>(LcdPins::d7), INPUT);
    pinMode(static_cast<int>(LcdPins::en), INPUT);
    pinMode(static_cast<int>(LcdPins::rs), INPUT);
  }

  /***********************************************************************/

  char LcdReader::readLcdData()
  {
    return  (digitalRead(static_cast<int>(LcdPins::d7)) == HIGH) << 7
        | (digitalRead(static_cast<int>(LcdPins::d6)) == HIGH) << 6
        | (digitalRead(static_cast<int>(LcdPins::d5)) == HIGH) << 5
        | (digitalRead(static_cast<int>(LcdPins::d4)) == HIGH) << 4
        | (digitalRead(static_cast<int>(LcdPins::d3)) == HIGH) << 3
        | (digitalRead(static_cast<int>(LcdPins::d2)) == HIGH) << 2
        | (digitalRead(static_cast<int>(LcdPins::d1)) == HIGH) << 1
        | (digitalRead(static_cast<int>(LcdPins::d0)) == HIGH);
  }

  /***********************************************************************/

  void LcdReader::setLcdMessage(const char *msg)
  {
    lock_guard<std::mutex> lk(mutex_lcdMessage);

    if (lcdMessage == msg) return;

    lcdMessage = msg;
    NVJ_LOG->append(NVJ_DEBUG, string ("new Lcd Message :" + lcdMessage));

    if (lcdMessage.substr(0, 10) == "set TRAVAI")
    {
      power = stoi(lcdMessage.substr(19,1));
      tempWater = stoi(lcdMessage.substr(24,2).c_str());
      tempWaterConsigne = stoi(lcdMessage.substr(27,2).c_str());
    }
  }

  const string LcdReader::getLcdMessage() const
  { 
    lock_guard<std::mutex> lk(mutex_lcdMessage);
    return lcdMessage; 
  };

  /***********************************************************************/

  void LcdReader::loop()
  {
    for (int i=0; i<NB_LINES; i++)
      bufText[i][0]=0;

//    if ( wiringPiSetup() == -1 )
//      exit( 1 );

    initPinMode(); 

    while ( !exiting )
    {
      if ( digitalRead(static_cast<int>(LcdPins::en)) == HIGH )
      {
        if ( digitalRead(static_cast<int>(LcdPins::rs)) == LOW ) // COMMAND
        {
          if (!rearmedCommand)
            continue;
          rearmedCommand=false;
 
          char data = readLcdData() ;
          if (( data & 0x02 )== 0x02 ) //CurrentHome
            nbChome++;

          else 
            if (data == 0x01) // cls
              nbCls++;
		        
        }
        else // digitalRead(rs) == HIGH 
             // => write a char
        { 
          if (!rearmedData)
            continue;
          rearmedData=false;

          char data = readLcdData();
          if ((nbCls>=2 && nbChome) && (data < 0x20 ))
          { 
            bufText[posLine][posCurr++]=0;
            int nbEq=0;
            for (int i=0; i<NB_LINES && nbEq < NB_LINES/2; i++)
              if (strcmp(bufText[posLine], bufText[(posLine + i)%NB_LINES] )== 0)
                nbEq++;
            if (nbEq == NB_LINES/2)
              setLcdMessage(bufText[posLine]);
            posLine=(posLine+1)%NB_LINES;
            posCurr=0;
          }

          nbChome=0;
          nbCls=0;

          if ( isprint(data)  && posCurr<MAX_SCREEN_SIZE)
            bufText[posLine][posCurr++]=data;

        }
      }
      else
      {
        rearmedData=true;
        rearmedCommand=true;
      }
    }
  }

  /***********************************************************************/

