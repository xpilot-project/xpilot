/*
 * Original work Copyright (c) 2019, Birger Hoppe
 * Modified work Copyright (c) 2020, Justin Shannon
 *
 * Parts of this project have been copied from LTAPI and is copyrighted
 * by Birger Hoppe under the terms of the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef XPilotAPI_h
#define XPilotAPI_h

#include <cstring>
#include <memory>
#include <string>
#include <list>
#include <map>
#include <chrono>

#include "XPLMDataAccess.h"

class XPilotDataRef;

class XPilotAPIAircraft
{
private:
    unsigned keyNum = 0;
    std::string key;

public:
    struct XPilotAPIBulkData {
    public:
        uint64_t keyNum             = 0;    // modeS identifier (xPilot assigns a unique and sequential hexadecimal number)
        double lat                  = 0.0f; // latitude
        double lon                  = 0.0f; // longitude
        double alt_ft               = 0.0f; // altitude (ft)
        float heading               = 0.0f; // true heading
        float roll                  = 0.0f; // roll/bank (positive = right)
        float pitch                 = 0.0f; // pitch (positive = up)
        float speed_kt              = 0.0f; // ground speed (knots)
        float terrainAlt_ft         = 0.0f; // terrain altitude beneath plane (ft)
        float flaps                 = 0.0f; // flap position: 0.0 retracted, 1.0 fully extended
        float gear                  = 0.0;  // gear position: 0.0 retracted, 1.0 fully extended
        float bearing               = 0.0f; // degrees from the camera position
        float dist_nm               = 0.0f; // distance to the camera

        struct BulkBitsTy {
            bool onGnd          : 1; // is the plane on the ground?
            bool taxi           : 1; // taxi lights
            bool land           : 1; // landing lights
            bool bcn            : 1; // beacon lights
            bool strb           : 1; // strobe lights
            bool nav            : 1; // navigation lights
            unsigned filler1    : 2; // unused, fills up the byte alignment
            int multiIdx        : 8; // multiplayer index if plane is reported via sim/multiplayer/position datarefs, 0 if not
            unsigned filler2    : 8;
            unsigned filler3    : 32;
        } bits;

        XPilotAPIBulkData() { memset(&bits, 0, sizeof(bits)); }
    };

    // Bulk text transfer structure
    struct XPilotAPIBulkInfoTexts {
    public:
        uint64_t keyNum;
        // aircraft model/operator
        char            modelIcao[8];       // ICAO aircraft type like "A321"
        char            acClass[4];         // Aircraft class like "L2J"
        char            wtc[4];             // Wake turbulence category like H,M,L/M,L
        // flight data
        char            callSign[8];        // Callsign like "SWA916"
        char            squawk[8];          // squawk code (as text) like "1000"
        char            origin[8];          // Origin airport ICAO like "KLAX"
        char            destination[8];     // Destination airport ICAO like "KJFK"
        char            cslModel[24];       // Name of CSL model used for actual rendering of the plane

        XPilotAPIBulkInfoTexts() { memset(this, 0, sizeof(*this)); }
    };

    struct XPilotLights {
        bool beacon         : 1; // beacon lights
        bool strobe         : 1; // strobe lights
        bool nav            : 1; // navigation lights
        bool landing        : 1; // landing lights
        bool taxi           : 1; // taxi lights

        XPilotLights(const XPilotAPIBulkData::BulkBitsTy b) : beacon(b.bcn), strobe(b.strb), nav(b.nav), landing(b.land), taxi(b.taxi) {}
    };

protected:
    XPilotAPIBulkData bulk;         // numerical plane data
    XPilotAPIBulkInfoTexts info;    // textual plane data
    bool bUpdated = false;          // update helper, gets reset before updates, set during updates, stays false if not updated

public:
    XPilotAPIAircraft();
    virtual ~XPilotAPIAircraft();

    // Updates the aircraft with fresh numerical values, called from XPilotAPIConnect::UpdateAcList()
    virtual bool updateAircraft(const XPilotAPIBulkData& __bulk, size_t __inSize);
    // Updates the aircraft with fresh textual information, called from XPilotAPIConnect::UpdateAcList()
    virtual bool updateAircraft(const XPilotAPIBulkInfoTexts& __info, size_t __inSize);

    bool isUpdated()const { return bUpdated; }
    void resetUpdated() { bUpdated = false; }

public:
    std::string getKey()            const { return key; }
    std::string getCallSign()       const { return info.callSign; }
    std::string getAcClass()        const { return info.acClass; }
    std::string getWtc()            const { return info.wtc; }
    std::string getModelIcao()      const { return info.modelIcao; }
    std::string getCslModel()       const { return info.cslModel; }
    std::string getSquawk()         const { return info.squawk; }
    std::string getOrigin()         const { return info.origin; }
    std::string getDestination()    const { return info.destination; }
    std::string getDescription()    const;

    // position, altitude
    double getLat()             const { return bulk.lat; }
    double getLon()             const { return bulk.lon; }
    float getHeading()          const { return bulk.heading; }
    double getAltFt()           const { return bulk.alt_ft; }
    float getTerrainFt()        const { return bulk.terrainAlt_ft; }
    float getRoll()             const { return bulk.roll; }
    float getPitch()            const { return bulk.pitch; }
    float getSpeedKn()          const { return bulk.speed_kt; }
    bool isOnGround()           const { return bulk.bits.onGnd; }
    float getFlaps()            const { return bulk.flaps; }
    float getGear()             const { return bulk.gear; }
    XPilotLights getLights()    const { return bulk.bits; }
    float getBearing()          const { return bulk.bearing; }
    float getDistNm()           const { return bulk.dist_nm; }
    int getMultiIdx()           const { return bulk.bits.multiIdx; }

public:
    static XPilotAPIAircraft* CreateNewObject() { return new XPilotAPIAircraft(); }
};

// Smart pointer to an XPilotAPIAircraft object
typedef std::shared_ptr<XPilotAPIAircraft> SPtrXPilotAPIAircraft;

// Map of all aircrafts stored as smart pointers to XPilotAPIAircraft objects
typedef std::map<std::string, SPtrXPilotAPIAircraft> MapXPilotAPIAircraft;

// Simple list of smart pointers to XPilotAPIAircraft objects
typedef std::list<SPtrXPilotAPIAircraft> ListXPilotAPIAircraft;

class XPilotAPIConnect
{
public:
    // Callback function type passed in to XPilotAPIConnect()
    // @return New XPilotAPIAircraft object or derived class' object
    //
    // The callback is actually called by UpdateAcList().
    //
    // If you use a class derived from XPilotAPIAircraft, then you
    // pass in a pointer to a callback function, which returns new empty
    // objects of _your_ derived class whenever UpdateAcList() needs to create
    // a new aircraft object.
    typedef XPilotAPIAircraft* fCreateAcObject();

    // Number of seconds between two calls of the expensive type, which
    // fetches texts from xPilot
    std::chrono::seconds sPeriodExpsv = std::chrono::seconds(3);

protected:
    //  Number of aircraft to fetch in one bulk operation
    const int iBulkAc = 50;

    // bulk data array for communication with xPilot
    std::unique_ptr<XPilotAPIAircraft::XPilotAPIBulkData[]> vBulkNum;

    // bulk info text array for communication with xPilot
    std::unique_ptr<XPilotAPIAircraft::XPilotAPIBulkInfoTexts[]> vInfoTexts;

protected:
    // Pointer to callback function returning new aircraft objects
    fCreateAcObject* pfCreateAcObject = nullptr;
    // The map of aircrafts
    MapXPilotAPIAircraft mapAc;
    // Last fetching of expensive data
    std::chrono::time_point<std::chrono::steady_clock> lastExpsvFetch;

public:
    XPilotAPIConnect(fCreateAcObject* _pfCreateAcObject = XPilotAPIAircraft::CreateNewObject, int numBulkAc = 50);
    virtual ~XPilotAPIConnect();
    // Is xPilot available? (checks via XPLMFindPluginBySignature)
    static bool isXPilotAvail();
    // How many aircraft is xPilot displaying right now?
    static int getXPilotNumAc();
    // Does xPilot have control of AI planes?
    static bool doesXPilotControlAI();
    // Updates map of aircrafts and returns reference to them
    const MapXPilotAPIAircraft& UpdateAcList(ListXPilotAPIAircraft* plistRemovedAc = nullptr);
    // Returns the map of aircraft
    const MapXPilotAPIAircraft& getAcMap() const { return mapAc; }
    // Find an aircraft for a given multiplayer slot
    SPtrXPilotAPIAircraft getAcByMultIdx(int multiIdx) const;

protected:
    template <class T>
    bool DoBulkFetch(int numAc, XPilotDataRef& DR, int& outSize, 
        std::unique_ptr<T[]>& vBulk);
};

class XPilotDataRef {
protected:
    std::string     sDataRef;           // dataRef name, passed in via constructor
    XPLMDataRef     dataRef = NULL;     // dataRef identifier returned by X-Plane
    XPLMDataTypeID  dataTypes = xplmType_Unknown;   // supported data types
    bool            bValid = true;      // does this object have a valid binding to a dataRef already?
public:
    XPilotDataRef(std::string _sDataRef);  // Constructor, set the dataRef's name
    inline bool needsInit() const { return bValid && !dataRef; }
    // @brief Found the dataRef _and_ it contains formats we can work with?
    bool    isValid();
    // Finds the dataRef (and would try again and again, no matter what bValid says)
    bool    FindDataRef();

    // Get types supported by the dataRef
    XPLMDataTypeID getDataRefTypes() const { return dataTypes; }
    // Is `int` a supported dataRef type?
    bool    hasInt()   const { return dataTypes & xplmType_Int; }
    // Is `float` a supported dataRef type?
    bool    hasFloat() const { return dataTypes & xplmType_Float; }
    // Defines which types to work with to become `valid`
    static constexpr XPLMDataTypeID usefulTypes =
        xplmType_Int | xplmType_Float | xplmType_Data;

    // @brief Get dataRef's integer value.
    // Silently returns 0 if dataRef doesn't exist.
    int     getInt();
    // @brief Get dataRef's float value.
    // Silently returns 0.0f if dataRef doesn't exist.
    float   getFloat();
    // Gets dataRef's integer value and returns if it is not zero
    inline bool getBool() { return getInt() != 0; }
    // Gets dataRef's binary data
    int     getData(void* pOut, int inOffset, int inMaxBytes);

    // Writes an integer value to the dataRef
    void    set(int i);
    // Writes a float vlue to the dataRef
    void    set(float f);
};

#endif // !XPilotAPI_h
