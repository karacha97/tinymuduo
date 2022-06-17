#include <tinymuduo/TcpServer.h>


namespace tinymuduo
{
    namespace net
    {
        class HttpRequest;
        class HttpResponse;
        using HttpCallback = std::function<void(const HttpRequest &,
                                                HttpResponse *)>;
        class HttpServer
        {
        public:
            HttpServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const std::string &name,
                       TcpServer::Option option = TcpServer::kNoReusePort);
           

            EventLoop *getLoop() const { return server_.getLoop(); }

           
            void setHttpCallback(const HttpCallback &cb)
            {
                httpCallback_ = cb;
            }

            void setThreadNum(int numThreads)
            {
                server_.setThreadNum(numThreads);
            }

            void start();

        private:
            void onConnection(const TcpConnectionPtr &conn);
            void onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp receiveTime);
            void onRequest(const TcpConnectionPtr &, const HttpRequest &);
            TcpServer server_;
            HttpCallback httpCallback_;
        };
    }
} // namespace tinymuduo
