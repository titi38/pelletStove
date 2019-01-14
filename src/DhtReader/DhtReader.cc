//------------------------------------------------------------------------------
//
// DhtReader.cc : exec command
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

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "rapidjson/document.h"    	// rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"  	// for stringify JSON


#include "DhtReader.hh"


// dispo : 8 9 11 15 16 17 18 19 20
#define DHTPIN		11 // GPIO7 (SPI_CE1_N)
#define MAXTIMINGS	95



using namespace rapidjson;


  DhtReader::DhtReader()
  {
//    if ( wiringPiSetup() == -1 )
//      exit( 1 );

    exiting = false;
    thread_loop=new thread(&DhtReader::loop, this);
    NVJ_LOG->append(NVJ_INFO, "DhtReader is starting" );
  }

  DhtReader::~DhtReader()
  {
    exiting = true;
    thread_loop->join();
  }

  string DhtReader::getInfoJson() const
  { 
    string resultat = "";
    GenericStringBuffer<UTF8<> > buffer;
    Writer<GenericStringBuffer<UTF8<> > > writer( buffer );
    writer.StartObject();
    writer.String( "temp" );
    writer.Double( getTemp() );
    writer.String( "humi" );
    writer.Double( getHumi() );
    writer.String( "humidex" );
    writer.Double( getHumidex() );
    writer.EndObject();
    resultat = buffer.GetString();
    buffer.Clear();
    return resultat;
  } 

  /***********************************************************************/
 
  bool DhtReader::read_dht_dat()
  {
	uint8_t laststate	= HIGH;
	uint8_t counter		= 0;
	uint8_t j		= 0;
 
	dht_dat[0] = dht_dat[1] = dht_dat[2] = dht_dat[3] = dht_dat[4] = 0;
 
	pinMode( DHTPIN, OUTPUT );

	digitalWrite( DHTPIN, LOW );
	delay( 18 );
	digitalWrite( DHTPIN, HIGH );
        delayMicroseconds( 40 );

	pinMode( DHTPIN, INPUT );
 
	for ( int i = 0; i < MAXTIMINGS; i++ )
	{
		counter = 0;
		while ( digitalRead( DHTPIN ) == laststate )
		{
			counter++;
			delayMicroseconds( 3 );
			if ( counter == 255 )
				break;
		}
		laststate = digitalRead( DHTPIN );
 
		if ( counter == 255 )
			break;
 
		if ( (i >= 4) && (i % 2 == 0) )
		{
			dht_dat[j / 8] <<= 1;
			if ( counter > 16 )
				dht_dat[j / 8] |= 1;
			j++;
		}
	}
 
	return ( (j >= 40) &&
	     (dht_dat[4] == ( (dht_dat[0] + dht_dat[1] + dht_dat[2] + dht_dat[3]) & 0xFF) ) );
	  
  }

  /***********************************************************************/

  double DhtReader::humidex(double temp, double hum)
  // Zone confort entre 19 et 29
  {
    double t=7.5*temp/(237.7+temp);
    double ex=6.112*pow(10,t)*(hum/100);
    double h=temp+(5.0/9)*(ex-10);
    if (h < temp)
      return temp;

    return h;
  }

  /***********************************************************************/
 
  void DhtReader::loop()
  { 
    while ( !exiting )
    {

      if (read_dht_dat())
      {
        humi = (double)((dht_dat[0] << 8) + dht_dat[1]) / 10;
        if ( humi > 100 )
          humi = dht_dat[0];	// for DHT11

        temp = (double)(((dht_dat[2] & 0x7F) << 8) + dht_dat[3]) / 10;

        if ( temp > 125 )								
          temp = dht_dat[2];	// for DHT11

        if ( dht_dat[2] & 0x80 )
	      temp = -temp;
      
        humideX = humidex (temp,humi);

        NVJ_LOG->append(NVJ_DEBUG, string ("new Dht Values: Humidity = ") + to_string(humi) + ", Temperature = " + to_string(temp) + "°C humidex = " + to_string(humideX) );
      }

      delay( 2000 );
    }
 
  }

  /***********************************************************************/


