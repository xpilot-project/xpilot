/// @file       Network.cpp
/// @brief      Low-level network communications, especially for TCP/UDP
/// @see        Some inital ideas and pieces of code take from
///             https://linux.m2osw.com/c-implementation-udp-clientserver
/// @details    XPMP2::SocketNetworking: Any network socket connection\n
///             XPMP2::UDPReceiver: listens to and receives UDP datagram\n
///             XPMP2::UDPMulticast: sends and receives multicast UDP datagrams\n
///             XPMP2::TCPConnection: receives incoming TCP connection\n
/// @author     Birger Hoppe
/// @copyright  (c) 2019-2024 Birger Hoppe
/// @copyright  Permission is hereby granted, free of charge, to any person obtaining a
///             copy of this software and associated documentation files (the "Software"),
///             to deal in the Software without restriction, including without limitation
///             the rights to use, copy, modify, merge, publish, distribute, sublicense,
///             and/or sell copies of the Software, and to permit persons to whom the
///             Software is furnished to do so, subject to the following conditions:\n
///             The above copyright notice and this permission notice shall be included in
///             all copies or substantial portions of the Software.\n
///             THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///             IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///             FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///             AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///             LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///             OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///             THE SOFTWARE.

// All includes are collected in one header
#include "XPMP2.h"

#if IBM
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2def.h>                                 // for WSACMSGHDR...
typedef SSIZE_T ssize_t;
#define IPV6_RECVPKTINFO IPV6_PKTINFO               // Windows doesn't define IPV6_RECVPKTINFO, but it seems there IPV6_PKTINFO does the trick
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#endif

namespace XPMP2 {

#if IBM
#undef errno
#define errno WSAGetLastError()     // https://docs.microsoft.com/en-us/windows/desktop/WinSock/error-codes-errno-h-errno-and-wsagetlasterror-2
#define close closesocket
typedef USHORT in_port_t;
#endif

constexpr int SERR_LEN = 100;                   // size of buffer for IO error texts (strerror_s)

//
// MARK: Local Network Interface Addresses
//

/// Stores information about local interface addresses
struct LocalIntfAddrTy
{
    uint32_t            intfIdx = 0;            ///< interface index
    std::string         intfName;               ///< interface name
    InetAddrTy          addr;                   ///< address, internally this is in_addr or in6_addr
    uint8_t             family = 0;             ///< either `AF_INET` or `AF_INET6`
    uint32_t            flags = 0;              ///< stuff like `IFF_MULTICAST`, `IFF_LOOPBACK`, `IFF_BROADCAST`...

    /// Standard constructor keeps all empty/invalid
    LocalIntfAddrTy () {}
    
    /// Constructor looks up interface idx from its name
    LocalIntfAddrTy (const char* _intfName,
                     const sockaddr* sa,
                     uint32_t _flags) :
    intfIdx(if_nametoindex(_intfName)), intfName(_intfName),
    addr(sa), family((uint8_t)sa->sa_family), flags(_flags)
    {}

    /// Constructor expects both interface name and index
    LocalIntfAddrTy(const char* _intfName, uint32_t _idx,
                    const sockaddr* sa,
                    uint32_t _flags) :
    intfIdx(_idx), intfName(_intfName),
    addr(sa), family((uint8_t)sa->sa_family), flags(_flags)
    {}

    /// equality is by interface and address
    bool operator== (const LocalIntfAddrTy& o) const
    { return intfIdx == o.intfIdx && addr == o.addr; }
    /// sorting order is by intf idx, then address; this guarantees that while traversing we see all addresses of one interface together
    bool operator< (const LocalIntfAddrTy& o) const
    { return
        intfIdx < o.intfIdx ||
        (intfIdx == o.intfIdx && addr < o.addr);
    }
    
    /// just compare the address
    bool operator== (const InetAddrTy& ia) const
    { return addr == ia; }
};

/// Set of local IP interface addresses
typedef std::set<LocalIntfAddrTy> SetLocalIntfAddrTy;

/// Set of local IP interface addresses
SetLocalIntfAddrTy gAddrLocal;
/// Mutex to safeguard access to `gAddrLocal`
std::recursive_mutex mtxAddrLocal;

/// Get an interface's name
std::string _NetwGetIntfName (uint32_t idx, uint8_t family)
{
    std::lock_guard<std::recursive_mutex> lock(mtxAddrLocal);
    
    // First search our own list of interfaces.
    // On Windows, that can stores differing intfIdx for IPv4 vs IPv6.
    // And it might be faster than going back to the OS
    SetLocalIntfAddrTy::const_iterator i =
        std::find_if(gAddrLocal.begin(), gAddrLocal.end(),
            [idx,family](const LocalIntfAddrTy& a) { return a.family == family && a.intfIdx == idx; });
    if (i != gAddrLocal.end())
        return i->intfName;

    // Not found in our own list, then try if_indextoname:
    char intfName[IF_NAMESIZE] = "";
    if (!if_indextoname(idx, intfName)) {
        LOG_MSG(logERR, "if_indextoname failed for index %u", idx);
        return "";
    }
    return intfName;
}

/// Find a matching local interface based on name and family
LocalIntfAddrTy _NetwFindLocalIntf (const std::string& intf, uint8_t family)
{
    if (!intf.empty()) {
        // a string full of digits? -> use as index
        uint32_t idx = 0;
        if (std::all_of(intf.begin(), intf.end(), ::isdigit))
            idx = (uint32_t)std::stol(intf);
        
        // Search our list of interfaces either by intf idx or by its name
        // On Windows, that can stores differing intfIdx for IPv4 vs IPv6.
        std::lock_guard<std::recursive_mutex> lock(mtxAddrLocal);
        SetLocalIntfAddrTy::const_iterator i =
        std::find_if(gAddrLocal.begin(), gAddrLocal.end(),
                     [intf,idx,family](const LocalIntfAddrTy& a)
                     { return a.family == family && (idx ? a.intfIdx == idx : a.intfName == intf); });
        if (i != gAddrLocal.end())
            return *i;
    }
    
    return LocalIntfAddrTy();
}

/// Fetch all local addresses and cache locally
void _NetwGetLocalAddresses (bool bForceRefresh = false)
{
    // Every once in a while we enforce the refresh
    static std::chrono::time_point<std::chrono::steady_clock> tpLastRefresh;
    if (std::chrono::steady_clock::now() - tpLastRefresh > std::chrono::seconds(30))
        bForceRefresh = true;
    
    // Quick exit
    if (!gAddrLocal.empty() && !bForceRefresh)
        return;
        
    std::lock_guard<std::recursive_mutex> lock(mtxAddrLocal);
    if (bForceRefresh)
        gAddrLocal.clear();
    
#if IBM
    // Fetch all local interface addresses
    ULONG ret = NO_ERROR;
    ULONG bufLen = 15000;
    std::unique_ptr<IP_ADAPTER_ADDRESSES, decltype(&free)> pAddresses(nullptr, &free);
    do {
        // allocate a return buffer
        pAddresses.reset((IP_ADAPTER_ADDRESSES*)malloc(bufLen));
        if (!pAddresses) {
            LOG_MSG(logERR, "malloc failed for %lu bytes", bufLen);
            return;
        }
        // try fetching all local addresses
        ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses.get(), &bufLen);
        switch (ret) {
        case ERROR_NO_DATA:                 // no addresses found (?)
            return;
        case ERROR_BUFFER_OVERFLOW:         // buffer too small, but bufLen has the required size now, try again
            break;
        case NO_ERROR:                      // success
            break;
        default:
            LOG_MSG(logERR, "GetAdaptersAddresses failed with error: %ld", ret);
            return;
        }
    } while (ret == ERROR_BUFFER_OVERFLOW);

    // Walk the list of addresses and add to our own vector
    for (const IP_ADAPTER_ADDRESSES* pAddr = pAddresses.get();
         pAddr;
         pAddr = pAddr->Next)
    {
        // Only interested in IPv4/6 enabled, operative interfaces
        if (!pAddr->Ipv4Enabled && !pAddr->Ipv6Enabled)
            continue;
        if (pAddr->OperStatus != IfOperStatusUp)
            continue;

        // Convert the friendly name from wchar to char
        char ifName[26] = { 0 };                                        // zero init and passing in sizeof-1 guarantees zero-termination
        std::wcstombs(ifName, pAddr->FriendlyName, sizeof(ifName)-1);
        
        // Compiler some flags in Unix style
        uint32_t flags = IFF_UP | IFF_BROADCAST;                        // We just _assume_ it's broadcast-enabled...can't find a flag here that would tell us
        if (!pAddr->NoMulticast) flags |= IFF_MULTICAST;
        if (pAddr->IfType == IF_TYPE_SOFTWARE_LOOPBACK) flags |= IFF_LOOPBACK;
            
        // Walk the list of unicast addresses
        for (const IP_ADAPTER_UNICAST_ADDRESS* pUni = pAddr->FirstUnicastAddress;
             pUni;
             pUni = pUni->Next)
        {
            const sockaddr* pSAddr = pUni->Address.lpSockaddr;
            if (!pSAddr) continue;

            gAddrLocal.emplace(
                ifName,
                uint32_t(pSAddr->sa_family == AF_INET6 ? pAddr->Ipv6IfIndex : pAddr->IfIndex),
                pSAddr,
                flags);
        }
    }
#else
    // Fetch all local interface addresses
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1)
        throw NetRuntimeError("getifaddrs failed");

    // add valid addresses to our local cache array
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        // ignore empty addresses and everything that is not IP
        if (ifa->ifa_addr == NULL)
            continue;
        
        // We are only interested in up&running interfaces
        if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING))
            continue;
        // We are interested in IPv4 and IPv6 interfaces only
        if (ifa->ifa_addr->sa_family != AF_INET &&
            ifa->ifa_addr->sa_family != AF_INET6)
            continue;

        // the others add to our list of local addresses
        gAddrLocal.emplace(ifa->ifa_name,
                           ifa->ifa_addr,
                           ifa->ifa_flags);
    }
    
    // free the return list
    freeifaddrs(ifaddr);
#endif
    
    // Next refresh may waitr
    tpLastRefresh = std::chrono::steady_clock::now();
}


/// Internal: Returns the _next_ interface index of a broadcast interface for the given protocol family
uint32_t _GetIntfNext (const uint32_t prevIdx, SetLocalIntfAddrTy::const_iterator& i,
                       uint8_t family, uint32_t fMust, uint32_t fSkip)
{
    for (; i != gAddrLocal.end(); ++i)              // search for the next _different_ intf idx,
    {                                               // of the right family type
        if (i->intfIdx != prevIdx && i->family == family &&
            ((i->flags & fMust) == fMust) &&        // having all must-have flags
            ((i->flags & fSkip) == 0))              // and none of the skip flags
            return i->intfIdx;
    }
    return 0;
}

/// Returns the _first_ interface index of a broadcast interface for the given protocol family
uint32_t GetIntfFirst (SetLocalIntfAddrTy::const_iterator& i, uint8_t family,
                       uint32_t fMust, uint32_t fSkip = 0)
{
    std::lock_guard<std::recursive_mutex> lock(mtxAddrLocal);
    return _GetIntfNext(0, i = gAddrLocal.begin(), family, fMust, fSkip);
}

/// Returns the _next_ interface index of a broadcast interface for the given protocol family
uint32_t GetIntfNext (SetLocalIntfAddrTy::const_iterator& i, uint8_t family,
                      uint32_t fMust, uint32_t fSkip = 0)
{
    std::lock_guard<std::recursive_mutex> lock(mtxAddrLocal);
    if (i == gAddrLocal.end()) return 0;                        // safeguard
    const uint32_t idx = i->intfIdx;                            // index we point to _now_ (before increment)
    return _GetIntfNext(idx, ++i, family, fMust, fSkip);        // increment already to next entry in list
}

// Return list of known local interfaces
std::vector<std::string> NetwGetInterfaces (uint8_t family, uint32_t fMust, uint32_t fSkip)
{
    std::vector<std::string> vIntf;
    SetLocalIntfAddrTy::const_iterator i;
    for (GetIntfFirst(i, family, fMust, fSkip);
         i != gAddrLocal.end();
         GetIntfNext(i, family, fMust, fSkip))
    {
        vIntf.push_back(i->intfName);
    }
    return vIntf;
}

// Return comma-separated string will all known local interfaces, calls NetwGetInterfaces()
std::string NetwGetInterfaceNames (uint8_t family, uint32_t fMust, uint32_t fSkip)
{
    std::string ret;
    const std::vector<std::string> vIntf = NetwGetInterfaces(family, fMust, fSkip);
    for (const std::string& intf: vIntf) {
        if (!ret.empty()) ret += ',';
        ret += intf;
    }
    return ret;
}


// Constructor copies given socket info
SockAddrTy::SockAddrTy (const sockaddr* pSa)
{
    memset((void*)this, 0, sizeof(SockAddrTy));
    if (pSa) {
        switch (pSa->sa_family) {
            case AF_INET:
                memcpy(&sa_in, pSa, sizeof(sa_in));
                break;
            case AF_INET6:
                memcpy(&sa_in6, pSa, sizeof(sa_in6));
                break;
            default:
                memcpy(&sa, pSa, sizeof(sa));
        }
    }
}


//
// MARK: SocketNetworking
//

/// Mutex to ensure closing is done in one thread only to avoid race conditions on deleting the buffer
std::recursive_mutex mtxSocketClose;

NetRuntimeError::NetRuntimeError(const std::string& w) :
std::runtime_error(w), fullWhat(w)
{
    // make network error available
    char sErr[SERR_LEN];
    strerror_s(sErr, sizeof(sErr), errno);
    errTxt = sErr;              // copy
    // And the complete message to be returned by what readily prepared:
    fullWhat += ": ";
    fullWhat += errTxt;
}

// cleanup: make sure the socket is closed and all memory cleanup up
SocketNetworking::~SocketNetworking()
{
    Close();
}

void SocketNetworking::Open(const std::string& _addr, int _port,
                       size_t _bufSize, unsigned _timeOut_ms, bool _bBroadcast)
{
    struct addrinfo *   addrinfo      = NULL;
    try {
        // store member values
        f_port = _port;
        f_addr = _addr;
        const std::string decimal_port(std::to_string(f_port));

        // get a valid address based on inAddr/port
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        GetAddrHints(hints);            // ask subclasses
        
        // Any address?
        if (f_addr.empty())
            f_addr = hints.ai_family == AF_INET6 ? "::" : "0.0.0.0";
        
        // Need to get
        int r = getaddrinfo(f_addr.c_str(),
                            decimal_port.c_str(), &hints, &addrinfo);
        if(r != 0 || addrinfo == NULL)
            throw NetRuntimeError("invalid address or port for socket: \"" + f_addr + ":" + decimal_port + "\"");
        
        // get a socket
        f_socket = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
        if(f_socket == INVALID_SOCKET)
            throw NetRuntimeError("could not create socket for: \"" + f_addr + ":" + decimal_port + "\"");
        
        // Reuse address and port to allow others to connect, too
        int setToVal = 1;
#if IBM
        if (setsockopt(f_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&setToVal, sizeof(setToVal)) < 0)
            throw NetRuntimeError("could not setsockopt SO_REUSEADDR for: \"" + f_addr + ":" + decimal_port + "\"");
#else
        if (setsockopt(f_socket, SOL_SOCKET, SO_REUSEADDR, &setToVal, sizeof(setToVal)) < 0)
            throw NetRuntimeError("could not setsockopt SO_REUSEADDR for: \"" + f_addr + ":" + decimal_port + "\"");
        if (setsockopt(f_socket, SOL_SOCKET, SO_REUSEPORT, &setToVal, sizeof(setToVal)) < 0)
            throw NetRuntimeError("could not setsockopt SO_REUSEPORT for: \"" + f_addr + ":" + decimal_port + "\"");
#endif

        // define receive timeout
#if IBM
        DWORD wsTimeout = _timeOut_ms;
        if (setsockopt(f_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&wsTimeout, sizeof(wsTimeout)) < 0)
            throw NetRuntimeError("could not setsockopt SO_RCVTIMEO for: \"" + f_addr + ":" + decimal_port + "\"");
#else
        struct timeval timeout;
        timeout.tv_sec = _timeOut_ms / 1000;
        timeout.tv_usec = (_timeOut_ms % 1000) * 1000;
        if (setsockopt(f_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
            throw NetRuntimeError("could not setsockopt SO_RCVTIMEO for: \"" + f_addr + ":" + decimal_port + "\"");
#endif
        
        // if requested allow for sending broadcasts
        if (_bBroadcast) {
            setToVal = 1;
#if IBM
            if (setsockopt(f_socket, SOL_SOCKET, SO_BROADCAST, (char*)&setToVal, sizeof(setToVal)) < 0)
                throw NetRuntimeError("could not setsockopt SO_BROADCAST for: \"" + f_addr + ":" + decimal_port + "\"");
#else
            if (setsockopt(f_socket, SOL_SOCKET, SO_BROADCAST, &setToVal, sizeof(setToVal)) < 0)
                throw NetRuntimeError("could not setsockopt SO_BROADCAST for: \"" + f_addr + ":" + decimal_port + "\"");
#endif
        }

        // bind the socket to the address:port
        r = bind(f_socket, addrinfo->ai_addr, (socklen_t)addrinfo->ai_addrlen);
        if(r != 0)
            throw NetRuntimeError("could not bind UDP socket with: \"" + f_addr + ":" + decimal_port + "\"");

        // free adress info
        freeaddrinfo(addrinfo);
        addrinfo = NULL;

        // reserve receive buffer
        SetBufSize(_bufSize);
    }
    catch (...) {
        // free adress info
        if (addrinfo) {
            freeaddrinfo(addrinfo);
            addrinfo = NULL;
        }
        // make sure everything is closed
        Close();
        // re-throw
        throw;
    }
}

// Connect to a server
void SocketNetworking::Connect(const std::string& _addr, int _port,
                               size_t _bufSize, unsigned _timeOut_ms)
{
    struct addrinfo *   addrinfo      = NULL;
    try {
        // store member values
        f_port = _port;
        f_addr = _addr;
        const std::string decimal_port(std::to_string(f_port));

        // get a valid address based on inAddr/port
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        GetAddrHints(hints);            // ask subclasses, but then overwrite some stuff
        hints.ai_flags &= ~AI_PASSIVE;
        
        int r = getaddrinfo(f_addr.c_str(), decimal_port.c_str(), &hints, &addrinfo);
        if(r != 0 || addrinfo == NULL)
            throw NetRuntimeError("invalid address or port for socket: \"" + f_addr + ":" + decimal_port + "\"");
        
        // get a socket
        f_socket = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
        if(f_socket == INVALID_SOCKET)
            throw NetRuntimeError("could not create socket for: \"" + f_addr + ":" + decimal_port + "\"");
        
        // define receive timeout
#if IBM
        DWORD wsTimeout = _timeOut_ms;
        if (setsockopt(f_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&wsTimeout, sizeof(wsTimeout)) < 0)
            throw NetRuntimeError("could not setsockopt SO_RCVTIMEO for: \"" + f_addr + ":" + decimal_port + "\"");
#else
        struct timeval timeout;
        timeout.tv_sec = _timeOut_ms / 1000;
        timeout.tv_usec = (_timeOut_ms % 1000) * 1000;
        if (setsockopt(f_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
            throw NetRuntimeError("could not setsockopt SO_RCVTIMEO for: \"" + f_addr + ":" + decimal_port + "\"");
#endif
        
        // actually connect
        r = connect(f_socket, addrinfo->ai_addr, (socklen_t)addrinfo->ai_addrlen);
        if(r != 0)
            throw NetRuntimeError("could not connect to: \"" + f_addr + ":" + decimal_port + "\"");

        // free adress info
        freeaddrinfo(addrinfo);
        addrinfo = NULL;

        // reserve receive buffer
        SetBufSize(_bufSize);
    }
    catch (...) {
        // free adress info
        if (addrinfo) {
            freeaddrinfo(addrinfo);
            addrinfo = NULL;
        }
        // make sure everything is closed
        Close();
        // re-throw
        throw;
    }
}

/** \brief Clean up the UDP server.
 *
 // Close: This function frees the address info structures and close the socket.
 */
void SocketNetworking::Close()
{
    std::lock_guard<std::recursive_mutex> lock(mtxSocketClose);
    // cleanup
    if (f_socket != INVALID_SOCKET) {
        close(f_socket);
        f_socket = INVALID_SOCKET;
    }
    
    // release buffer
    SetBufSize(0);
}

// allocates the receiving buffer
void SocketNetworking::SetBufSize(size_t _bufSize)
{
    std::lock_guard<std::recursive_mutex> lock(mtxSocketClose);
    // remove existing buffer
    if (buf) {
        delete[] buf;
        buf = NULL;
        bufSize = 0;
    }
    
    // create a new one
    if (_bufSize > 0) {
        buf = new char[bufSize=_bufSize];
        memset(buf, 0, bufSize);
    }
}

// updates the error text and returns it
std::string SocketNetworking::GetLastErr()
{
    char sErr[SERR_LEN];
#if IBM
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,   // flags
        NULL,                                                                   // lpsource
        WSAGetLastError(),                                                      // message id
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),                              // languageid
        sErr,                                                                   // output buffer
        sizeof(sErr),                                                           // size of msgbuf, bytes
        NULL);
#else
    strerror_s(sErr, sizeof(sErr), errno);
#endif
    return std::string(sErr);
}

// Set blocking mode
/// @see https://stackoverflow.com/a/1549344
void SocketNetworking::setBlocking (bool bBlock)
{
    LOG_ASSERT(f_socket != INVALID_SOCKET);

 #if IBM
    unsigned long mode = bBlock ? 0 : 1;
    if (ioctlsocket(f_socket, FIONBIO, &mode) == SOCKET_ERROR)
        throw NetRuntimeError("ioctlsocket failed to set blocking flag");
 #else
    int flags = fcntl(f_socket, F_GETFL, 0);
    if (flags == -1)
        throw NetRuntimeError("fcntl failed to get flags");
    flags = bBlock ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    if (fcntl(f_socket, F_SETFL, flags) < 0)
        throw NetRuntimeError("fcntl failed to set blocking flag");
 #endif
}

/** \brief Waits to receive a message, ensures zero-termination in the buffer
 *
 * This function waits until a message is received on this UDP server.
 * There are no means to return from this function except by receiving
 * a message. Remember that UDP does not have a connect state so whether
 * another process quits does not change the status of this UDP server
 * and thus it continues to wait forever.
 *
 * Note that you may change the type of socket by making it non-blocking
 * (use the `get_socket()` to retrieve the socket identifier) in which
 * case this function will not block if no message is available. Instead
 * it returns immediately.
 */
long SocketNetworking::recv(std::string* _pFromAddr,
                            SockAddrTy* _pFromSockAddr)
{
    if (!buf) {
#if IBM
        WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
#else
        errno = ENOMEM;
#endif
        return -1;
    }
    
    SockAddrTy  safrom;
    socklen_t   fromlen = sizeof(safrom);
    long ret =
#if IBM
    (long) recvfrom(f_socket, buf, (int)bufSize, 0, &safrom.sa, &fromlen);
#else
    recvfrom(f_socket, buf, bufSize, 0, &safrom.sa, &fromlen);
#endif
    if (ret >= 0)  {                    // we did receive something
        buf[ret] = 0;                   // zero-termination
    } else {
        buf[0] = 0;                     // empty string
    }
    
    // Sender address wanted?
    if (_pFromAddr)
        *_pFromAddr = GetAddrString(safrom);
    if (_pFromSockAddr)
        *_pFromSockAddr = safrom;
    
    return ret;
}

/** \brief Waits to receive a message with timeout, ensures zero-termination in the buffer
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after `max_wait_ms`, the function returns with `-1` and
 * `errno` set to `EAGAIN`.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * `max_wait_ms`. It may return sooner with an error or a message.
 *
 */
long SocketNetworking::timedRecv(int max_wait_ms,
                                 std::string* _pFromAddr,
                                 SockAddrTy* _pFromSockAddr)
{
    fd_set sRead, sErr;
    struct timeval timeout;

    FD_ZERO(&sRead);
    FD_SET(f_socket, &sRead);           // check our socket
    FD_ZERO(&sErr);                     // also for errors
    FD_SET(f_socket, &sErr);

    timeout.tv_sec = max_wait_ms / 1000;
    timeout.tv_usec = (max_wait_ms % 1000) * 1000;
    int retval = select((int)f_socket + 1, &sRead, NULL, &sErr, &timeout);
    if(retval == -1)
    {
        // select() set errno accordingly
        buf[0] = 0;                     // empty string
        return -1;
    }
    if(retval > 0)
    {
        // was it an error that triggered?
        if (FD_ISSET(f_socket,&sErr)) {
            return -1;
        }
        
        // our socket has data
        if (FD_ISSET(f_socket, &sRead))
            return recv(_pFromAddr, _pFromSockAddr);
    }
    
    // our socket has no data
    buf[0] = 0;                     // empty string
#if IBM
    WSASetLastError(WSAEWOULDBLOCK);
#else
    errno = EAGAIN;
#endif
    return -1;
}

// write a message out
bool SocketNetworking::send(const char* msg)
{
    if (f_socket == INVALID_SOCKET)
        throw ("send: Undefined Socket");

    // Loop to send the message in potentially several pieces
    int index=0;
    int length = (int)strlen(msg);
    while (index<length) {
        int count = (int)::send(f_socket, msg + index, (socklen_t)(length - index), 0);
        if (count<0) {
            if (errno==EINTR) continue;
            LOG_MSG(logERR, "send to %s failed: %s",
                    f_addr.c_str(), GetLastErr().c_str());
            return false;
        } else {
            index+=count;
        }
    }
    return true;
}

// sends the message as a broadcast
bool SocketNetworking::broadcast (const char* msg)
{
    int index=0;
    int length = (int)strlen(msg);
    
    struct sockaddr_in s;
    memset(&s, '\0', sizeof(struct sockaddr_in));
    s.sin_family = AF_INET;
    s.sin_port = htons((in_port_t)f_port);
    s.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    
    while (index<length) {
        int count = (int)::sendto(f_socket, msg + index, (socklen_t)(length - index), 0,
                                  (struct sockaddr *)&s, sizeof(s));
        if (count<0) {
            if (errno==EINTR) continue;
            LOG_MSG(logERR, "%s (%s)",
                    ("sendto failed: \"" + f_addr + ":" + std::to_string(f_port) + "\"").c_str(),
                    GetLastErr().c_str());
            return false;
        } else {
            index+=count;
        }
    }
    return true;
}


// return a string for a IPv4 and IPv6 address
std::string SocketNetworking::GetAddrString (const SockAddrTy& sa, bool withPort)
{
    std::string ret;
    char host[INET6_ADDRSTRLEN] = "", service[NI_MAXSERV] = "";
    
    int gai_err = getnameinfo(&sa.sa, sa.size(),
                              host, sizeof(host),
                              service, sizeof(service), NI_NUMERICSERV | NI_NUMERICHOST);
    if (gai_err)
        throw std::runtime_error(gai_strerror(gai_err));

    // combine host and service as requested
    if (withPort) {
        if (sa.isIp6()) ret = '[';
        ret += host;
        if (sa.isIp6()) ret += "]:"; else ret += ':';
        ret += service;
    } else {
        ret = host;
    }
    return ret;
}

#if IBM
// Wrapper around Windows' weird way of hiding WSAREcvMsg: Finds the pointer to the function, then calls it
int SocketNetworking::WSARecvMsg(
    LPWSAMSG lpMsg,
    LPDWORD lpdwNumberOfBytesRecvd,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    // Don't have the pointer to the WSARecvMsg function yet?
    if (!pWSARecvMsg)
    {
        // @see https://stackoverflow.com/a/37334943
        GUID guid = WSAID_WSARECVMSG;
        DWORD dwBytesReturned = 0;
        if (WSAIoctl(f_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &pWSARecvMsg, sizeof(pWSARecvMsg), &dwBytesReturned, NULL, NULL) != 0)
            throw NetRuntimeError("WSAIoctl did not return the LPFN_WSARECVMSG pointer");
        LOG_ASSERT(pWSARecvMsg);
    }
    // make the call
    return (pWSARecvMsg)(f_socket, lpMsg, lpdwNumberOfBytesRecvd, lpOverlapped, lpCompletionRoutine);
}
#endif // IBM

//
// MARK: UDPReceiver
//

// UDP only allows UDP
void UDPReceiver::GetAddrHints (struct addrinfo& hints)
{
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
}

//
// MARK: UDPMulticast
//

// Constructor
UDPMulticast::UDPMulticast(const std::string& _multicastAddr, int _port,
                           const std::string& _sendIntf,
                           int _ttl, size_t _bufSize, unsigned _timeOut_ms) :
    SocketNetworking()
{
    Join(_multicastAddr, _port, _sendIntf, _ttl, _bufSize, _timeOut_ms);
}

// makes sure pMCAddr is cleared up
UDPMulticast::~UDPMulticast()
{
    Cleanup();
}

// Connect to the multicast group
void UDPMulticast::Join (const std::string& _multicastAddr, int _port,
                         const std::string& _sendIntf,
                         int _ttl, size_t _bufSize, unsigned _timeOut_ms)
{
    // Already connected? -> disconnect first
    if (isOpen())
        Close();
    
    // save all values
    f_port = _port;
    const std::string decimal_port(std::to_string(f_port));
    
    // Resolve the multicast address, also determines AI Family
    Cleanup();
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo(_multicastAddr.c_str(), decimal_port.c_str(), &hints, &pMCAddr) != 0)
        throw NetRuntimeError ("getaddrinfo failed for " + _multicastAddr);
    if (pMCAddr == nullptr)
        throw NetRuntimeError ("No address info found for " + _multicastAddr);
    if (pMCAddr->ai_family != AF_INET && pMCAddr->ai_family != AF_INET6)
        throw NetRuntimeError ("Multicast address " + _multicastAddr + " was not resolved as IPv4 or IPv6 address but as " + std::to_string(pMCAddr->ai_family));

    // fill our string with a nicely formatted string
    multicastAddr = GetAddrString(pMCAddr->ai_addr);

    // Make sure all local addresses are known, are needed during the MC send process
    std::lock_guard<std::recursive_mutex> lock(mtxAddrLocal);
    _NetwGetLocalAddresses(true);
    // If configured, lookup a specific interface (address) to send from and join only
    const LocalIntfAddrTy sendIntf = _NetwFindLocalIntf(_sendIntf, GetFamily());
    if (!_sendIntf.empty() && !sendIntf.intfIdx) {
        LOG_MSG(logERR, "MC %s: Configured remoteSendIntf '%s' not found! Available interfaces are: %s",
                GetMCAddr().c_str(), _sendIntf.c_str(), NetwGetInterfaceNames(GetFamily(), IFF_MULTICAST).c_str());
    }
    
    // open the socket
    Open("", _port, _bufSize, _timeOut_ms);

    // Actually join the multicast group
    if (IsIPv4())
    {
        // Setup the v4 option values and ip_mreq structure
        const int on = 1;
        if (setsockopt(f_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&_ttl, sizeof(_ttl)) < 0)
            throw NetRuntimeError ("setsockopt(IP_MULTICAST_TTL) failed");
        if (setsockopt(f_socket, IPPROTO_IP, IP_PKTINFO, (char*)&on, sizeof(on)) < 0)
            throw NetRuntimeError ("setsockopt(IP_PKTINFO) failed");
        
        // Join multicast on all interfaces separately
        // (Theoretically joining once with INADDR_ANY is sufficient,
        //  but tests show that specifically joining all interfaces is more reliable.)
        ip_mreq mreqv4 = {};
        mreqv4.imr_multiaddr.s_addr = ((sockaddr_in*)pMCAddr->ai_addr)->sin_addr.s_addr;

        std::string sIntfJoined;
        SetLocalIntfAddrTy::const_iterator i;
        for (GetIntfFirst(i, AF_INET, IFF_MULTICAST);
             i != gAddrLocal.end();
             GetIntfNext(i, AF_INET, IFF_MULTICAST))
        {
            // If a single interface is configured then we only join on that interface
            if (sendIntf.intfIdx && sendIntf.intfIdx != i->intfIdx)
                continue;
            
            // Set the interface's address and join
            mreqv4.imr_interface.s_addr = i->addr.in_addr.s_addr;
            if (setsockopt(f_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*) &mreqv4, sizeof(mreqv4)) < 0) {
                char sErr[SERR_LEN];
                strerror_s(sErr, sizeof(sErr), errno);
                LOG_MSG(logWARN, "MC %s: setsockopt(IP_ADD_MEMBERSHIP) failed for intf %s: %d - %s",
                        multicastAddr.c_str(), i->intfName.c_str(),
                        errno, sErr);
            } else {
                // Success:
                if (!sIntfJoined.empty()) sIntfJoined += ',';
                sIntfJoined += i->intfName;
            }
        }
        // not a single interface accepted me?
        if (sIntfJoined.empty())
            throw NetRuntimeError ("setsockopt(IP_ADD_MEMBERSHIP) failed for all interfaces!");
        
        LOG_MSG(logDEBUG, "MC %s: Joined on interfaces %s",
                multicastAddr.c_str(), sIntfJoined.c_str());
    }
    else    // AF_INET6
    {
        const int on = 1;
        setsockopt(f_socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (const char*) &on, sizeof(on));
        if (setsockopt(f_socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char*)&_ttl, sizeof(_ttl)) < 0)
            throw NetRuntimeError ("setsockopt(IPV6_MULTICAST_HOPS) failed");
        if (setsockopt(f_socket, IPPROTO_IPV6, IPV6_RECVPKTINFO, (char*)&on, sizeof(on)) < 0)
            throw NetRuntimeError ("setsockopt(IPV6_RECVPKTINFO) failed");

        // Setup the v6 option values and ipv6_mreq structure
        ipv6_mreq mreqv6 = {};
        mreqv6.ipv6mr_multiaddr = ((sockaddr_in6*)pMCAddr->ai_addr)->sin6_addr;
        
        // Need to join for all interface separately
        std::string sIntfJoined;
        SetLocalIntfAddrTy::const_iterator i;
        for (mreqv6.ipv6mr_interface = GetIntfFirst(i, AF_INET6, IFF_MULTICAST);
             mreqv6.ipv6mr_interface > 0;
             mreqv6.ipv6mr_interface = GetIntfNext(i, AF_INET6, IFF_MULTICAST))
        {
            // If a single interface is configured then we only joing on that interface
            if (sendIntf.intfIdx && sendIntf.intfIdx != mreqv6.ipv6mr_interface)
                continue;
            
            if (setsockopt(f_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char*) &mreqv6, sizeof(mreqv6)) < 0) {
                char sErr[SERR_LEN];
                strerror_s(sErr, sizeof(sErr), errno);
                LOG_MSG(logWARN, "MC %s: setsockopt(IPV6_JOIN_GROUP) failed for intf %s: %d - %s",
                        multicastAddr.c_str(), i->intfName.c_str(),
                        errno, sErr);
            } else {
                // Success:
                if (!sIntfJoined.empty()) sIntfJoined += ',';
                sIntfJoined += i->intfName;
            }
        }
        // not a single interface accepted me?
        if (sIntfJoined.empty())
            throw NetRuntimeError ("setsockopt(IPV6_JOIN_GROUP) failed for all interfaces!");
        
        LOG_MSG(logDEBUG, "MC %s: Joined on interfaces %s",
                multicastAddr.c_str(), sIntfJoined.c_str());
    }

    // If a sending interface is given let's set it
    if (sendIntf.intfIdx)
        SendToIntf(sendIntf.intfIdx);
}

// Send future datagrams on default interfaces only (default)
void UDPMulticast::SendToDefault ()
{
    if (!pMCAddr) throw NetRuntimeError("Multicast address not yet defined");
    
    // Set the send interface to default
    if (IsIPv4()) {                                 // IPv4
        SetSendInterface(in_addr{INADDR_ANY});
        LOG_MSG(logDEBUG, "MC %s: Sending via default interface",
                multicastAddr.c_str());
    }
    else if (IsIPv6()) {                            // IPv6 doesn't allow assigning `0` generally, so we pick the first from our list
        SetLocalIntfAddrTy::const_iterator i;
        SetSendInterface(GetIntfFirst(i, AF_INET6, IFF_MULTICAST));
        if (i != gAddrLocal.end()) {
            LOG_MSG(logDEBUG, "MC %s: Sending via interface '%s'",
                    multicastAddr.c_str(), i->intfName.c_str());
        }
    }
    bSendToAll = false;
    oneIntfIdx = 0;
}

// Send future datagrams on _all_ interfaces
void UDPMulticast::SendToAll ()
{
    _NetwGetLocalAddresses ();              // makes sure lists of multicast interfaces are filled, too
    if ((bSendToAll = !gAddrLocal.empty())) {
        oneIntfIdx = 0;
        LOG_MSG(logINFO, "MC %s: Sending on ALL interfaces: %s",
                multicastAddr.c_str(), NetwGetInterfaceNames(GetFamily(), IFF_MULTICAST).c_str());
    }
}

// Send future datagrams on this particular interface only
void UDPMulticast::SendToIntf (uint32_t idx)
{
    if (idx == oneIntfIdx) return;                  // no change, quick exit
    if (!pMCAddr) throw NetRuntimeError("Multicast address not yet defined");
    
    // Let's get the interfce's name first, also as a minimal check for the interface's existance
    const std::string intfName = _NetwGetIntfName(idx, GetFamily());
    if (intfName.empty())
        throw NetRuntimeError("if_indextoname failed");
    
    // Actually set the interface
    SetSendInterface(idx);
    bSendToAll = false;
    oneIntfIdx = idx;
    LOG_MSG(logINFO, "MC %s: Sending via interface '%s' only",
            multicastAddr.c_str(), intfName.c_str());
}


// Send a multicast
size_t UDPMulticast::SendMC (const void* _bufSend, size_t _bufSendSize)
{
    ssize_t bytesSent = 0;
    if (!pMCAddr || !isOpen())
        throw NetRuntimeError("SendMC: Multicast socket not open");
    
    // If sending to all we want to update the list of interfaces every once in a while
    if (bSendToAll)
        _NetwGetLocalAddresses();

    // We start a loop over all broadcast interfaces, exit early in case of !bSendToAll
    int numIntfSent = 0;
    SetLocalIntfAddrTy::const_iterator i;
    for (uint32_t idx = bSendToAll ? GetIntfFirst(i, GetFamily(), IFF_MULTICAST) : 1;
         idx > 0;
         idx = GetIntfNext(i, GetFamily(), IFF_MULTICAST))
    {
        // Set the sending interface if we are looping over all interfaces
        if (bSendToAll) {
            try {
                if (IsIPv4())
                    SetSendInterface(i->addr.in_addr);
                else
                    SetSendInterface(idx);
            }
            catch (const NetRuntimeError&) {
                // We ignore network errors, but just try the net interface
                continue;
            }
        }
        
        // Do the actual sending
        bytesSent =
    #if IBM
        (ssize_t)sendto(f_socket, (const char*)_bufSend, (int)_bufSendSize, 0, pMCAddr->ai_addr, (int)pMCAddr->ai_addrlen);
    #else
        sendto(f_socket, _bufSend, _bufSendSize, 0, pMCAddr->ai_addr, pMCAddr->ai_addrlen);
    #endif
        if (bytesSent < 0)
            throw NetRuntimeError("SendMC: sendto failed to send " + std::to_string(_bufSendSize) + " bytes");
        
        // Sent to one more interface
        ++numIntfSent;
        
        // exit early if we were to send to one interface only
        if (!bSendToAll)
            break;
    }
    
    // Did not send our data anywhere?
    if (!numIntfSent)
        throw NetRuntimeError("SendMC could not send to any interface");
    
    return size_t(bytesSent);
}

// @brief Receive a multicast
size_t UDPMulticast::RecvMC (bool bSwitchToRecvIntf,
                             std::string* _pFromAddr,
                             SockAddrTy* _pFromSockAddr,
                             bool* _pbIntfChanged)
{

    if (!pMCAddr || !buf || !isOpen())
        throw NetRuntimeError("RecvMC: Multicast socket not initialized");
    
    // Information vairables filled during the rcvmsg operation
    SockAddrTy sa;                              // socket address
    sa.sa.sa_family = GetFamily();
    uint32_t ifIdx = 0;                         // interface index the msg was received on

    memset(buf, 0, bufSize);
    ssize_t bytesRcvd = 0;
#if IBM
    // Set up msg header
    // Originally based on https://stackoverflow.com/a/37334943
    char msg_control[1024] = "";
    WSABUF  wsaBuf = { static_cast<ULONG>(bufSize), buf };
    DWORD br = 0;
    WSAMSG  msg = {
        &sa.sa,                 // LPSOCKADDR       name;              /* Remote address */
        sa.size(),              // INT              namelen;           /* Remote address length */
        &wsaBuf,                // LPWSABUF         lpBuffers;         /* Data buffer array */
        1,                      // ULONG            dwBufferCount;     /* Number of elements in the array */
        {                       // WSABUF           Control;           /* Control buffer */
            sizeof(msg_control),
            msg_control
        },
        0                       // ULONG            dwFlags;           /* Flags */
    };

    // Receive the message, fill the control info
    if (WSARecvMsg(&msg, &br, nullptr, nullptr) < 0) {
        if (errno != WSAEWOULDBLOCK)            // we ignore no-data events
            throw NetRuntimeError("RecvMC: WSARecvMsg failed");
    }
    bytesRcvd = (ssize_t)br;

    // loop the control headers to find which interface the msg was received on
    if (bytesRcvd > 0 && bSwitchToRecvIntf) {
        for (WSACMSGHDR* cmsg = WSA_CMSG_FIRSTHDR(&msg);
             cmsg;
             cmsg = WSA_CMSG_NXTHDR(&msg, cmsg))
        {
            if (sa.isIp4() && cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO)
            {
                const struct in_pktinfo* in_pktinfo = (struct in_pktinfo*)WSA_CMSG_DATA(cmsg);
                ifIdx = in_pktinfo->ipi_ifindex;
                break;
            }
            if (sa.isIp6() && cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO)
            {
                const struct in6_pktinfo* in6_pktinfo = (struct in6_pktinfo*)WSA_CMSG_DATA(cmsg);
                ifIdx = in6_pktinfo->ipi6_ifindex;
                break;
            }
        }
    }

#else
    // Set up msg header
    // Originally based on https://stackoverflow.com/a/12398774
    char msg_control[1024];
    struct iovec iovec = { buf, bufSize };
    struct msghdr msg = {
        &sa.sa,                 //    void            *msg_name;      /* [XSI] optional address */
        sa.size(),              //    socklen_t       msg_namelen;    /* [XSI] size of address */
        &iovec,                 //    struct          iovec *msg_iov; /* [XSI] scatter/gather array */
        1,                      //    int             msg_iovlen;     /* [XSI] # elements in msg_iov */
        msg_control,            //    void            *msg_control;   /* [XSI] ancillary data, see below */
        sizeof(msg_control),    //    socklen_t       msg_controllen; /* [XSI] ancillary data buffer len */
        0                       //    int             msg_flags;      /* [XSI] flags on received message */
    };

    // Receive the message, fill the control info
    bytesRcvd = recvmsg(f_socket, &msg, 0);
    // We don't treat EAGAIN as an error...that just means we received nothing
    if (bytesRcvd < 0 && errno == EAGAIN)
        bytesRcvd = 0;
    if (bytesRcvd < 0)
        throw NetRuntimeError("RecvMC: recvfrom failed");

    // loop the control headers to find which interface the msg was received on
    if (bytesRcvd > 0 && bSwitchToRecvIntf) {
        for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg != 0; cmsg = CMSG_NXTHDR(&msg, cmsg))
        {
            if (sa.isIp4() && cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO)
            {
                const struct in_pktinfo* in_pktinfo = (struct in_pktinfo*)CMSG_DATA(cmsg);
                ifIdx = in_pktinfo->ipi_ifindex;
                break;
            }
            if (sa.isIp6() && cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO)
            {
                const struct in6_pktinfo* in6_pktinfo = (struct in6_pktinfo*)CMSG_DATA(cmsg);
                ifIdx = in6_pktinfo->ipi6_ifindex;
                break;
            }
        }
    }
#endif
    
    // Was that packet send from someplace we didn't hear yet or in some time?
    if (bSwitchToRecvIntf) {
        std::chrono::time_point<std::chrono::steady_clock> &lastRcvd = mapSender[sa];
        if (std::chrono::steady_clock::now() - lastRcvd < std::chrono::seconds(180))
            ifIdx = 0;                      // we know that sender and have heard from it in past 3 minutes, don't switch interface because of it
        lastRcvd = std::chrono::steady_clock::now();
    }
    
    // If requested, set outgoing interface (quick exit if no change)
    if (bSwitchToRecvIntf && ifIdx && ifIdx != oneIntfIdx) {
        SendToIntf(ifIdx);
        if (_pbIntfChanged) *_pbIntfChanged = true;
    }
    else
        if (_pbIntfChanged) *_pbIntfChanged = false;
        

    // Sender address wanted?
    if (_pFromAddr)
        *_pFromAddr = GetAddrString(sa);
    if (_pFromSockAddr)
        *_pFromSockAddr = sa;
    
    return size_t(bytesRcvd);
}

// frees pMCAddr
void UDPMulticast::Cleanup ()
{
    std::lock_guard<std::recursive_mutex> lock(mtxSocketClose);
    if (pMCAddr) {
        freeaddrinfo(pMCAddr);
        pMCAddr = nullptr;
    }
}


// returns information from `*pMCAddr`
void UDPMulticast::GetAddrHints (struct addrinfo& hints)
{
    assert(pMCAddr);                // must have called Join first! This will break in the debugger and we can investigate
    if (!pMCAddr) throw NetRuntimeError ("pMCAddr is NULL, didn't call Join before?");  // this kinda protects the release version

    hints.ai_family = pMCAddr->ai_family;
    hints.ai_socktype = pMCAddr->ai_socktype;
    hints.ai_protocol = pMCAddr->ai_protocol;
}

// Set multicast send interface (IPv4 only)
void UDPMulticast::SetSendInterface (const in_addr& addr)
{
    LOG_ASSERT(IsIPv4());
    if (setsockopt(f_socket, IPPROTO_IP, IP_MULTICAST_IF,
                   (const char*) &addr, sizeof(addr)) < 0)
        throw NetRuntimeError ("setsockopt(IP_MULTICAST_IF) failed");
}

// Set multicast send interface
void UDPMulticast::SetSendInterface (uint32_t ifIdx)
{
    // In case of IPv4 we need to translate the index to an address
    if (IsIPv4()) {
        std::unique_lock<std::recursive_mutex> lock(mtxAddrLocal);
        SetLocalIntfAddrTy::const_iterator i =
        std::find_if(gAddrLocal.begin(), gAddrLocal.end(),
                     [ifIdx](const LocalIntfAddrTy& a)
                     { return a.family == AF_INET && a.intfIdx == ifIdx; });
        if (i == gAddrLocal.end()) {
            LOG_MSG(logERR, "Found no IPv4 address for interface index %u", ifIdx);
            throw NetRuntimeError("No IPv4 address found for requested interface");
        }
        lock.unlock();
        SetSendInterface(i->addr.in_addr);
    }
    else if (IsIPv6()) {
        if (setsockopt(f_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                       (const char*) &ifIdx, sizeof(ifIdx)) < 0)
            throw NetRuntimeError ("setsockopt(IPV6_MULTICAST_IF) failed");
    }
}


//
// MARK: TCPConnection
//

void TCPConnection::Close()
{
    // close listener first
    CloseListenerOnly();

    // also close session connection
    if (f_session_socket != INVALID_SOCKET) {
        close(f_session_socket);
        f_session_socket = INVALID_SOCKET;
    }
}

// only closes the listening socket, but not a session connection
void TCPConnection::CloseListenerOnly()
{
#if APL == 1 || LIN == 1
    // Mac/Lin: Try writing something to the self-pipe to stop gracefully
    if (selfPipe[1] == INVALID_SOCKET ||
        write(selfPipe[1], "STOP", 4) < 0)
    {
        // if the self-pipe didn't work:
#endif
        // just call the base class Close, bypassing our own virtual function
        SocketNetworking::Close();
#if APL == 1 || LIN == 1
    }
#endif
}

// Listen for and accept connections
void TCPConnection::listen (int numConnections)
{
    if (::listen(f_socket, numConnections) < 0)
        throw NetRuntimeError("Can't listen on socket " + f_addr + ":" + std::to_string(f_port));
}

bool TCPConnection::accept (bool bUnlisten)
{
    socklen_t addrLen = sizeof(f_session_addr);
    memset (&f_session_addr, 0, sizeof(f_session_addr));
    
    // potentially blocking call
    f_session_socket = ::accept (f_socket, (struct sockaddr*)&f_session_addr, &addrLen);
    
    // if we are to "unlisten" then we close the listening socket
    if (f_session_socket != INVALID_SOCKET && bUnlisten) {
        CloseListenerOnly();
    }
    
    // successful?
    return f_session_socket != INVALID_SOCKET;
}

// just combines the above and includes a `select` on a self-pipe
// to be able to interrupt waiting at any time
bool TCPConnection::listenAccept (int numConnections)
{
    try {
        listen(numConnections);

#if APL == 1 || LIN == 1
        // the self-pipe to shut down the listener gracefully
        if (pipe(selfPipe) < 0)
            throw NetRuntimeError("Couldn't create self-pipe");
        const SOCKET readPipe = selfPipe[0];
        fcntl(readPipe, F_SETFL, O_NONBLOCK);
        
        // wait for an incoming connection or a signal on the pipe
        fd_set sRead;
        FD_ZERO(&sRead);
        FD_SET(f_socket, &sRead);       // check our listen sockets
        FD_SET(readPipe, &sRead);       // and the self-pipe
        const int maxSock = std::max(f_socket, selfPipe[0]) + 1;
        for(;;) {
            int retval = select(maxSock, &sRead, NULL, NULL, NULL);
            
            // select call failed?
            if(retval < 0)
                throw NetRuntimeError("'select' failed");
            
            // if anything is available
            if (retval > 0) {
                // something is available now, so we no longer need the self-pipe
                for (SOCKET &s: selfPipe) {
                    close(s);
                    s = INVALID_SOCKET;
                }

                // check for self-pipe...if so then just exit
                if (FD_ISSET(readPipe, &sRead))
                    return false;
            
                // exit loop if the listen-socket is ready to be read
                if (FD_ISSET(f_socket, &sRead))
                    break;
            }
        }
#endif
        // if we wait for exactly one connection then we "unlisten" once we accepted that one connection:
        return accept(numConnections == 1);
    }
    catch (std::exception& e) {
        LOG_MSG(logERR, "%s", e.what());
    }
    return false;
}

// write a message out
bool TCPConnection::send(const char* msg)
{
    // prefer the session socket set after waiting for a connection,
    // but try the main socket if session socket not set
    SOCKET f = f_session_socket == INVALID_SOCKET ? f_socket : f_session_socket;
    
    int index=0;
    int length = (int)strlen(msg);
    while (index<length) {
        int count = (int)::send(f, msg + index, (socklen_t)(length - index), 0);
        if (count<0) {
            if (errno==EINTR) continue;
            LOG_MSG(logERR, "%s (%s)",
                    ("send failed: \"" + f_addr + ":" + std::to_string(f_port) + "\"").c_str(),
                    GetLastErr().c_str());
            return false;
        } else {
            index+=count;
        }
    }
    return true;
}

// TCP only allows TCP
void TCPConnection::GetAddrHints (struct addrinfo& hints)
{
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
}

//
// MARK: IP address helpers
//

/// Extracts the numerical address and puts it into addr for generic address use
void InetAddrTy::CopyFrom (const SockAddrTy& sa)
{
    switch (sa.family()) {
        case AF_INET: {
            addr[0] = sa.sa_in.sin_addr.s_addr;
            addr[1] = addr[2] = addr[3] = 0;
            break;
        }
            
        case AF_INET6: {
            memcpy(addr, sa.sa_in6.sin6_addr.s6_addr, sizeof(addr));
            break;
        }
            
        default:
            memset(addr, 0, sizeof(addr));
            break;
    }
}



/// Is given address a local one?
bool NetwIsLocalAddr (const InetAddrTy& addr)
{
    std::lock_guard<std::recursive_mutex> lock(mtxAddrLocal);
    _NetwGetLocalAddresses();
    return std::find(gAddrLocal.begin(), gAddrLocal.end(), addr) != gAddrLocal.end();
}


} // namespace XPMP2
