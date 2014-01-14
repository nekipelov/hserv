/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_RESPONSEIMPL_H
#define HSERV_RESPONSEIMPL_H

namespace hserv {

template<typename Tag>
struct ResponseImpl
{
    struct CaseInsensitive : public std::binary_function<std::string, std::string, bool> {
        bool operator() (const std::string &left, const std::string &right) const {
            return strcasecmp(left.c_str(), right.c_str()) < 0;
        }
    };

    std::map<std::string,std::string, CaseInsensitive> headers;
    std::vector<char> content;
    int status;

    static boost::asio::const_buffer httpVersionToBuffer(int major, int minor)
    {
        static const std::string http10 = "HTTP/1.0 ";
        static const std::string http11 = "HTTP/1.1 ";

        if( major == 1 )
        {
            switch(minor) {
            case 0:
                return boost::asio::buffer(http10);
            case 1:
                return boost::asio::buffer(http11);
            }
        }

        throw std::runtime_error( "HTTP version invalid");
    }

    static boost::asio::const_buffer httpStatusToBuffer(typename Tag::StatusType status)
    {
        static const std::string switchingProtocol =
            "101 Switching Protocols\r\n";
        static const std::string ok =
            "200 OK\r\n";
        static const std::string created =
            "201 Created\r\n";
        static const std::string accepted =
            "202 Accepted\r\n";
        static const std::string noContent =
            "204 No Content\r\n";
        static const std::string multipleChoices =
            "300 Multiple Choices\r\n";
        static const std::string movedPermanently =
            "301 Moved Permanently\r\n";
        static const std::string found =
            "302 Found\r\n";
        static const std::string seeOther =
            "303 Found\r\n";
        static const std::string notModified =
            "304 Not Modified\r\n";
        static const std::string temporaryRedirect =
            "307 Temporary Redirect\r\n";
        static const std::string badRequest =
            "400 Bad Request\r\n";
        static const std::string unauthorized =
            "401 Unauthorized\r\n";
        static const std::string forbidden =
            "403 Forbidden\r\n";
        static const std::string notFound =
            "404 Not Found\r\n";
        static const std::string methodNotAllowed =
            "405 Method Not Allowed\r\n";
        static const std::string internalServerError =
            "500 Internal Server Error\r\n";
        static const std::string notImplemented =
            "501 Not Implemented\r\n";
        static const std::string badGateway =
            "502 Bad Gateway\r\n";
        static const std::string serviceUnavailable =
            "503 Service Unavailable\r\n";
        static const std::string httpVersionNotSupported =
            "505 HTTP Version Not Supported\r\n";

        switch (status)
        {
        case Tag::SwitchingProtocol:
            return boost::asio::buffer(switchingProtocol);
        case Tag::Ok:
            return boost::asio::buffer(ok);
        case Tag::Created:
            return boost::asio::buffer(created);
        case Tag::Accepted:
            return boost::asio::buffer(accepted);
        case Tag::NoContent:
            return boost::asio::buffer(noContent);
        case Tag::MultipleChoices:
            return boost::asio::buffer(multipleChoices);
        case Tag::MovedPermanently:
            return boost::asio::buffer(movedPermanently);
        case Tag::Found:
            return boost::asio::buffer(found);
        case Tag::SeeOther:
            return boost::asio::buffer(seeOther);
        case Tag::NotModified:
            return boost::asio::buffer(notModified);
        case Tag::TemporaryRedirect:
            return boost::asio::buffer(temporaryRedirect);
        case Tag::BadRequest:
            return boost::asio::buffer(badRequest);
        case Tag::Unauthorized:
            return boost::asio::buffer(unauthorized);
        case Tag::Forbidden:
            return boost::asio::buffer(forbidden);
        case Tag::NotFound:
            return boost::asio::buffer(notFound);
        case Tag::MethodNotAllowed:
            return boost::asio::buffer(methodNotAllowed);
        case Tag::InternalServerError:
            return boost::asio::buffer(internalServerError);
        case Tag::NotImplemented:
            return boost::asio::buffer(notImplemented);
        case Tag::BadGateway:
            return boost::asio::buffer(badGateway);
        case Tag::ServiceUnavailable:
            return boost::asio::buffer(serviceUnavailable);
        case Tag::HttpVersionNotSupported:
            return boost::asio::buffer(httpVersionNotSupported);
        default:
            return boost::asio::buffer(internalServerError);
        }
    }

    static std::string stockReply(typename Tag::StatusType status)
    {
        static const char ok[] = "";
        const char created[] =
            "<html>"
            "<head><title>Created</title></head>"
            "<body><h1>201 Created</h1></body>"
            "</html>";
        const char accepted[] =
            "<html>"
            "<head><title>Accepted</title></head>"
            "<body><h1>202 Accepted</h1></body>"
            "</html>";
        const char noContent[] =
            "<html>"
            "<head><title>No Content</title></head>"
            "<body><h1>204 Content</h1></body>"
            "</html>";
        const char multipleChoices[] =
            "<html>"
            "<head><title>Multiple Choices</title></head>"
            "<body><h1>300 Multiple Choices</h1></body>"
            "</html>";
        const char movedPermanently[] =
            "<html>"
            "<head><title>Moved Permanently</title></head>"
            "<body><h1>301 Moved Permanently</h1></body>"
            "</html>";
        const char found[] =
            "<html>"
            "<head><title>Found</title></head>"
            "<body><h1>302 Found</h1></body>"
            "</html>";
        const char seeOther[] =
            "<html>"
            "<head><title>See Other</title></head>"
            "<body><h1>303 See Other</h1></body>"
            "</html>";
        const char notModified[] =
            "<html>"
            "<head><title>Not Modified</title></head>"
            "<body><h1>304 Not Modified</h1></body>"
            "</html>";
        const char temporaryRedirect[] =
            "<html>"
            "<head><title>Temporary Redirect</title></head>"
            "<body><h1>307 Temporary Redirect</h1></body>"
            "</html>";
        const char badRequest[] =
            "<html>"
            "<head><title>Bad Request</title></head>"
            "<body><h1>400 Bad Request</h1></body>"
            "</html>";
        const char unauthorized[] =
            "<html>"
            "<head><title>Unauthorized</title></head>"
            "<body><h1>401 Unauthorized</h1></body>"
            "</html>";
        const char forbidden[] =
            "<html>"
            "<head><title>Forbidden</title></head>"
            "<body><h1>403 Forbidden</h1></body>"
            "</html>";
        const char notFound[] =
            "<html>"
            "<head><title>Not Found</title></head>"
            "<body><h1>404 Not Found</h1></body>"
            "</html>";
        const char methodNotAllowed[] =
            "<html>"
            "<head><title>Method Not Allowed</title></head>"
            "<body><h1>405 Method Not Allowed</h1></body>"
            "</html>";
        const char internalServerError[] =
            "<html>"
            "<head><title>Internal Server Error</title></head>"
            "<body><h1>500 Internal Server Error</h1></body>"
            "</html>";
        const char notImplemented[] =
            "<html>"
            "<head><title>Not Implemented</title></head>"
            "<body><h1>501 Not Implemented</h1></body>"
            "</html>";
        const char badGateway[] =
            "<html>"
            "<head><title>Bad Gateway</title></head>"
            "<body><h1>502 Bad Gateway</h1></body>"
            "</html>";
        const char serviceUnavailable[] =
            "<html>"
            "<head><title>Service Unavailable</title></head>"
            "<body><h1>503 Service Unavailable</h1></body>"
            "</html>";
        const char httpVersionNotSupported[] =
            "<html>"
            "<head><title>HTTP Version Not Supported</title></head>"
            "<body><h1>505 HTTP Version Not Supported</h1></body>"
            "</html>";

        switch (status)
        {
        case Tag::Ok:
            return ok;
        case Tag::Created:
            return created;
        case Tag::Accepted:
            return accepted;
        case Tag::NoContent:
            return noContent;
        case Tag::MultipleChoices:
            return multipleChoices;
        case Tag::MovedPermanently:
            return movedPermanently;
        case Tag::Found:
            return found;
        case Tag::SeeOther:
            return seeOther;
        case Tag::NotModified:
            return notModified;
        case Tag::TemporaryRedirect:
            return temporaryRedirect;
        case Tag::BadRequest:
            return badRequest;
        case Tag::Unauthorized:
            return unauthorized;
        case Tag::Forbidden:
            return forbidden;
        case Tag::NotFound:
            return notFound;
        case Tag::MethodNotAllowed:
            return methodNotAllowed;
        case Tag::InternalServerError:
            return internalServerError;
        case Tag::NotImplemented:
            return notImplemented;
        case Tag::BadGateway:
            return badGateway;
        case Tag::ServiceUnavailable:
            return serviceUnavailable;
        case Tag::HttpVersionNotSupported:
            return httpVersionNotSupported;
        default:
            return internalServerError;
        }
    }
};

} // namespace hserv


#endif // HSERV_RESPONSEIMPL_H
