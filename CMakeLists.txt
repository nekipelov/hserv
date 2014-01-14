PROJECT(hserv)
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.3)

FIND_PACKAGE(Threads)
FIND_PACKAGE(Boost COMPONENTS thread REQUIRED)
FIND_PACKAGE(Boost COMPONENTS system REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)
FIND_PACKAGE(OpenSSL)

INCLUDE(cmake/CheckCXX11Features.cmake)

IF(MSVC)
    SET( CMAKE_CXX_FLAGS "-W3 -DWIN32 -EHsc -DBOOST_ALL_NO_LIB")
ELSEIF(CMAKE_COMPILER_IS_GNUCXX)
    IF(HAS_CXX11_LAMBDA)
        SET( CMAKE_CXX_FLAGS "-Wextra -Wall -std=c++0x")
    ELSE(HAS_CXX11_LAMBDA)
        SET( CMAKE_CXX_FLAGS "-Wextra -Wall")
    ENDIF(HAS_CXX11_LAMBDA)
ELSE(MSVC)
    SET( CMAKE_CXX_FLAGS "-Wextra -Wall")
ENDIF(MSVC)

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} )

SET (HEADERS
    hserv/context.h
    hserv/fastcgiserver.h
    hserv/header.h
    hserv/httpserver.h
    hserv/httpsserver.h
    hserv/request.h
    hserv/response.h
    hserv/serverinterface.h
    hserv/impl/connection.h
    hserv/impl/fastcgiconnection.h
    hserv/impl/fastcgiserverimpl.h
    hserv/impl/httpconnection.h
    hserv/impl/httpserverimpl.h
    hserv/impl/httpsserverimpl.h
    hserv/impl/requestimpl.h
    hserv/impl/requestparser.h
    hserv/impl/responseimpl.h
)

IF(HAS_CXX11_LAMBDA)
    ADD_EXECUTABLE(helloworld_cxx11  examples/helloworld_cxx11.cpp ${HEADERS})
ENDIF(HAS_CXX11_LAMBDA)

IF(OPENSSL_FOUND)
    ADD_EXECUTABLE(helloworld_ssl  examples/helloworld_ssl.cpp ${HEADERS})
ENDIF(OPENSSL_FOUND)

ADD_EXECUTABLE(helloworld  examples/helloworld.cpp ${HEADERS})
ADD_EXECUTABLE(fileserver  examples/fileserver.cpp ${HEADERS})
ADD_EXECUTABLE(proxyserver examples/proxyserver.cpp ${HEADERS})

TARGET_LINK_LIBRARIES(fileserver
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_THREAD_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)

TARGET_LINK_LIBRARIES(proxyserver
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_THREAD_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)

TARGET_LINK_LIBRARIES(helloworld
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_THREAD_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)

IF(HAS_CXX11_LAMBDA)
    TARGET_LINK_LIBRARIES(helloworld_cxx11
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_THREAD_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
    )
ENDIF(HAS_CXX11_LAMBDA)

IF(OPENSSL_FOUND)
    TARGET_LINK_LIBRARIES(helloworld_ssl
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_THREAD_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${OPENSSL_LIBRARIES}
    )
ENDIF(OPENSSL_FOUND)

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/hserv DESTINATION include)
