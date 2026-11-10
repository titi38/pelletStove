#include <unistd.h>
//------------------------------------------------------------------------------
//
// OpenWeatherClient.cc : exec command
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

#include <fstream>
#include <sstream>
#include <string.h>
#include <iostream>

#include "OpenWeatherClient.hh"
#include "Stats.hh"

#include <curl/curl.h>
#include <cstdlib>
#include <cstdint>
#include <new>
#include <cstring>
#include <mutex>
#include <stdexcept>

#include "rapidjson/document.h"      // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"    // for stringify JSON
#include "rapidjson/filewritestream.h"  // wrapper of C stream for prettywriter as output
#include "rapidjson/stringbuffer.h"


using namespace rapidjson;

//------------------------------------------------------------------------------

OpenWeatherClient *OpenWeatherClient::theOpenWeatherClient = NULL;

//------------------------------------------------------------------------------

OpenWeatherClient::OpenWeatherClient ()
{
  exiting = false;
  thread_loop = new thread (&OpenWeatherClient::loop, this);
  NVJ_LOG->append (NVJ_INFO, "OpenWeatherClient is starting");
}

OpenWeatherClient::~OpenWeatherClient ()
{
  exiting = true;
  thread_loop->join ();
}

void OpenWeatherClient::loop ()
{
  while ( !exiting )
  {
    if ( nextDataAvailable == 0 )
    {
      char *resultJson = nullptr;
      char *beuuug = nullptr;
      try
      {
	beuuug = send_get_http_query ("api.weatherapi.com", "https://api.weatherapi.com/v1/current.json","key=fdbbf4743489484285b115210242809&q=45.36,5.67&aqi=no");
        resultJson = beuuug; // libcurl returns the JSON body, not HTTP headers.
      }
      catch (const std::exception &e)
      {
        NVJ_LOG->append(NVJ_ERROR,
            string("OpenWeatherClient HTTP request failed: ") + e.what());
        sleep(10);
        continue;
      }
      if (!resultJson)
      {
        NVJ_LOG->append(NVJ_ERROR, "OpenWeatherClient empty HTTP response");
        sleep(10);
        continue;
      }
      // ---------------------------------

      Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.
      if ( document.Parse<0> (resultJson).HasParseError () )
      {
        NVJ_LOG->append (NVJ_DEBUG, "OpenWeatherClient bad json document", resultJson);
//printf("%s", resultJson);
        free(beuuug);
        sleep(10);
        continue;
      }

      lock_guard <std::mutex> lk (mutex_values);



      // refresh 15mn
//      nextDataAvailable = 450;

      if (document.HasMember("current") && document["current"].IsObject())
      {
        const rapidjson::Value& current_value = document["current"];
	if (current_value.HasMember("temp_c") && current_value["temp_c"].IsNumber())
          temp = current_value[ "temp_c" ].GetDouble ();
        else temp = 0;

        if (current_value.HasMember("humidity") && current_value["humidity"].IsNumber())
          humi = current_value[ "humidity" ].GetInt ();
        else humi = 0;

        if (current_value.HasMember("temp_c") && current_value["temp_c"].IsNumber()
            && current_value.HasMember("humidity") && current_value["humidity"].IsNumber())
          Stats::getInstance()->updateOutdoorStats(temp, humi);
 
        if (current_value.HasMember("condition") && current_value["condition"].IsObject()) 
        {	
          const rapidjson::Value& condition_value = current_value["condition"];
	  if (condition_value.HasMember("icon") && condition_value["icon"].IsString())
            icon = condition_value[ "icon" ].GetString(); 
	  if (condition_value.HasMember("code") && condition_value["code"].IsNumber())
	    clearForcast =  condition_value[ "code" ].GetInt () <= 1006 ;
        }

        if (current_value.HasMember("wind_kph") && current_value["wind_kph"].IsNumber()) 
          wind_speed = current_value[ "wind_kph" ].GetDouble ();
        else wind_speed = 0;
        
        if (current_value.HasMember("wind_degree") && current_value["wind_degree"].IsNumber())
            wind_dir   = current_value["wind_degree"].GetDouble();
        else wind_dir = 0;
      }


      // ---------------------------------

free(beuuug);
     // free (resultJson);
    }
    else
    {
      nextDataAvailable--;
      sleep (1);
    }

sleep(450);
  }
}

//------------------------------------------------------------------------------

string OpenWeatherClient::getInfoJson () const
{
  lock_guard <std::mutex> lk (mutex_values);
  string resultat = "";
  char timeString[6];  // HH:MM nécessite 5 caractères + 1 pour le terminateur null

  GenericStringBuffer <UTF8<>> buffer;
  Writer <GenericStringBuffer<UTF8<> >> writer (buffer);
 
  writer.StartObject ();
 
  writer.Key ("temp");
  writer.Double (temp);
  writer.Key ("humi");
  writer.Int (humi);
 
  writer.Key ("wind_speed");
  writer.Int (wind_speed);
  writer.Key ("wind_dir");
  writer.Int (wind_dir);
 
 /* writer.Key ("sunrise");
  strftime(timeString, sizeof(timeString), "%H:%M", &sunrise);
  writer.String (timeString);
  writer.Key ("sunset");
  strftime(timeString, sizeof(timeString), "%H:%M", &sunset);
  writer.String (timeString);*/

  writer.Key ("icon");
  string iconURL = "http://" + icon;
  writer.String (iconURL.c_str ());
 
  writer.EndObject ();
  resultat = buffer.GetString ();
  buffer.Clear ();
  return resultat;
}

//------------------------------------------------------------------------------

namespace {
constexpr size_t MAX_WEATHER_RESPONSE = 512 * 1024;

size_t writeWeatherResponse(char *data, size_t size, size_t count, void *user)
{
    auto *body = static_cast<std::string *>(user);
    if (size && count > SIZE_MAX / size) return 0;
    const size_t len = size * count;
    if (len > MAX_WEATHER_RESPONSE - body->size()) return 0;
    body->append(data, len);
    return len;
}

char *weatherRequest(const std::string &url, const char *postData)
{
    static std::once_flag curlInit;
    std::call_once(curlInit, [] {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
            throw std::runtime_error("curl_global_init failed");
    });

    CURL *curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");
    std::string body;
    char error[CURL_ERROR_SIZE] = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeWeatherResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PelletStove/WeatherClient");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    if (postData) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
    const CURLcode result = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    const std::string failure = error[0] ? error : curl_easy_strerror(result);
    curl_easy_cleanup(curl);
    // Never include the URL in errors: it contains the WeatherAPI key.
    if (result != CURLE_OK)
        throw std::runtime_error(std::string("libcurl: ") + failure);
    if (status != 200)
        throw std::runtime_error("WeatherAPI HTTP status " + std::to_string(status));
    if (body.empty()) throw std::runtime_error("WeatherAPI returned an empty body");
    char *buffer = static_cast<char *>(std::malloc(body.size() + 1));
    if (!buffer) throw std::bad_alloc();
    std::memcpy(buffer, body.data(), body.size());
    buffer[body.size()] = '\0';
    return buffer; // caller uses free()
}
} // namespace

char *OpenWeatherClient::send_get_http_query(
    const string &hostname, const string &url, const string &param)
{
    (void)hostname;
    const string fullUrl = url + (param.empty() ? "" : "?" + param);
    return weatherRequest(fullUrl, nullptr);
}

char *OpenWeatherClient::send_post_http_query(
    const string &hostname, const string &url, const string &param)
{
    (void)hostname;
    return weatherRequest(url, param.c_str());
}
