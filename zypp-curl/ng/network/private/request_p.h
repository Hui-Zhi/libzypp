/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
----------------------------------------------------------------------/
*
* This file contains private API, this might break at any time between releases.
* You have been warned!
*
*/
#ifndef ZYPP_NG_MEDIA_CURL_PRIVATE_REQUEST_P_H_INCLUDED
#define ZYPP_NG_MEDIA_CURL_PRIVATE_REQUEST_P_H_INCLUDED

#include <zypp-core/zyppng/base/private/base_p.h>
#include <zypp-curl/ng/network/request.h>
#include <zypp-media/MediaException>
#include <zypp-core/zyppng/base/Timer>
#include <zypp-core/base/Regex.h>
#include <curl/curl.h>
#include <array>
#include <memory>
#include <zypp-core/Digest.h>
#include <zypp-core/AutoDispose.h>

#include <boost/optional.hpp>
#include <variant>
#include <boost/utility/string_view.hpp>

namespace zyppng {



  class NetworkRequestPrivate : public BasePrivate
  {
    ZYPP_DECLARE_PUBLIC(NetworkRequest)
  public:
    enum class ProtocolMode{
      Default, //< use this mode if no special checks are required in header or write callbacks
      HTTP    //< this mode is used for HTTP and HTTPS downloads
    } _protocolMode = ProtocolMode::Default;

    NetworkRequestPrivate(Url &&url, zypp::Pathname &&targetFile, NetworkRequest::FileMode fMode, NetworkRequest &p );
    virtual ~NetworkRequestPrivate();

    bool initialize(std::string &errBuf );
    void aboutToStart ();
    void setResult ( NetworkRequestError &&err );
    void reset ();
    void resetActivityTimer ();
    void onActivityTimeout (Timer &);
    bool checkIfRangeChkSumIsValid(const NetworkRequest::Range &rng);
    void validateRange ( NetworkRequest::Range &rng );
    bool parseContentRangeHeader (const boost::string_view &line, size_t &start , size_t &len);
    bool parseContentTypeMultiRangeHeader ( const boost::string_view &line, std::string &boundary );

    std::string errorMessage () const;


    std::array<char, CURL_ERROR_SIZE+1> _errorBuf; //provide a buffer for a nicely formatted error for CURL
    std::optional<std::string> _originalError; // if this is initialized we manually set a error that caused the error stored in _errorBuf

    template<typename T>
    void setCurlOption ( CURLoption opt, T data )
    {
      auto ret = curl_easy_setopt( _easyHandle, opt, data );
      if ( ret != 0 ) {
        ZYPP_THROW( zypp::media::MediaCurlSetOptException( _url, _errorBuf.data() ) );
      }
    }

    Url                                 _url;        //file URL
    zypp::Pathname                      _targetFile; //target file
    TransferSettings                    _settings;
    NetworkRequest::Options             _options;
    zypp::ByteCount                     _expectedFileSize; // the file size as expected by the user code
    std::vector<NetworkRequest::Range>  _requestedRanges; ///< the requested ranges that need to be downloaded

    NetworkRequest::FileMode            _fMode = NetworkRequest::WriteExclusive;
    NetworkRequest::Priority            _priority = NetworkRequest::Normal;

    long _curlDebug = 0L;
    std::string _lastRedirect;	///< to log/report redirections
    const std::string _currentCookieFile = "/var/lib/YaST2/cookies";

    void *_easyHandle = nullptr; // the easy handle that controlling this request
    NetworkRequestDispatcher *_dispatcher = nullptr; // the parent downloader owning this request

    //signals
    Signal< void ( NetworkRequest &req )> _sigStarted;
    Signal< void ( NetworkRequest &req, zypp::ByteCount count )> _sigBytesDownloaded;
    Signal< void ( NetworkRequest &req, off_t dltotal, off_t dlnow, off_t ultotal, off_t ulnow )> _sigProgress;
    Signal< void ( NetworkRequest &req, const NetworkRequestError &err )> _sigFinished;

    static int curlProgressCallback ( void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow );
    size_t headerCallback (  char *ptr, size_t size, size_t nmemb  );
    size_t writeCallback ( char *ptr, size_t size, size_t nmemb );

    std::unique_ptr< curl_slist, decltype (&curl_slist_free_all) > _headers;

    struct pending_t {
      pending_t(){}
      bool _requireStatusPartial  = false;
    };

    struct running_t  {
      running_t( pending_t &&prevState );

      Timer::Ptr _activityTimer = Timer::create();

      zypp::AutoFILE _outFile;
      off_t  _currentRange = -1;
      std::optional<NetworkRequest::Range> _currentSrvRange;

      bool _allHeadersReceived    = false;
      bool _gotContentRangeHeader = false;
      bool _gotMultiRangeHeader   = false;
      bool _requireStatusPartial  = false;

      // handle the case when cancel() is called from a slot to the progress signal
      bool _isInCallback          = false;
      std::optional<NetworkRequestError> _cachedResult;

      off_t _lastProgressNow = -1; // last value returned from CURL, lets only send signals if we get actual updates
      off_t _downloaded = 0; //downloaded bytes
      zypp::ByteCount _contentLenght; // the content length as reported by the server

      //multirange support for HTTP requests (https://tools.ietf.org/html/rfc7233)
      std::string _seperatorString; ///< The seperator string for multipart responses as defined in RFC 7233 Section 4.1
      std::vector<char> _rangePrefaceBuffer; ///< Here we buffer
    };

    struct finished_t {
      off_t               _downloaded = 0; //downloaded bytes
      zypp::ByteCount     _contentLenght = 0; // the content length as reported by the server
      NetworkRequestError _result; // the overall result of the download
    };

    std::variant< pending_t, running_t, finished_t > _runningMode = pending_t();
  };

  std::vector<char> peek_data_fd ( FILE *fd, off_t offset, size_t count );
}

#endif
