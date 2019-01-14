//------------------------------------------------------------------------------
//
// PelletService.hh : Press button of pellet stove controller
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

#ifndef PELLETSERVICE_HH_
#define PELLETSERVICE_HH_

#include <string>

#include "libnavajo/libnavajo.hh"
#include "libnavajo/LogStdOutput.hh"

#include "ButtonControl.hh"
#include "DhtReader.hh"
#include "LcdReader.hh"
#include "OpenWeatherClient.hh"



  /****************************************************************************
  * PelletInfoMonitor - libnavajo DynamicPage 
  ****************************************************************************/

  class PelletInfoMonitor: public DynamicPage
  { 
    bool getPage(HttpRequest* request, HttpResponse *response); 

    DhtReader* dhtReader;
    LcdReader* lcdReader;
    OpenWeatherClient* openWeatherClient;

    public:
      PelletInfoMonitor(DhtReader *dr, LcdReader *lr, OpenWeatherClient *owc): 
          dhtReader(dr), lcdReader (lr), openWeatherClient(owc) {};
  };

  /****************************************************************************
  * PelletCommand - libnavajo DynamicPage 
  ****************************************************************************/

  class PelletCommand: public DynamicPage
  { 
    bool getPage(HttpRequest* request, HttpResponse *response); 

    ButtonControl* buttonControl;

    public:
      PelletCommand(ButtonControl *bc): 
          buttonControl(bc) {};
  };

  /****************************************************************************
  * PelletService - generic class to handle REST API
  ****************************************************************************/

  class PelletService : public DynamicRepository
  {

    PelletInfoMonitor *pelletInfoMonitor = nullptr;
    PelletCommand *pelletCommand = nullptr;

    ButtonControl buttonControl;
    DhtReader dhtReader;
    LcdReader lcdReader;
    OpenWeatherClient openWeatherClient;

    public:
      PelletService();
      ~PelletService();
  };

  

#endif
