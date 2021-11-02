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
#include <mutex>

#include "libnavajo/libnavajo.hh"
#include "libnavajo/LogStdOutput.hh"

#include "ButtonControl.hh"
#include "DhtReader.hh"
#include "LcdReader.hh"
#include "OpenWeatherClient.hh"
#include "Gauge.hh"
#include "AutoMode.hh"


/****************************************************************************
* PelletInfoMonitor - libnavajo DynamicPage
****************************************************************************/

class PelletInfoMonitor : public DynamicPage
{
    bool getPage ( HttpRequest *request, HttpResponse *response );

    DhtReader *dhtReader;
    LcdReader *lcdReader;
    Gauge *gauge;
    OpenWeatherClient *openWeatherClient;
    AutoMode *autoMode;

  public:
    PelletInfoMonitor ( DhtReader *dr, LcdReader *lr, Gauge *g, OpenWeatherClient *owc, AutoMode *am ) :
        dhtReader (dr), lcdReader (lr), gauge (g), openWeatherClient (owc), autoMode (am)
    { };
};

/****************************************************************************
* PelletCommand - libnavajo DynamicPage
****************************************************************************/

class PelletCommand : public DynamicPage
{
    bool getPage ( HttpRequest *request, HttpResponse *response );

    ButtonControl *buttonControl;
    LcdReader *lcdReader;
    AutoMode *autoMode;

    mutable mutex mutex_command;

  public:
    PelletCommand ( ButtonControl *bc, LcdReader *lr, AutoMode *am ) :
        buttonControl (bc), lcdReader (lr), autoMode (am)
    { };
};

/****************************************************************************
* PelletService - generic class to handle REST API
****************************************************************************/

class PelletService : public DynamicRepository
{

    PelletInfoMonitor *pelletInfoMonitor = nullptr;
    PelletCommand *pelletCommand = nullptr;

    ButtonControl *buttonControl = nullptr;
    DhtReader *dhtReader = nullptr;
    LcdReader *lcdReader = nullptr;
    Gauge *gauge = nullptr;
    OpenWeatherClient openWeatherClient;
    AutoMode *autoMode = nullptr;

  public:
    PelletService ();
    ~PelletService ();
};


#endif
