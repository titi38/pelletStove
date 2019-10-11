//------------------------------------------------------------------------------
//
// OpenWeatherClient.hh : Press button of pellet stove controller
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

#ifndef OPENWEATHERCLIENT_HH_
#define OPENWEATHERCLIENT_HH_

#include <string>

using namespace std;


#include <stdio.h>
#include <strings.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <iomanip>
#include <sstream>

#include <thread>
#include <mutex>

#include "libnavajo/libnavajo.hh"


using namespace std;


  /**
  * OpenWeatherClient - generic class to handle the relay
  */

class OpenWeatherClient /**< LocalFlowEntry datatype definition */ 
{
    static OpenWeatherClient *theOpenWeatherClient;
    static const unsigned char ca_gz[];
    BIO *sbio;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    X509 *server_cert;
    bool connected = false;

    bool matches_common_name(const char *hostname, const X509 *server_cert);
    bool validate_hostname(const char *hostname, const X509 *server_cert);
    void free_ssl();
    char *send_http_query (const string& hostname, const string &query);

    mutable mutex mutex_icon;
    thread *thread_loop=nullptr;
    volatile bool exiting = false;
    void loop();

    int nextDataAvailable = 0;

    double temp=0, wind_speed=0, wind_dir=0;
    int humi=0;
    string icon;


  public:
    OpenWeatherClient ();
    ~OpenWeatherClient ();
    string getInfoJson() const;
    bool isClearForcast() const { return icon == "01d" || icon == "02d" ; };

    char *send_get_http_query (const string &hostname, const string &url,  const string &param=NULL);
    char *send_post_http_query (const string &hostname, const string &url, const string &param);
    void connect(const string &hostname, const unsigned short port );
    void disconnect();

    class httpParam
    {
      string paramStr;

      inline static string convert(const string &str)
      {
        string res;
        for (size_t i=0; i<str.length(); i++)
          if (isalnum(str[i]))
            res+=str[i];
          else
          {
            stringstream codess; codess << setfill('0') << setw(2) << hex << (int)str[i];
            res+="%"+codess.str();
          }
        return res;
      };

      public:
        inline void addValue(const char *var, const char *val)
        {
          if (paramStr.length()) paramStr+="&";
          paramStr+=convert(var)+"="+convert(val);
        };
        inline string str()
        {
          return paramStr;
        };
    };
};

  

#endif
