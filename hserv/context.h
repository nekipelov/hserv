/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_CONTEXT_H
#define HSERV_CONTEXT_H

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>
#include <hserv/request.h>
#include <hserv/response.h>
#include <hserv/impl/connection.h>

namespace hserv {

class Context : private boost::noncopyable {
public:
    Context(boost::asio::io_service &ioService, Connection &connection,
            const Request &req, Response &resp)
        : ioServ(ioService), conn(connection), req(req), resp(resp)
    {
    }

    boost::asio::io_service &ioService() {
        return ioServ;
    }

    const Request &request() const {
        return req;
    }

    Response &response() {
        return resp;
    }

    const Response &response() const {
        return resp;
    }

    void asyncDone() {
        conn.writeResponse();
    }

    void asyncPartialSend(boost::function<void()> callback) {
        conn.writeResponsePartial(callback);
    }

private:
    boost::asio::io_service &ioServ;
    Connection &conn;
    const Request &req;
    Response &resp;
};

} // namespace hserv

#endif // HSERV_CONTEXT_H
