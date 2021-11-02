//------------------------------------------------------------------------------
//
// LogWebBuffer.hh : Get/Put last messages for web Itf
//
//------------------------------------------------------------------------------
//
// AUTHOR      : T.DESCOMBES (descombt@ipnl.in2p3.fr)
// PROJECT     : EventBuilder
//
//------------------------------------------------------------------------------
//           Versions and Editions  historic
//------------------------------------------------------------------------------
// Ver      | Ed  |   date   | resp.       | comment
//----------+-----+----------+-------------+------------------------------------
// 1        | 1   | 10/12/02 | T.DESCOMBES | source creation
//----------+-----+----------+-------------+------------------------------------
//
//           Detailed modification operations on this source
//------------------------------------------------------------------------------
//  Date  :                            Author :
//  REL   :
//  Description :
//
//------------------------------------------------------------------------------

#ifndef LOGWEBBUFFER_HH_
#define LOGWEBBUFFER_HH_

#include <pthread.h>

#include "libnavajo/LogOutput.hh"

#define WEBBUFFER_NBLINES 5000

using namespace std;

/**
* LogWebBuffer - LogOutput
*/
class LogWebBuffer : public LogOutput
{
  public:
    LogWebBuffer ();
    ~LogWebBuffer ();

    void append ( const NvjLogSeverity &l, const std::string &m, const std::string &details );
    void initialize ();
    string getJsonLogBuffer ( const int decalageHoraire, int prevId = -1 );

  private:
    string logMessages[WEBBUFFER_NBLINES]; // 150 lignes
    string logDetails[WEBBUFFER_NBLINES]; // 150 lignes
    NvjLogSeverity logSeverity[WEBBUFFER_NBLINES];
    unsigned firstLine, nextLine;
    pthread_mutex_t weblog_mutex;

};


#endif
