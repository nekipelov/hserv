/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_RESPONSE_H
#define HSERV_RESPONSE_H

#include <string>
#include <vector>
#include <functional>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <hserv/header.h>
#include <hserv/impl/responseimpl.h>

namespace hserv {

class Response
{
public:
    Response()
    {
        // Date header value

        char buff[64] = {0};
        static const char format[] = "%a, %d %b %Y %H:%M:%S GMT"; // rfc 1123
        struct tm tm;

        time_t now = time(0);
#ifdef _MSC_VER
        tm = *gmtime( &now );
#else
        gmtime_r( &now, &tm );
#endif
        strftime(buff, sizeof(buff) - 1, format, &tm);

        static const std::string date = "Date";
        static const std::string server = "Server";
        static const std::string hserv = "hserv";

        addHeader(date, buff);
        addHeader(server, hserv);

        setStatus( Ok );
    }

    // The status of the response.
    enum StatusType
    {
        SwitchingProtocol = 101,
        Ok = 200,
        Created = 201,
        Accepted = 202,
        NoContent = 204,
        MultipleChoices = 300,
        MovedPermanently = 301,
        Found = 302,
        SeeOther = 303,
        NotModified = 304,
        TemporaryRedirect = 307,
        BadRequest = 400,
        Unauthorized = 401,
        Forbidden = 403,
        NotFound = 404,
        MethodNotAllowed = 405,
        InternalServerError = 500,
        NotImplemented = 501,
        BadGateway = 502,
        ServiceUnavailable = 503,
        HttpVersionNotSupported = 505
    };

    void setStatus(StatusType status)
    {
        impl.status = status;
    }

    StatusType status() const
    {
        return static_cast<StatusType>(impl.status);
    }

    std::map<std::string, std::string> headers() const
    {
        return std::map<std::string, std::string>(
                    impl.headers.begin(),
                    impl.headers.end());
    }

    std::string header(const std::string &name) const
    {
        std::map<std::string,std::string, ResponseImpl<Response>::CaseInsensitive>::const_iterator
                it = impl.headers.find(name);

        if( it == impl.headers.end() )
            return std::string();
        else
            return it->second;
    }

    void addHeader(const std::string &name, const std::string &value)
    {
        impl.headers[name] = value;
    }

    const std::vector<char> &content() const
    {
        return impl.content;
    }

    void setContent(const std::vector<char> &content)
    {
        impl.content = content;
    }

    void setContent(const std::string &s)
    {
        impl.content.assign( s.begin(), s.end() );
    }

    void setContent(const char *s, size_t size = static_cast<size_t>(-1))
    {
        if( size == static_cast<size_t>(-1) )
            impl.content.assign( s, s + strlen(s) );
        else
            impl.content.assign( s, s + size );
    }

    void addContent(const std::vector<char> &content)
    {
        impl.content.insert( impl.content.end(), content.begin(), content.end() );
    }

    void addContent(const std::string &s)
    {
        impl.content.insert( impl.content.end(), s.begin(), s.end() );
    }

    void addContent(const char *s, size_t size = static_cast<size_t>(-1))
    {
        if( size == static_cast<size_t>(-1) )
            impl.content.insert( impl.content.end(), s, s + strlen(s) );
        else
            impl.content.insert( impl.content.end(), s, s + size );
    }

    std::vector<boost::asio::const_buffer> statusBuffers(int versionMajor,
                                                         int versionMinor) const
    {
        std::vector<boost::asio::const_buffer> buffers;

        buffers.reserve( 2 );
        buffers.push_back(ResponseImpl<Response>::httpVersionToBuffer(versionMajor, versionMinor));
        buffers.push_back(ResponseImpl<Response>::httpStatusToBuffer(status()));

        return buffers;
    }

    static boost::asio::const_buffer statusBuffer(StatusType status)
    {
        return ResponseImpl<Response>::httpStatusToBuffer(status);
    }

    std::vector<boost::asio::const_buffer> headerBuffers() const
    {
        static const char name_value_separator[] = { ':', ' ' };
        static const char crlf[] = { '\r', '\n' };

        std::vector<boost::asio::const_buffer> buffers;

        buffers.reserve( 4 * impl.headers.size() + 1);

        std::map<std::string, std::string, ResponseImpl<Response>::CaseInsensitive>::const_iterator
                it = impl.headers.begin(), end = impl.headers.end();

        for(; it != end; ++it)
        {
            buffers.push_back(boost::asio::buffer(it->first));
            buffers.push_back(boost::asio::buffer(name_value_separator));
            buffers.push_back(boost::asio::buffer(it->second));
            buffers.push_back(boost::asio::buffer(crlf));
        }

        return buffers;
    }

    static Response makeResponse(StatusType status)
    {
        Response resp;

        const std::string &s = ResponseImpl<Response>::stockReply(status);
        resp.impl.status = status;
        resp.impl.content.assign( s.begin(), s.end() );

        resp.impl.headers["Content-Type"] = "text/html; charset=utf-8";
        resp.impl.headers["Content-Length"] =
                boost::lexical_cast<std::string>(resp.impl.content.size());

        return resp;
    }

private:
    ResponseImpl<Response> impl;
};

} // namespace hserv

#endif // HSERV_RESPONSE_H
