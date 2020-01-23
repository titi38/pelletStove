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
#include <thread>
#include "libnavajo/libnavajo.hh"


using namespace std;


  /**
  * DhtReader - generic class to handle the relay
  */
  class DhtReader
  {
    int dht_dat[5] = { 0, 0, 0, 0, 0 };
    bool read_dht_dat();
    void loop();

    thread *thread_loop=nullptr;
    volatile bool exiting = false;

    volatile double humi=1234.0, temp=1234.0;

    public:

      DhtReader();
      ~DhtReader();
      double getTemp() const { return temp; };
      double getHumi() const { return humi; };
      string getInfoJson() const;

  };
  

#endif
