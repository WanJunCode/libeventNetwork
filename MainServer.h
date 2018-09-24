#ifndef __MAIN_SERVER_H__
#define __MAIN_SERVER_H__

#include "MyTransport.h"
#include "TSocket.h"
#include "TConnection.h"

#include <event.h>
#include <vector>
#include <queue>
#include <mutex>

class TSocket;
class MyTransport;
class TConnection;
class MainServer
{
public:
  explicit MainServer(int port);
  ~MainServer();

  struct event_base *getBase();
  void serve();
  void handlerConn(void *args);
  void returnTConnection(TConnection *conn);

private:
  struct event_base *main_base;
  struct event *ev_stdin;
  int port_;
  MyTransport *transport_;
  std::mutex connMutex;

  std::vector<TConnection *> activeTConnection;
  std::queue<TConnection *> connectionQueue;

private:
  static void stdin_cb(evutil_socket_t stdin_fd, short what, void *args);
};

#endif