/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_REQUEST_H
#define HSERV_REQUEST_H

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/noncopyable.hpp>

#include <hserv/header.h>
#include <hserv/impl/requestimpl.h>

namespace hserv {

class Request : private boost::noncopyable
{
public:
    Request(const RequestImpl &req) : impl(req) {}

    std::string method() const {
        return impl.method;
    }

    std::string uri() const {
        return impl.uri;
    }

    int versionMajor() const {
        return impl.versionMajor;
    }

    int versionMinor() const {
        return impl.versionMinor;
    }

    bool keepAlive() const {
        return impl.keepAlive;
    }

    std::vector<char> postData() const {
        return impl.postData;
    }

    std::vector<HeaderItem> headers() const {
        return impl.headers;
    }

    boost::asio::ip::tcp::endpoint endpoint() const {
        return impl.endpoint;
    }

    bool hasHeader(const char *s) {
        std::vector<HeaderItem>::const_iterator it = impl.headers.begin(), end = impl.headers.end();

        for(; it != end; ++it)
        {
            if( strcasecmp(it->name.c_str(), s) == 0 )
                return true;
        }

        return false;
    }

    std::string headerValue(const char *s) const {
        std::vector<HeaderItem>::const_iterator it = impl.headers.begin(), end = impl.headers.end();

        for(; it != end; ++it)
        {
            if( strcasecmp(it->name.c_str(), s) == 0 )
                return it->value;
        }

        return std::string();
    }

    bool hasHeader(const std::string &s) {
        return hasHeader(s.c_str());
    }

    std::string headerValue(const std::string &s) const {
        return headerValue(s.c_str());
    }

private:
    const RequestImpl &impl;
};

} // namespace hserv

#endif // HSERV_REQUEST_H
