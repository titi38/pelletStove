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

#include "libnavajo/nvjGzip.h"

#include "rapidjson/document.h"    	// rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"  	// for stringify JSON
#include "rapidjson/filewritestream.h"  // wrapper of C stream for prettywriter as output
#include "rapidjson/stringbuffer.h"


using namespace rapidjson;

#define SECURE_CIPHER_LIST "RC4-SHA:HIGH:!ADH:!AECDH:!CAMELLIA"

//------------------------------------------------------------------------------

OpenWeatherClient * OpenWeatherClient::theOpenWeatherClient = NULL;

//------------------------------------------------------------------------------

OpenWeatherClient::OpenWeatherClient()
{
    exiting = false;
    thread_loop=new thread(&OpenWeatherClient::loop, this);
    NVJ_LOG->append(NVJ_INFO, "OpenWeatherClient is starting" );
}

OpenWeatherClient::~OpenWeatherClient()
{
    exiting = true;
    thread_loop->join();
}

void OpenWeatherClient::loop()
{
  while ( !exiting )
  {
    if (nextDataAvailable == 0)
    {
      char *resultJson = nullptr;
      try
      {
        resultJson = send_get_http_query ("api.openweathermap.org", "https://api.openweathermap.org/data/2.5/weather",
                                                "lat=45.37&lon=5.7&units=metric&cnt=2&APPID=2af48f565fec5c7cdb3600a6270fef5c");
      }
      catch(...)
      {
        NVJ_LOG->append(NVJ_ERROR, "OpenWeatherClient failed to send http request" );
        sleep(10);
        continue;
      }

      //  printf(" res1:\n%s\n",resultJson); fflush(NULL);
      // ---------------------------------

      Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.
      if ( document.Parse<0>( resultJson ).HasParseError()
        || !document.HasMember("weather")
        || !document.HasMember("main")
        || !document.HasMember("wind") )
      {
        NVJ_LOG->append(NVJ_ERROR, "OpenWeatherClient bad json document" );

        free(resultJson);
        disconnect();
        continue ;
      }

      lock_guard<std::mutex> lk(mutex_icon);

      // refresh 10mn
      nextDataAvailable = 300;
      temp = document["main"]["temp"].GetDouble();
      humi = document["main"]["humidity"].GetInt();
      wind_speed = document["wind"]["speed"].GetDouble(), 
      wind_dir   = document["wind"]["deg"].GetDouble();
      icon = string("http://openweathermap.org/img/w/") + document["weather"][0]["icon"].GetString() + ".png";

      // ---------------------------------

      free(resultJson);
      disconnect();

    }
    else
    {
      nextDataAvailable --;
      sleep(1);
    }
  }
}

//------------------------------------------------------------------------------

string OpenWeatherClient::getInfoJson() const
{ 
    lock_guard<std::mutex> lk(mutex_icon);
    string resultat = "";
    GenericStringBuffer<UTF8<> > buffer;
    Writer<GenericStringBuffer<UTF8<> > > writer( buffer );
    writer.StartObject();
    writer.String( "temp" );
    writer.Double( temp );
    writer.String( "humi" );
    writer.Int( humi );
    writer.String( "wind_speed" );
    writer.Int( wind_speed );
    writer.String( "wind_dir" );
    writer.Int( wind_dir );
    writer.String( "icon" );
    writer.String( icon.c_str() );
    writer.EndObject();
    resultat = buffer.GetString();
    buffer.Clear();
    return resultat;
} 

//------------------------------------------------------------------------------

bool OpenWeatherClient::matches_common_name(const char *hostname, const X509 *server_cert)
{
	int common_name_loc = -1;
	X509_NAME_ENTRY *common_name_entry = NULL;
	ASN1_STRING *common_name_asn1 = NULL;
	char *common_name_str = NULL;

	// Find the position of the CN field in the Subject field of the certificate
	common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name((X509 *) server_cert), NID_commonName, -1);
	if (common_name_loc < 0) 
		return false;

	// Extract the CN field
	common_name_entry = X509_NAME_get_entry(X509_get_subject_name((X509 *) server_cert), common_name_loc);
	if (common_name_entry == NULL)
		return false;

	// Convert the CN field to a C string
	common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
	if (common_name_asn1 == NULL)
		return false;
		
	common_name_str = (char *) ASN1_STRING_data(common_name_asn1);

	// Make sure there isn't an embedded NUL character in the CN
	if (ASN1_STRING_length(common_name_asn1) != (int)strlen(common_name_str))
	  return false;

	// Compare expected hostname with the CN
	if (strcasecmp(hostname, common_name_str) == 0)
	  return true;
	else
	  return false;
}

//------------------------------------------------------------------------------

bool OpenWeatherClient::validate_hostname(const char *hostname, const X509 *server_cert)
{
	bool result = false;

	if((hostname == NULL) || (server_cert == NULL))
	  return false;

	// First try the Subject Alternative Names extension
	
	// Try to extract the names within the SAN extension from the certificate
	STACK_OF(GENERAL_NAME) *san_names =  (stack_st_GENERAL_NAME*)X509_get_ext_d2i((X509 *) server_cert, NID_subject_alt_name, NULL, NULL);
	if (san_names == NULL) // NoSANPresent 
	{
//	  printf("Extension was not found: try the Common Name\n"); fflush(NULL);
	  return matches_common_name(hostname, server_cert);
	}
	else
	{
	  // Check each name within the extension
	  for (int i=0; i < sk_GENERAL_NAME_num(san_names) ; i++)
          {
	    const GENERAL_NAME *current_name = (const GENERAL_NAME*) sk_GENERAL_NAME_value(san_names, i);

	    if (current_name->type == GEN_DNS) 
	    {
	      // Current name is a DNS name, let's check it
	      char *dns_name = (char *) ASN1_STRING_data(current_name->d.dNSName);

	      // Make sure there isn't an embedded NUL character in the DNS name
	      if (ASN1_STRING_length(current_name->d.dNSName) != (int)strlen(dns_name)) 
              {
	        result = false;
	        break;
	      }
              else 
              { // Compare expected hostname with the DNS name
	        if (strcasecmp(hostname, dns_name) == 0) 
                {
		  result = true;
	          break;
	        }
	      }
	    }
	  }
	  sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
	}

	return false;
}

//------------------------------------------------------------------------------

void OpenWeatherClient::disconnect()
{
  free_ssl();
  connected=false;
}

//------------------------------------------------------------------------------

void OpenWeatherClient::connect(const string &hostname, const unsigned short tcpport) 
{
  if (connected) return;

  sbio=NULL;
  ssl_ctx=NULL;
  ssl=NULL;
  server_cert=NULL;

  // Initialize OpenSSL
  SSL_library_init();
  SSL_load_error_strings();

  // Check OpenSSL PRNG
  if(RAND_status() != 1)
  {
    free_ssl();	
    throw std::runtime_error(std::string("Connection to znets website failed (OpenSSL PRNG seeds issue)") );
  }

  ssl_ctx = SSL_CTX_new(TLSv1_client_method());
	
  // Enable certificate validation
  //SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, NULL);
  // Configure the CA trust store to be used
  //if (SSL_CTX_load_verify_locations(ssl_ctx, TRUSTED_CA_PATHNAME, NULL) != 1)
  //{
  //  free_ssl();	
  //  throw std::runtime_error(std::string("Connection to znets website failed (couldn't load certificate)") );
  //}

  // Only support secure cipher suites
  if (SSL_CTX_set_cipher_list(ssl_ctx, SECURE_CIPHER_LIST) != 1)
  {
    free_ssl();	
    throw std::runtime_error(std::string("Connection to znets website failed (unsupported cipher suites)") );
  }

  // Create the SSL connection
  sbio = BIO_new_ssl_connect(ssl_ctx);
  BIO_get_ssl(sbio, &ssl); 
  if(!ssl)
  {
    free_ssl();	
    throw std::runtime_error(std::string("Connection to znets website failed (SSL connect issue)") );
  }

  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

  // Do the SSL handshake
  string server = hostname+':'+to_string(tcpport);
  BIO_set_conn_hostname(sbio, server.c_str());
/*  if(SSL_do_handshake(ssl) <= 0)
  {
    // SSL Handshake failed
    long verify_err = SSL_get_verify_result(ssl);
    if (verify_err != X509_V_OK) 
    { 
      // It failed because the certificate chain validation failed
      free_ssl();	
      throw std::runtime_error(std::string("Connection to znets website failed (certificate chain validation failed: ") 
                    + X509_verify_cert_error_string(verify_err) + std::string(")"));
    }
    else 
    {
      // It failed for another reason
      free_ssl();	
      throw std::runtime_error(std::string("Connection to znets website failed"));
    }

    return;
  }

  
  // Recover the server's certificate
  server_cert =  SSL_get_peer_certificate(ssl);
  if (server_cert == NULL)
  {
    // The handshake was successful although the server did not provide a certificate
    // Most likely using an insecure anonymous cipher suite... get out!
    free_ssl();	
    throw std::runtime_error(std::string("Connection to znets website failed (server authentification failed)") );
  }
printf("4\n"); fflush(NULL);
  // Validate the hostname
  if (!validate_hostname(hostname.c_str(), server_cert)) 
  {
    free_ssl();
    throw std::runtime_error(std::string("Connection to znets website failed (hostname validation failed)") );
  }
*/
 // removeCA();
  connected=true;
}

//------------------------------------------------------------------------------

void OpenWeatherClient::free_ssl()
{
  if (server_cert != NULL)
    X509_free(server_cert);
  if (sbio != NULL)
  {
    BIO_ssl_shutdown(sbio);
    BIO_free_all(sbio);
  }
  if (ssl_ctx != NULL)
    SSL_CTX_free(ssl_ctx);

  EVP_cleanup();
  ERR_free_strings();
}

//------------------------------------------------------------------------------

char *OpenWeatherClient::send_get_http_query (const string &hostname, const string &url, const string &param)
{
  string query="GET "+url;

  if (param.length())
    query+="?"+param;

  query+=" HTTP/1.1\r\nHost: "+hostname+"\r\nConnection: keep-alive\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nUser-Agent: libnavajo\r\nAccept-Encoding: gzip\r\nAccept-Language: fr-FR,fr;q=0.8,en-US;q=0.6,en;q=0.4\r\n\r\n";

  return send_http_query (hostname, query);
}

//------------------------------------------------------------------------------

char *OpenWeatherClient::send_post_http_query (const string &hostname, const string &url, const string &param)
{
  stringstream lenss; lenss << param.length();

  string query="POST "+url+" HTTP/1.1\r\nHost: "+hostname+"\r\nConnection: keep-alive\r\nContent-Length: "+lenss.str()+"\r\nUser-Agent: libnavajo\r\nContent-Type: application/x-www-form-urlencoded; charset=UTF-8\r\nAccept: */*\r\nReferer: https://"+hostname+"/\r\nAccept-Encoding: gzip\r\nAccept-Language: fr-FR,fr;q=0.8,en-US;q=0.6,en;q=0.4\r\n\r\n"+param+"\r\n";

  return send_http_query (hostname, query);
}

//------------------------------------------------------------------------------

char *OpenWeatherClient::send_http_query (const string &hostname, const string &query)
{ 
  char *res=NULL;
  int len;
  char tmpbuf[4096];
  char *startBuf= NULL;

  if (!connected)
    try
    {
      connect(hostname, 443);
    }
    catch(std::runtime_error& e)
    {
      printf("%s\n", e.what()); fflush(NULL);
    }

  while (BIO_write(sbio, query.c_str(), query.length()) <= 0)
  {
    if(! BIO_should_retry(sbio))
    {
      throw std::runtime_error(std::string("Connection to znets website failed (BIO_write)") );
      return NULL;
    }
    // retry
  }

  BIO_flush(sbio);

  bool header=true, protoOK=false, gzip=false;
  int contentLength=0, offset=0;
  int retCode=0;

  do
  {
    len = BIO_read(sbio, tmpbuf, sizeof tmpbuf);

    if (header && len < 20)
    { 
      throw std::runtime_error(std::string("Connection to znets website failed (too short answer)") );
      return NULL; 
    }
      

    if (header)
    {
      char *protostr = strcasestr(tmpbuf,"HTTP/1.");
      if (!protoOK && (protostr == NULL) )
      { 
        throw std::runtime_error(std::string("Connection to znets website failed (Unknown protocol)") );
        return NULL; 
      }
      else
      {
        retCode=atoi(protostr+9);
	//printf("retCode=%d\n",retCode); fflush(NULL);
        if (retCode != 200)
        { 
          throw std::runtime_error(std::string("Connection to znets website failed (retCode != 200)") );
          return NULL; 
        }          
      }

      char *lengthStr = strcasestr(tmpbuf, "Content-Length: ");
      if (lengthStr != NULL) 
      {
        contentLength = atoi(lengthStr+16);
        res=(char*)malloc((contentLength+1)*sizeof(char));  
      }

      if (!gzip)
        if (strcasestr(tmpbuf, "Content-Encoding: gzip") != NULL)
          gzip = true;

      char *headerEnd = strstr(tmpbuf,"\r\n\r\n");
      if (headerEnd == NULL )
        continue;
      header=false;

      startBuf = headerEnd + 4;
    }
    else
      startBuf = tmpbuf;

    if (len > startBuf - tmpbuf)
    {
      memcpy(res+offset, startBuf, len - (startBuf - tmpbuf));
      offset += len;
    }
  }
  while ( offset < contentLength && (len > 0  || (len < 0 && BIO_should_retry(sbio)) ) );

  *(res+contentLength)='\0';

  if (len == 0) BIO_reset(sbio);

  if (gzip) nvj_gunzip( (unsigned char**)&res, (unsigned char*)res, contentLength );

  return res;
}



