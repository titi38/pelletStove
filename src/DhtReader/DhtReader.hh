//------------------------------------------------------------------------------
//
// DhtReader.hh : Press button of pellet stove controller
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

#ifndef DHTREADER_HH_
#define DHTREADER_HH_

#include <string>

using namespace std;


  /**
  * DhtReader - generic class to handle the relay
  */
  class DhtReader
  {
    int dht_dat[5] = { 0, 0, 0, 0, 0 };
    bool read_dht_dat();
    double humidex(double temp, double hum);
    void loop();


    public:

      void startThread();
      void stopThread();

  };
  

#endif
