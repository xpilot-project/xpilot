/// @file       Remote.h
/// @brief      Master/Client communication for aircraft synchronization
///             on remote networked instances
/// @details    The network thread tries not to fiddle with main thread's data
///             to reduce the need for synchronization through locks to the
///             bare minimum. Instead the main thread passes a limited information
///             set to the network thread by way of a deque.
/// @author     Birger Hoppe
/// @copyright  (c) 2020 Birger Hoppe
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

#ifndef _Remote_h_
#define _Remote_h_

namespace XPMP2 {

//
// MARK: ENQUEUE Data Structure for Caching and Passing from main to network thread
//

/// Holds a copy of some aircraft data as was sent out last
struct RmtAcCacheTy {
    const unsigned  fullUpdGrp;         ///< full update group, with which planes are distributed to load balance sending XPMP2::RMT_MSG_AC_DETAILED messages
    float ts;                           ///< timestamp this position was valid (XP network time)
    double lat;                         ///< degree latitude
    double lon;                         ///< degree longitude
    double alt_ft;                      ///< feet altitude
    XPLMDrawInfo_t  drawInfo;           ///< position and orientation last sent
    std::vector<float> v;               ///< dataRef values
    bool bValid     : 1;                ///< is this object valid? (Will be reset in case of exceptions)
    bool bVisible   : 1;                ///< Shall this plane be drawn at the moment?
    bool bRender    : 1;                ///< Shall the CSL model be drawn in 3D world?
    const CSLModel* pCSLMdl;            ///< the CSL model in use

    /// Constructor copies relevant values from the passed-in aircraft
    RmtAcCacheTy (const Aircraft& ac, double _lat, double _lon, double _alt_ft);
    /// Updates current values from given aircraft
    void UpdateFrom (const Aircraft& ac, double _lat, double _lon, double _alt_ft);
};

/// Defines a map with the plane id as key and the aboce cache structure as payload
typedef std::map<XPMPPlaneID,RmtAcCacheTy> mapRmtAcCacheTy;



/// Base structure for passing information from XP's main thread to the multicast thread
class RmtDataBaseTy {
public:
    RemoteMsgTy     msgTy;          ///< which message to send?
public:
    /// Constructor just set the message type
    RmtDataBaseTy (RemoteMsgTy _ty) : msgTy(_ty) {}
    /// Virtual destructor makes sure correct deletion of derived class' objects even through (smart) pointers to this base class
    virtual ~RmtDataBaseTy() {}
};

/// Smart pointer to XPMP2::RmtDataBaseTy by which we manage objects of this and derived types
typedef std::unique_ptr<RmtDataBaseTy> ptrRmtDataBaseTy;
/// A queue managing the information objects, used to transfer data between main thread and network thread
typedef std::queue<ptrRmtDataBaseTy,std::list<ptrRmtDataBaseTy> > queueRmtDataTy;


/// Passing information about any data type (like a/c full detail message); the data is contained as copy
template <class ElemTy, RemoteMsgTy MsgTy>
class RmtDataEnqeueTy : public RmtDataBaseTy {
public:
    /// the payload data
    ElemTy data;
public:
    /// Default constructor only sets message type, leaves rest defaulted
    RmtDataEnqeueTy () : RmtDataBaseTy(MsgTy) {}
    /// Data copy constructor copies the passed-in data
    RmtDataEnqeueTy (const ElemTy& _elem) : RmtDataBaseTy(MsgTy), data(_elem) {}
};

// predefine required template instances
typedef RmtDataEnqeueTy<RemoteAcDetailTy,RMT_MSG_AC_DETAILED> RmtDataAcDetailTy;            ///< A/C Detail queue type
typedef RmtDataEnqeueTy<RemoteAcPosUpdateTy,RMT_MSG_AC_POS_UPDATE> RmtDataAcPosUpdateTy;    ///< A/C Position Update queue type
typedef RmtDataEnqeueTy<RemoteAcRemoveTy,RMT_MSG_AC_REMOVE> RmtDataAcRemoveTy;                   ///< A/C Removal queue type

/// @brief Animation dataRef requires a special queue data type as each msg element is already of variable length
/// @details This queue structure provides for the maximum possible length.
///          As it is copied directly into the network message it requires to
///          be packed like a network message is with 1-byte alignment
class RmtDataAcAnimTy : public RmtDataBaseTy {
public:
    union {
        /// The animation dataRef values element
        RemoteAcAnimTy  data;
        /// a declaration that only makes sure that enough memory is reserved so that XPMP2::RmtDataAcAnimTy::data::v can fill up to max
        char            bufSize[RemoteAcAnimTy::msgSize(V_COUNT)];
    };
public:
    /// Default constructor only sets message type and id, makes sure `data` is initialized
    RmtDataAcAnimTy (XPMPPlaneID _id) : RmtDataBaseTy(RMT_MSG_AC_ANIM), data(_id) {}
    /// Add a pair of animation type and value to the structure
    void add (DR_VALS idx, float f);
};

/// Smart pointer to XPMP2::RmtDataAcAnimTy
typedef std::unique_ptr<RmtDataAcAnimTy> ptrRmtDataAcAnimTy;

//
// MARK: SENDING Data Structures
//

/// Helper class to manage the temporary buffers in which the network message are put together
template <class ElemTy, RemoteMsgTy MsgTy, std::uint8_t msgVer>
class RmtMsgBufTy {
public:
    void*           pMsg = nullptr;     ///< points to the actual message buffer of size glob.remoteBufSize
    size_t          elemCount = 0;      ///< number of elements already added to the message
    size_t          size = 0;           ///< current message size
public:
    /// Constructor: everything stays zeroed
    RmtMsgBufTy () {}
    /// Destructor makes sure the buffer is freed up
    ~RmtMsgBufTy() { RmtMsgBufTy::free(); }
    /// Free up the buffer, basically a reset
    void free ();
    /// If necessary allocate the required buffer, then initialize it to an empty message
    void init ();
    /// Add another element to the buffer, returns if successful (otherwise full!)
    bool add (const ElemTy& _elem);
    /// is empty, contains no payload?
    bool empty () const { return !elemCount; }
    /// send the message (if there is any), then reset the buffer
    void send (UDPMulticast& _mc);
    /// Perform add(), then if necessary send(); returns if sent
    bool add_send (const ElemTy& _elem, UDPMulticast& _mc);
};


//
// MARK: Global Functions
//

/// Initialize the module
void RemoteInit ();

/// Grace cleanup, stops all threads
void RemoteCleanup ();

/// Compares current vs. expected status and takes appropriate action
void RemoteSenderUpdateStatus ();


/// Informs us that updating a/c will start now, do some prep work
/// @note Internally acquires a lock, XPMP2::RemoteAcEnqueueDone() _must_ be called to release that lock
void RemoteAcEnqueueStarts (float now);

/// @brief Regularly called from the flight loop callback to send a/c date onto the network
/// @details Will collect data into network messages but only send out when full
void RemoteAcEnqueue (const Aircraft& ac);

/// Informs us that all a/c have been processed: All pending messages to be sent now
void RemoteAcEnqueueDone ();

/// Informs us about an aircraft deletion
void RemoteAcRemove (const Aircraft& ac);

/// Informs us that there are no more aircraft, clear our caches!
void RemoteAcClearAll ();

}
#endif
