//------------------------------------------------------------------------------
//
//  LogWebBuffer.cc : Get/Put last messages for web Itf
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


#include <stdio.h>
#include "WebsocketEventNotifier.hh"

#ifdef WIN32
#include "strptime.h"
#endif

#include "rapidjson/document.h"		// rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"	// for stringify JSON
#include "rapidjson/filewritestream.h"	// wrapper of C stream for prettywriter as output
#include "rapidjson/stringbuffer.h"

#include "LogWebBuffer.hh"
#include "AlertManager.hh"
#include "ZNeTSparams.hh"

using namespace rapidjson;

  /***********************************************************************/
  /**
  * append - append a message
  * \param l - LogSeverity
  * \param m - message
  */
  void LogWebBuffer::append(const NvjLogSeverity& l, const std::string& message, const std::string &details)
  {
    if (l == NVJ_DEBUG)
 	return;

    pthread_mutex_lock( &weblog_mutex );

    string msg=message;
    string::size_type found = msg.find_first_of("\"\n\t");

    while( found != string::npos ) 
    {
      msg.erase(found,1);
      found = msg.find_first_of("\"\n\t",found);
    }

    string det=details;
    found = det.find_first_of("\"\n\t");

    while( found != string::npos ) 
    {
      char c=det[found];
      det.erase(found,1);
      if (c == '\n') det.insert(found++,"\\n");  
      found = det.find_first_of("\"\n\t",found);
    }

    logMessages[nextLine] = msg;
    logDetails[nextLine]  = det;
    logSeverity[nextLine] = l;

    WebsocketEventNotifier::getInstance()->newLogEntry(nextLine, l, message, details);

    nextLine=(nextLine+1) % WEBBUFFER_NBLINES;
    if (firstLine == nextLine)
      firstLine=(firstLine+1) % WEBBUFFER_NBLINES;


    pthread_mutex_unlock( &weblog_mutex );

  }

/***********************************************************************/
  /**
  *  initialize the logoutput
  */ 

  void LogWebBuffer::initialize()
  {
    firstLine=0;
    nextLine=0;
    pthread_mutex_init(&weblog_mutex, NULL);
  }

  /***********************************************************************/
  /**
  *  initialize the logoutput
  */ 

  std::string LogWebBuffer::getJsonLogBuffer(const int decalageHoraire, int prevId)
  {
    unsigned i;

    std::string resultat;

    if (prevId >= WEBBUFFER_NBLINES)
      return "";


    GenericStringBuffer< UTF8 < > > buffer;
    Writer<GenericStringBuffer< UTF8 < > > > writer(buffer);

    writer.StartObject();

    writer.String("content");
    writer.StartArray();
    writer.String("severity");
    writer.String("date");
    writer.String("msg");
    writer.String("detail");
    writer.EndArray();
    
    writer.String("data");
    writer.StartArray();
    
    pthread_mutex_lock( &weblog_mutex );
    
  try
  {
    if (prevId < 0) 
      i=firstLine;
    else
      i=prevId;

    for (; i != nextLine; i=(i+1)%WEBBUFFER_NBLINES)
    {
      writer.StartArray();
    
      switch ( logSeverity[i] )
      {
	case NVJ_DEBUG:
	  writer.String("DEBUG");
	  break;
        case NVJ_WARNING:
	  writer.String("WARNING");
	  break;
	case NVJ_ALERT:
	  writer.String("ALERT");
	  break;
        case NVJ_ERROR:
	  writer.String("ERROR");
	  break;
        case NVJ_FATAL:
	  writer.String("FATAL");
	  break;
        case NVJ_INFO:
	  writer.String("INFO");
	  break;
	default:
	  writer.String("UNKNOWN");
	  break;
      }

      string message=logMessages[i].substr(23);

      struct tm tm;
      strptime(logMessages[i].substr(1,17).c_str(), "%Y-%m-%d %H:%M:%S", &tm);
      tm.tm_isdst=-1;
      tm.tm_sec=0;
      time_t t=mktime(&tm)+3600*decalageHoraire;
      char dateLog[128];
      localtime_r ( &t, &tm );
      strftime( dateLog, 128, "%Y-%m-%d %H:%M:%S", &tm );
      writer.String(dateLog);

      writer.String(message.c_str());

      writer.String(logDetails[i].c_str());

      writer.EndArray();

    }
    writer.EndArray();

    writer.String("nextId");
    writer.Int((int)nextLine);
    
    if (ZNeTSparams::getZcurrentConfig()->getDBMS() != NO_DBMS) // Alert in the log ? No -> add AlertIdx
    {
      writer.String("lastAlertIdx");
      writer.Int((int)AlertManager::getInstance()->getLastAlertIdx());
    }
    writer.EndObject();
  
    resultat=buffer.GetString();
    buffer.Clear();
  }
  catch(...)
  {
    resultat = "{ errMsg:\"Bad index\" }";
  }
  
  pthread_mutex_unlock( &weblog_mutex );

  return resultat;

  }

  /***********************************************************************/
  /**
  * LogWebBuffer - constructor
  */ 
  
  LogWebBuffer::LogWebBuffer()
  {
  }

  /***********************************************************************/
  /**
  * ~LogRecorder - destructor
  */   

  LogWebBuffer::~LogWebBuffer()
  {
     AlertManager::freeInstance();
  }

  /***********************************************************************/


