/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef LIBAHTTP_REQUESTPARSER_H
#define LIBAHTTP_REQUESTPARSER_H

#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>

#include <hserv/request.h>

namespace hserv {

// Parser for incoming requests.
class RequestParser
{
public:
    RequestParser()
        : state(MethodStart), postSize(0)
    {
    }

    enum ParseState {
        CompletedState,
        IncompletedState,
        ErrorState
    };

    ParseState parse(RequestImpl &req, const char *begin, const char *end)
    {
        return consume(req, begin, end);
    }

private:
    static bool checkIfConnection(const HeaderItem &item)
    {
        return strcasecmp(item.name.c_str(), "Connection") == 0;
    }

    ParseState consume(RequestImpl &req, const char *begin, const char *end)
    {
        while( begin != end )
        {
            char input = *begin++;

            switch (state)
            {
            case MethodStart:
                if (!isChar(input) || isControl(input) || isSpecial(input))
                {
                    return ErrorState;
                }
                else
                {
                    state = Method;
                    req.method.push_back(input);
                    continue;
                }
            case Method:
                if (input == ' ')
                {
                    state = UriStart;
                    continue;
                }
                else if (!isChar(input) || isControl(input) || isSpecial(input))
                {
                    return ErrorState;
                }
                else
                {
                    req.method.push_back(input);
                    continue;
                }
            case UriStart:
                if (isControl(input))
                {
                    return ErrorState;
                }
                else
                {
                    state = Uri;
                    req.uri.push_back(input);
                    continue;
                }
            case Uri:
                if (input == ' ')
                {
                    state = HttpVersion_h;
                    continue;
                }
                else if (input == '\r')
                {
                    req.versionMajor = 0;
                    req.versionMinor = 9;

                    return CompletedState;
                }
                else if (isControl(input))
                {
                    return ErrorState;
                }
                else
                {
                    req.uri.push_back(input);
                    continue;
                }
            case HttpVersion_h:
                if (input == 'H')
                {
                    state = HttpVersion_t1;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HttpVersion_t1:
                if (input == 'T')
                {
                    state = HttpVersion_t2;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HttpVersion_t2:
                if (input == 'T')
                {
                    state = HttpVersion_p;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HttpVersion_p:
                if (input == 'P')
                {
                    state = HttpVersion_slash;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HttpVersion_slash:
                if (input == '/')
                {
                    req.versionMajor = 0;
                    req.versionMinor = 0;
                    state = HttpVersion_majorStart;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HttpVersion_majorStart:
                if (isDigit(input))
                {
                    req.versionMajor = input - '0';
                    state = HttpVersion_major;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HttpVersion_major:
                if (input == '.')
                {
                    state = HttpVersion_minorStart;
                    continue;
                }
                else if (isDigit(input))
                {
                    req.versionMajor = req.versionMajor * 10 + input - '0';
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HttpVersion_minorStart:
                if (isDigit(input))
                {
                    req.versionMinor = input - '0';
                    state = HttpVersion_minor;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HttpVersion_minor:
                if (input == '\r')
                {
                    state = ExpectingNewline_1;
                    continue;
                }
                else if (isDigit(input))
                {
                    req.versionMinor = req.versionMinor * 10 + input - '0';
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case ExpectingNewline_1:
                if (input == '\n')
                {
                    state = HeaderLineStart;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HeaderLineStart:
                if (input == '\r')
                {
                    state = ExpectingNewline_3;
                    continue;
                }
                else if (!req.headers.empty() && (input == ' ' || input == '\t'))
                {
                    state = HeaderLws;
                    continue;
                }
                else if (!isChar(input) || isControl(input) || isSpecial(input))
                {
                    return ErrorState;
                }
                else
                {
                    req.headers.push_back(HeaderItem());
                    req.headers.back().name.reserve(16);
                    req.headers.back().value.reserve(16);
                    req.headers.back().name.push_back(input);
                    state = HeaderName;
                    continue;
                }
            case HeaderLws:
                if (input == '\r')
                {
                    state = ExpectingNewline_2;
                    continue;
                }
                else if (input == ' ' || input == '\t')
                {
                    continue;
                }
                else if (isControl(input))
                {
                    return ErrorState;
                }
                else
                {
                    state = HeaderValue;
                    req.headers.back().value.push_back(input);
                    continue;
                }
            case HeaderName:
                if (input == ':')
                {
                    state = SpaceBeforeHeaderValue;
                    continue;
                }
                else if (!isChar(input) || isControl(input) || isSpecial(input))
                {
                    return ErrorState;
                }
                else
                {
                    req.headers.back().name.push_back(input);
                    continue;
                }
            case SpaceBeforeHeaderValue:
                if (input == ' ')
                {
                    state = HeaderValue;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case HeaderValue:
                if (input == '\r')
                {
                    if( req.method == "POST" )
                    {
                        HeaderItem &h = req.headers.back();

                        if( strcasecmp(h.name.c_str(), "Content-Length") == 0 )
                        {
                            postSize = atoi(h.value.c_str());
                            req.postData.reserve( postSize );
                        }
                    }
                    state = ExpectingNewline_2;
                    continue;
                }
                else if (isControl(input))
                {
                    return ErrorState;
                }
                else
                {
                    req.headers.back().value.push_back(input);
                    continue;
                }
            case ExpectingNewline_2:
                if (input == '\n')
                {
                    state = HeaderLineStart;
                    continue;
                }
                else
                {
                    return ErrorState;
                }
            case ExpectingNewline_3: {
                std::vector<HeaderItem>::iterator it = std::find_if(req.headers.begin(),
                                                                    req.headers.end(),
                                                                    checkIfConnection);

                if( it != req.headers.end() )
                {
                    if( strcasecmp(it->value.c_str(), "Keep-Alive") == 0 )
                        req.keepAlive = true;
                }
                else
                {
                    if( req.versionMajor > 1 || (req.versionMajor == 1 && req.versionMinor == 1) )
                        req.keepAlive = true;
                }

                if( postSize == 0 )
                {
                    if( input == '\n')
                        return CompletedState;
                    else
                        return ErrorState;
                }
                else
                {
                    state = Post;
                    continue;
                }
            }
            case Post:
                --postSize;
                req.postData.push_back( input );
                if( postSize == 0 )
                    return CompletedState;
                else
                    continue;
            default:
                return ErrorState;
            }
        }

        return IncompletedState;
    }

    // Check if a byte is an HTTP character.
    static inline bool isChar(int c)
    {
        return c >= 0 && c <= 127;
    }

    // Check if a byte is an HTTP control character.
    static inline bool isControl(int c)
    {
        return (c >= 0 && c <= 31) || (c == 127);
    }

    // Check if a byte is defined as an HTTP special character.
    static inline bool isSpecial(int c)
    {
        switch (c)
        {
        case '(': case ')': case '<': case '>': case '@':
        case ',': case ';': case ':': case '\\': case '"':
        case '/': case '[': case ']': case '?': case '=':
        case '{': case '}': case ' ': case '\t':
            return true;
        default:
            return false;
        }
    }

    // Check if a byte is a digit.
    static inline bool isDigit(int c)
    {
        return c >= '0' && c <= '9';
    }

    // The current state of the parser.
    enum State
    {
        MethodStart,
        Method,
        UriStart,
        Uri,
        HttpVersion_h,
        HttpVersion_t1,
        HttpVersion_t2,
        HttpVersion_p,
        HttpVersion_slash,
        HttpVersion_majorStart,
        HttpVersion_major,
        HttpVersion_minorStart,
        HttpVersion_minor,
        ExpectingNewline_1,
        HeaderLineStart,
        HeaderLws,
        HeaderName,
        SpaceBeforeHeaderValue,
        HeaderValue,
        ExpectingNewline_2,
        ExpectingNewline_3,
        Post
    } state;

    size_t postSize;
};

} // namespace hserv

#endif // LIBAHTTP_REQUESTPARSER_H
