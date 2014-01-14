/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_REQUESTIMPL_H
#define HSERV_REQUESTIMPL_H

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <hserv/header.h>

namespace hserv {

struct RequestImpl {
    std::string method;
    std::string uri;
    int versionMajor;
    int versionMinor;
    std::vector<HeaderItem> headers;
    std::vector<char> postData;
    boost::asio::ip::tcp::endpoint endpoint;
    bool keepAlive;
};

} // namespace hserv

#endif // HSERV_REQUESTIMPL_H
