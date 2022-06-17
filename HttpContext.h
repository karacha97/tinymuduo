#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#include "copyable.h"

#include "HttpRequest.h"

namespace tinymuduo
{
    namespace net
    {

        class Buffer;

        class HttpContext : public tinymuduo::copyable
        {
        public:
            enum HttpRequestParseState
            {
                kExpectRequestLine,
                kExpectHeaders,
                kExpectBody,
                kGotAll,
            };

            HttpContext()
                : state_(kExpectRequestLine)
            {
            }

            // default copy-ctor, dtor and assignment are fine

            // return false if any error
            bool parseRequest(Buffer *buf, Timestamp receiveTime);

            bool gotAll() const
            {
                return state_ == kGotAll;
            }

            void reset()
            {
                state_ = kExpectRequestLine;
                HttpRequest dummy;
                request_.swap(dummy);
            }

            const HttpRequest &request() const
            {
                return request_;
            }

            HttpRequest &request()
            {
                return request_;
            }

        private:
            bool processRequestLine(const char *begin, const char *end);

            HttpRequestParseState state_;
            HttpRequest request_;
        };

    } // namespace net
} // namespace tinymuduo

#endif // HTTPCONTEXT_H
