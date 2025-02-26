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
#ifndef ZYPP_NG_MEDIA_CURL_PRIVATE_CURL_P_H_INCLUDED
#define ZYPP_NG_MEDIA_CURL_PRIVATE_CURL_P_H_INCLUDED

#include <zypp-curl/ng/network/networkrequestdispatcher.h>
#include <zypp-core/zyppng/base/private/base_p.h>
#include <curl/curl.h>
#include <deque>
#include <set>
#include <unordered_map>

namespace zyppng {

class Timer;
class SocketNotifier;

class NetworkRequestDispatcherPrivate : public BasePrivate
{
  ZYPP_DECLARE_PUBLIC(NetworkRequestDispatcher)
public:
  NetworkRequestDispatcherPrivate ( NetworkRequestDispatcher &p );
  virtual ~NetworkRequestDispatcherPrivate();

  int _maxConnections = 10;

  std::deque< std::shared_ptr<NetworkRequest> > _pendingDownloads;
  std::vector< std::shared_ptr<NetworkRequest> > _runningDownloads;

  std::shared_ptr<Timer> _timer;
  std::map< curl_socket_t, std::shared_ptr<SocketNotifier> > _socketHandler;

  bool  _isRunning = false;
  bool  _locked = false; //if set to true, no new requests will be dequeued
  CURLM *_multi = nullptr;

  NetworkRequestError _lastError;

  std::string _userAgent;
  std::unordered_map< std::string, std::unordered_map<std::string, std::string> > _customHeaders;

  //signals
  Signal< void ( NetworkRequestDispatcher &, NetworkRequest & )> _sigDownloadStarted;
  Signal< void ( NetworkRequestDispatcher &, NetworkRequest & )> _sigDownloadFinished;
  Signal< void ( NetworkRequestDispatcher & )> _sigQueueFinished;
  Signal< void ( NetworkRequestDispatcher & )> _sigError;

private:
  static int multi_timer_cb ( CURLM *multi, long timeout_ms, void *g );
  static int static_socket_callback(CURL *easy, curl_socket_t s, int what, void *userp, SocketNotifier *socketp );

  void multiTimerTimout ( const Timer &t );
  int  socketCallback(CURL *easy, curl_socket_t s, int what, void * );

  void cancelAll ( NetworkRequestError result );
  void setFinished( NetworkRequest &req , NetworkRequestError result );

  void onSocketActivated  ( const SocketNotifier &listener, int events );

  void handleMultiSocketAction ( curl_socket_t nativeSocket, int evBitmask );
  void dequeuePending ();
};
}

#endif
