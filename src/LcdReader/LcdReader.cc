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

#include "rapidjson/document.h"    	// rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"  	// for stringify JSON
using namespace rapidjson;


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
    currentOperatingMode=OperatingMode::unknown;

    if ( lcdMessage.substr(0, 10) == "set TRAVAI" )
      currentOperatingMode=OperatingMode::on;
    else if ( lcdMessage.substr(0, 10) == "  OFF     " )
       currentOperatingMode=OperatingMode::off;
    else if ( lcdMessage.substr(0, 6) == "COMNC " )
       currentOperatingMode=OperatingMode::starting;
    else if ( lcdMessage.substr(0, 11) == "  NETTOYAGE")
       currentOperatingMode=OperatingMode::cleaning;
    else if ( lcdMessage.substr(0, 28) == "    ALARME         TEMP FUME" )   
       currentOperatingMode=OperatingMode::alertTempFume;
    else if ( lcdMessage.substr(0, 28) == "    ALARME       TERM- DEPR." )
       currentOperatingMode=OperatingMode::alertTermDepr;
    else if ( lcdMessage.substr(0, 7) == "ATTENTE")
       currentOperatingMode=OperatingMode::delayedStart;

    if ( (currentOperatingMode == OperatingMode::on)
      || (currentOperatingMode == OperatingMode::off)
      || (currentOperatingMode == OperatingMode::starting) )
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

  string LcdReader::getInfoJson() const
  { 
    string resultat = "";
    GenericStringBuffer<UTF8<> > buffer;
    Writer<GenericStringBuffer<UTF8<> > > writer( buffer );
    writer.StartObject();
    writer.String( "power" );
    writer.Int( getPower() );
    writer.String( "tempWater" );
    writer.Int( getTempWater() );
    writer.String( "tempWaterConsigne" );
    writer.Int( getTempWaterConsigne() );
    writer.String( "operatingMode" );

    switch(currentOperatingMode)
    {
      case OperatingMode::unknown:
        writer.String("unknown");
        break;
      case OperatingMode::on:
        writer.String("on");
        break;
      case OperatingMode::off:
        writer.String("off");
        break;
      case OperatingMode::starting:
        writer.String("starting");
        break;
      case OperatingMode::cleaning:
        writer.String("cleaning");
        break;
      case OperatingMode::alertTempFume:
        writer.String("alertTempFume");
        break;
      case OperatingMode::alertTermDepr:
        writer.String("alertTermDepr");
        break;
      case OperatingMode::delayedStart:
        writer.String("delayedStart");
        break;

    }
    writer.EndObject();
    resultat = buffer.GetString();
    buffer.Clear();
    return resultat;
  }

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

