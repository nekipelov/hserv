/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_FASTCGICONNECTION_H
#define HSERV_FASTCGICONNECTION_H

#include <vector>
#include <iostream>

#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>

#include <hserv/response.h>
#include <hserv/request.h>
#include <hserv/impl/connection.h>

namespace hserv {

template<typename T>
class FastCGIConnection : public Connection,
        public boost::enable_shared_from_this< FastCGIConnection<T> >
{
public:
    FastCGIConnection(boost::asio::io_service &io_service,
                      const boost::function<void(const boost::shared_ptr<Context> &)> &callback);

    void start();

    void stop();

    T& getSocket();

    virtual void writeResponse();
    virtual void writeResponsePartial(const boost::function<void()> &callback);

private:
    struct FcgiHeader {
        unsigned char version;
        unsigned char type;
        unsigned char requestIdB1;
        unsigned char requestIdB0;
        unsigned char contentLengthB1;
        unsigned char contentLengthB0;
        unsigned char paddingLength;
        unsigned char reserved;
    };

    struct FcgiBeginRequestBody {
        unsigned char roleB1;
        unsigned char roleB0;
        unsigned char flags;
        unsigned char reserved[5];
    };

    struct FcgiBeginRequestRecord {
        FcgiHeader header;
        FcgiBeginRequestBody body;
    };

    enum {
        FcgiVersion = 1,
        FcgiKeepConnection = 1,
        MaxFcgiMessageLength = 0xffff
    };

    enum FcgiMessageType {
        MessageBeginRequest = 1,
        MessageAbortRequest = 2,
        MessageEndRequest = 3,
        MessageParams = 4,
        MessageStdin = 5,
        MessageStdout = 6,
        MessageStderr = 7,
        MessageData = 8,
        MessageGetValues = 9,
        MessageGetValuesResult = 10,
        MessageMax = 11
    };

    enum FcgiRoleType {
        RoleResponder = 1,
        RoleAuthorizer = 2,
        RoleFilter = 3
    };

    void handleBeginRequest(const boost::system::error_code &ec);
    void handleNextHeader(const boost::system::error_code &ec);
    void handleParams(uint16_t len, const boost::system::error_code &ec);
    void handleMessageStdin(uint16_t len, const boost::system::error_code &ec);
    void handleMessagesComplete(const boost::system::error_code &ec);
    virtual void handleComplete(const boost::system::error_code &ec);
    void handlePartialComplete(const boost::function<void()> &callback,
                               const boost::system::error_code &ec);
    void writeResponseInternal(const boost::function<void()> &callback);


    void processCgiHeader(const std::string &name, const std::string &value);
    FcgiHeader makeHeader(FcgiMessageType type, uint16_t length) const;

    std::vector<char> msgBuf;
    boost::asio::streambuf streamBuf;
    uint16_t requestId;
    bool keepConnection;
    bool firstChunk;

    T socket;

    boost::function<void(boost::shared_ptr<Context> &)> callback;

    boost::array<char, 8192> buffer;

    RequestImpl requestImpl;
    Request request;

    Response response;

    boost::shared_ptr<Context> context;
};

template<typename T>
FastCGIConnection<T>::FastCGIConnection(
        boost::asio::io_service &io_service,
        const boost::function<void(const boost::shared_ptr<Context> &)> &callback)
    : Connection(), requestId(0),
      keepConnection(false), firstChunk(true),
      socket(io_service), callback(callback), request(requestImpl)
{
    context.reset(new Context(io_service, *this, request, response));
}

template<typename T>
void FastCGIConnection<T>::start()
{
    try
    {
        requestImpl = RequestImpl();
        // FIXME!!
        // requestImpl.endpoint = socket.remote_endpoint();
        response = Response();
        firstChunk = true;

        boost::asio::async_read(socket,
                                boost::asio::buffer(&buffer[0],
                                                    sizeof(FcgiBeginRequestRecord)),
                                boost::bind(&FastCGIConnection::handleBeginRequest,
                                            this->shared_from_this(),
                                            boost::asio::placeholders::error));
    }
    catch(std::exception &e)
    {
        std::cerr << "exception: " << e.what() << std::endl;
        stop();
    }
}

template<typename T>
void FastCGIConnection<T>::stop()
{
    socket.close();
}

template<typename T>
T& FastCGIConnection<T>::getSocket()
{
    return socket;
}

template<typename T>
void FastCGIConnection<T>::handleBeginRequest(const boost::system::error_code &ec)
{
    if( !ec )
    {
        FcgiBeginRequestRecord *req = reinterpret_cast<FcgiBeginRequestRecord *>(&buffer[0]);

        if( req->header.version != 1 )
        {
            std::cerr << "Invalid protocol version " << req->header.version << std::endl;
            stop();
            return;
        }

        requestId = (req->header.requestIdB1 << 8) + req->header.requestIdB0;
        uint16_t len = (req->header.contentLengthB1 << 8) + req->header.contentLengthB0;

        if( req->header.type != MessageBeginRequest
                || len != sizeof(FcgiBeginRequestBody) )
        {

            std::cerr << "Protocol error: req->header.type != MessageBeginRequest || "
                         "len != sizeof(FcgiBeginRequestBody)" << std::endl;
            stop();
            return;
        }

        this->keepConnection = req->body.flags & FcgiKeepConnection;
        int role = (req->body.roleB1 << 8) + req->body.roleB0;

        if( role != RoleResponder )
        {
            std::cerr << "Invalid role " << role << std::endl;
            stop();
            return;
        }

        boost::asio::async_read(socket,
                                boost::asio::buffer(&buffer[0],
                                                    sizeof(FcgiHeader)),
                                boost::bind(&FastCGIConnection::handleNextHeader,
                                            this->shared_from_this(),
                                            boost::asio::placeholders::error));
    }
    else
    {
        std::cerr << "FastCGIConnection<T>::handleBeginRequest: " << ec.message() << std::endl;
        stop();
    }
}

template<typename T>
void FastCGIConnection<T>::handleNextHeader(const boost::system::error_code &ec)
{
    if( !ec )
    {
        FcgiHeader *HeaderItem = reinterpret_cast<FcgiHeader *>(&buffer[0]);

        if( HeaderItem->version != FcgiVersion )
        {
            std::cerr << "Invalid protocol version " << HeaderItem->version << std::endl;
            stop();
            return;
        }

        uint16_t requestId = (HeaderItem->requestIdB1 << 8) + HeaderItem->requestIdB0;
        uint16_t len = (HeaderItem->contentLengthB1 << 8) + HeaderItem->contentLengthB0;

        if( this->requestId != requestId )
        {
            stop();
            return;
        }

        switch(HeaderItem->type)
        {
        case MessageBeginRequest:
            std::cerr << "Protocol error: MessageBeginRequest duplicate" << std::endl;
            stop();
            return;
        case MessageAbortRequest:
            stop();
            return;
        case MessageParams:
            if( len == 0 )
            {
                boost::asio::async_read(socket,
                                        boost::asio::buffer(&buffer[0],
                                                            sizeof(FcgiHeader)),
                                        boost::bind(&FastCGIConnection::handleNextHeader,
                                                    this->shared_from_this(),
                                                    boost::asio::placeholders::error));
            }
            else
            {
                msgBuf.resize(len + HeaderItem->paddingLength);
                boost::asio::async_read(socket,
                                        boost::asio::buffer(&msgBuf[0],
                                                            len + HeaderItem->paddingLength),
                                        boost::bind(&FastCGIConnection::handleParams,
                                                    this->shared_from_this(), len,
                                                    boost::asio::placeholders::error));
            }
            break;
        case MessageStdin:
            if( len == 0 )
            {
                // request done
                callback(context);
            }
            else
            {
                msgBuf.resize(len + HeaderItem->paddingLength);
                boost::asio::async_read(socket,
                                        boost::asio::buffer(&msgBuf[0],
                                                            len + HeaderItem->paddingLength),
                                        boost::bind(&FastCGIConnection::handleMessageStdin,
                                                    this->shared_from_this(), len,
                                                    boost::asio::placeholders::error));
            }
            break;
        case MessageEndRequest:
        case MessageStdout:
        case MessageStderr:
        case MessageData:
        case MessageGetValues:
        case MessageGetValuesResult:
        default:
            std::cerr << "Invalid message " << HeaderItem->type << std::endl;
            stop();
            assert(false);
            return;
        }
    }
    else
    {
        std::cerr << "exception: " << ec.message() << std::endl;
        stop();
    }
}

template<typename T>
void FastCGIConnection<T>::handleParams(uint16_t len, const boost::system::error_code &ec)
{
    if( !ec )
    {
        const unsigned char *ptr = NULL;

        if( msgBuf.empty() == false )
        {
            ptr = reinterpret_cast<const unsigned char *>(&msgBuf[0]);
        }
        else
        {
            stop();
            return;
        }

        const unsigned char *end = ptr + len;
        int nameLen = 0;

        while( ptr != end )
        {
            nameLen = *ptr;

            if( nameLen & 0x80 )
            {
                if( end - ptr < 3 ) {
                    stop();
                    return;
                }
                nameLen = ((nameLen & 0x7f) << 24) + (ptr[1] << 16)
                        + (ptr[2] << 8) + ptr[3];
                ptr += 3;
            }
            ++ptr;

            int valueLen = *ptr;

            if( valueLen & 0x80 )
            {
                if( end - ptr < 3 ) {
                    stop();
                    return;
                }
                valueLen = ((valueLen & 0x7f) << 24) + (ptr[1] << 16)
                        + (ptr[2] << 8) + ptr[3];
                ptr += 3;
            }
            ++ptr;

            if( nameLen + valueLen > end - ptr ) {
                stop();
                return;
            }

            std::string name(ptr, ptr + nameLen);
            ptr += nameLen;
            std::string value(ptr, ptr + valueLen);
            ptr += valueLen;

            processCgiHeader(name, value);
        }

        boost::asio::async_read(socket,
                                boost::asio::buffer(&buffer[0],
                                                    sizeof(FcgiHeader)),
                                boost::bind(&FastCGIConnection::handleNextHeader,
                                            this->shared_from_this(),
                                            boost::asio::placeholders::error));
    }
    else
    {
        std::cerr << "exception: " << ec.message() << std::endl;
        stop();
    }
}

template<typename T>
void FastCGIConnection<T>::handleMessageStdin(uint16_t len, const boost::system::error_code &ec)
{
    if( !ec )
    {
        requestImpl.postData.reserve( requestImpl.postData.size() + len );
        requestImpl.postData.insert( requestImpl.postData.end(),
                                     &msgBuf[0], &msgBuf[0] + len);

        boost::asio::async_read(socket,
                                boost::asio::buffer(&buffer[0],
                                                    sizeof(FcgiHeader)),
                                boost::bind(&FastCGIConnection::handleNextHeader,
                                            this->shared_from_this(),
                                            boost::asio::placeholders::error));
    }
    else
    {
        std::cerr << "exception: " << ec.message() << std::endl;
        stop();
    }
}

template<typename T>
void FastCGIConnection<T>::writeResponse()
{
    writeResponseInternal( boost::function<void()>() );
}

template<typename T>
void FastCGIConnection<T>::writeResponsePartial(const boost::function<void()> &callback)
{
    writeResponseInternal( callback );
}

template<typename T>
void FastCGIConnection<T>::writeResponseInternal(const boost::function<void()> &callback)
{
    std::ostream stream(&streamBuf);
    const std::vector<char> &content = response.content();
    std::vector<char> data;

    data.reserve( content.size() );

    if( firstChunk )
    {
        static const char status[] = {'S', 't', 'a', 't', 'u', 's', ':', ' '};
        static const char crlf[] = {'\r', '\n'};
        boost::asio::const_buffer replyStatus = Response::statusBuffer(response.status());

        {
            const char *ptr = boost::asio::buffer_cast<const char *>(replyStatus);
            data.insert(data.end(), status, status + sizeof(status));
            data.insert(data.end(), ptr, ptr + boost::asio::buffer_size(replyStatus));
        }

        const std::vector<boost::asio::const_buffer> &headers =
                response.headerBuffers();
        std::vector<boost::asio::const_buffer>::const_iterator it = headers.begin(),
                end = headers.end();
        for(; it != end; ++it) {
            const char *ptr = boost::asio::buffer_cast<const char *>(*it);
            data.insert(data.end(), ptr, ptr + boost::asio::buffer_size(*it));
        }

        data.insert(data.end(), crlf, crlf + sizeof(crlf));

        firstChunk = false;
    }

    data.insert( data.end(), content.begin(), content.end() );

    const size_t size = data.size();
    const size_t max = MaxFcgiMessageLength;

    for(size_t i = 0; i < size;)
    {
        size_t chunkSize = i + max > size ? size - i : max;

        FcgiHeader HeaderItem = makeHeader(MessageStdout, chunkSize);

        stream << HeaderItem.version << HeaderItem.type
               << HeaderItem.requestIdB1 << HeaderItem.requestIdB0
               << HeaderItem.contentLengthB1 << HeaderItem.contentLengthB0
               << HeaderItem.paddingLength << HeaderItem.reserved;

        const char *ptr = &data[i];
        std::copy(ptr, ptr + chunkSize, std::ostream_iterator<char>(stream));

        i += chunkSize;
    }

    if( callback )
    {
        boost::asio::async_write(socket, streamBuf,
                                 boost::bind(
                                     &FastCGIConnection::handlePartialComplete,
                                     this->shared_from_this(),
                                     callback,
                                     boost::asio::placeholders::error) );
    }
    else
    {
        boost::asio::async_write(socket, streamBuf,
                                 boost::bind(
                                     &FastCGIConnection::handleMessagesComplete,
                                     this->shared_from_this(),
                                     boost::asio::placeholders::error) );
    }
}

template<typename T>
void FastCGIConnection<T>::handleMessagesComplete(const boost::system::error_code &ec)
{
    if( !ec )
    {
        std::ostream stream(&streamBuf);
        {
            FcgiHeader HeaderItem = makeHeader(MessageStdout, 0);

            stream << HeaderItem.version << HeaderItem.type
                   << HeaderItem.requestIdB1 << HeaderItem.requestIdB0
                   << HeaderItem.contentLengthB1 << HeaderItem.contentLengthB0
                   << HeaderItem.paddingLength << HeaderItem.reserved;
        }

        {
            FcgiHeader HeaderItem = makeHeader(MessageStdout, 0);

            stream << HeaderItem.version << HeaderItem.type
                   << HeaderItem.requestIdB1 << HeaderItem.requestIdB0
                   << HeaderItem.contentLengthB1 << HeaderItem.contentLengthB0
                   << HeaderItem.paddingLength << HeaderItem.reserved;
        }

        boost::asio::async_write(socket, streamBuf,
                                 boost::bind(
                                     &FastCGIConnection::handleComplete,
                                     this->shared_from_this(),
                                     boost::asio::placeholders::error) );
    }
    else if(ec != boost::asio::error::operation_aborted)
    {
        std::cerr << "FastCGIConnection<T>::handleMessagesComplete error: " << ec.message() << std::endl;
        stop();
    }
}

template<typename T>
void FastCGIConnection<T>::handlePartialComplete(const boost::function<void()> &callback,
                                                 const boost::system::error_code &ec)
{
    if (!ec)
    {
        if( callback )
            callback();
    }
    else if (ec != boost::asio::error::operation_aborted)
    {
        stop();
    }
}

template<typename T>
void FastCGIConnection<T>::handleComplete(const boost::system::error_code &ec)
{
    if (!ec)
    {
        if( keepConnection == true )
        {
            start();
            return;
        }
        else
        {
            // FIXME
            stop();
            // Initiate graceful connection closure.
//            boost::system::error_code ignored_ec;
//            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        }
    }
    else if (ec != boost::asio::error::operation_aborted)
    {
        stop();
    }
}

template<typename T>
void FastCGIConnection<T>::processCgiHeader(const std::string &name, const std::string &value)
{
    if( name.empty() )
        return;

    switch(name[0])
    {
    case 'C':
        if( strcmp(name.c_str(), "CONTENT_LENGTH") == 0 )
        {
            HeaderItem h = {"Content-length", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "CONTENT_TYPE") == 0 )
        {
            HeaderItem h = {"Content-type", value};
            requestImpl.headers.push_back(h);
            return;
        }
        break;
    case 'H':
        if( strcmp(name.c_str(), "HTTP_HOST") == 0 )
        {
            HeaderItem h = {"Host", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_COOKIE") == 0 )
        {
            HeaderItem h = {"Cookie", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_USER_AGENT") == 0 )
        {
            HeaderItem h = {"User-Agent", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_IF_MODIFIED_SINCE") == 0 )
        {
            HeaderItem h = {"If-Modified-Since", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_IF_MATCH") == 0 )
        {
            HeaderItem h = {"If-Match", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_IF_NONE_MATCH") == 0 )
        {
            HeaderItem h = {"If-None-Match", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_ACCEPT") == 0 )
        {
            HeaderItem h = {"Accept", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_ACCEPT_ENCODING") == 0 )
        {
            HeaderItem h = {"Accept-Encoding", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_ACCEPT_LANGUAGE") == 0 )
        {
            HeaderItem h = {"Accept-Language", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_ACCEPT_CHARSET") == 0 )
        {
            HeaderItem h = {"Accept-Charset", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_X_REQUESTED_WITH") == 0 )
        {
            HeaderItem h = {"X-Requested-With", value};
            requestImpl.headers.push_back(h);
            return;
        }
        else if( strcmp(name.c_str(), "HTTP_USER_AGENT") == 0 )
        {
            HeaderItem h = {"User-Agent", value};
            requestImpl.headers.push_back(h);
            return;
        }
        break;
    case 'R':
        if( strcmp(name.c_str(), "REQUEST_METHOD") == 0 )
        {
            requestImpl.method = value;
            return;
        }
        else if( strcmp(name.c_str(), "REQUEST_URI") == 0 )
        {
            requestImpl.uri = value;
            return;
        }
        break;
    case 'Q':
        if( strcmp(name.c_str(), "QUERY_STRING") == 0 )
        {
            return;
        }
    }
}

template<typename T>
typename FastCGIConnection<T>::FcgiHeader FastCGIConnection<T>::makeHeader(FcgiMessageType type, uint16_t length) const
{
    FcgiHeader result;

    result.version = FcgiVersion;
    result.type = type;
    result.requestIdB1 = requestId >> 8;
    result.requestIdB0 = requestId & 0xff;
    result.contentLengthB1 = length >> 8;
    result.contentLengthB0 = length & 0xff;
    result.paddingLength = 0;
    result.reserved = 0;

    return result;
}

} // namespace hserv

#endif // HSERV_FASTCGICONNECTION_H
