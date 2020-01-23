//------------------------------------------------------------------------------
//
// Gauge.hh : exec command
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


  #include <string>
  #include <thread>

  #define GAUGE_NBVAL 10

  using namespace std;

  /**
  * Gauge - generic class to mesure pellet level
  */
  class Gauge
  {

    enum class Sensors : int { trigger=27 /*BCM 16*/, echo=28/*BCM 20*/};

    void loop();

    thread *thread_loop=nullptr;
    volatile bool exiting = false;

    volatile double histDistance[GAUGE_NBVAL];
    size_t nbDistance = 0;
    int temperature = 20;

    void readMesure();
    double getAvgDistance() const;



    public:

      Gauge();
      ~Gauge();
      string getInfoJson() const;
      double getLevel() const;
  };

