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

#include "DhtReader.hh"

// dispo : 8 9 11 15 16 17 18 19 20
#define DHTPIN		11 // GPIO7 (SPI_CE1_N)
#define MAXTIMINGS	95


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
			{
				break;
			}
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
 
    if ( wiringPiSetup() == -1 )
      exit( 1 );
 
    while ( true )
    {

      if (read_dht_dat())
      {
        float humi = (float)((dht_dat[0] << 8) + dht_dat[1]) / 10;
        if ( humi > 100 )
          humi = dht_dat[0];	// for DHT11

        double temp = (double)(((dht_dat[2] & 0x7F) << 8) + dht_dat[3]) / 10;

        if ( temp > 125 )								
          temp = dht_dat[2];	// for DHT11

        if ( dht_dat[2] & 0x80 )
	      temp = -temp;
      
        double humideX = humidex (temp,humi);
        printf( "Humidity = %f %% Temperature = %f C humidex = %f\n", 
                        humi, temp, humideX );
      // updateRunning();
      }

      delay( 2000 );
    }
 
  }

  /***********************************************************************/

  void DhtReader::startThread()
  {
  }

  /***********************************************************************/

  void DhtReader::stopThread()
  {
  }

