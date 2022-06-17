#include "HttpServer.h"

#include <tinymuduo/Logging.h>
#include <tinymuduo/HttpContext.h>
#include <tinymuduo/HttpRequest.h>
#include <tinymuduo/HttpResponse.h>
#include "Icons.h"
using namespace tinymuduo;
using namespace tinymuduo::net;
extern char favicon[555];
namespace tinymuduo
{
  namespace net
  {
    namespace detail
    {

      void defaultHttpCallback(const HttpRequest &req, HttpResponse *resp)
      {
        if (req.path() == "/good")
        {
          resp->setStatusCode(HttpResponse::k200Ok);
          resp->setStatusMessage("OK");
          resp->setContentType("text/plain");
          resp->addHeader("Server", "Webserver");
          // 增大数据量，体现ringbuffer性能
          std::string AAA(950, '!');
          AAA += "\n";
          resp->setBody(AAA);
        }
        else if (req.path() == "/hello")
        {
          resp->setStatusCode(HttpResponse::k200Ok);
          resp->setStatusMessage("OK");
          resp->setContentType("text/plain");
          resp->addHeader("Server", "Webserver");
          resp->setBody("hello, world!\n");
        }
        else if (req.path() == "/")
        {
          resp->setStatusCode(HttpResponse::k200Ok);
          resp->setStatusMessage("OK");
          resp->setContentType("text/html");
          resp->addHeader("Server", "Webserver");
          std::string now = Timestamp::now().toString();
          resp->setBody("<html><head><title>Welcome to MyWeb</title></head>"
                        "<body><h1>Hello, I am Karacha </h1>Now is " +
                        now + "</body></html>");
        }
        else if (req.path() == "/favicon.ico")
        {
          resp->setStatusCode(HttpResponse::k200Ok);
          resp->setStatusMessage("OK");
          resp->setContentType("image/png");
          resp->setBody(std::string(favicon, sizeof(favicon)));
        }
        else
        {
          resp->setStatusCode(HttpResponse::k404NotFound);
          resp->setStatusMessage("Not Found");
          resp->setCloseConnection(true);
        }
      }

    } // namespace detail
  }   // namespace net
} // namespace muduo

HttpServer::HttpServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const std::string &name,
                       TcpServer::Option option)
    : server_(loop, listenAddr, name, option),
      httpCallback_(detail::defaultHttpCallback)
{
  server_.setConnectionCallback(
      std::bind(&HttpServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

void HttpServer::start()
{
  LOG_WARN << "HttpServer[" << server_.name()
           << "] starts listening on " << server_.ipPort();
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn)
{
  if (conn->connected())
  {
    conn->setContext(HttpContext());
  }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp receiveTime)
{
  HttpContext *context = (conn->getMutableContext());

  if (!context->parseRequest(buf, receiveTime))
  {
    conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->shutdown();
  }

  if (context->gotAll())
  {
    onRequest(conn, context->request());
    context->reset();
  }
}

void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest &req)
{
  const std::string &connection = req.getHeader("Connection");
  bool close = connection == "close" ||
               (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
  HttpResponse response(close);
  httpCallback_(req, &response);
  Buffer buf;
  response.appendToBuffer(&buf);
  conn->send(&buf);
  if (response.closeConnection())
  {
    conn->shutdown();
  }
}
