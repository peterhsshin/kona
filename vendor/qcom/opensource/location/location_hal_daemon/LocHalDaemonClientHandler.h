/* Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LOCHAL_CLIENT_HANDLER_H
#define LOCHAL_CLIENT_HANDLER_H

#include <queue>
#include <mutex>
#include <log_util.h>
#include <loc_pla.h>

#ifdef NO_UNORDERED_SET_OR_MAP
    #include <map>
#else
    #include <unordered_map>
#endif

#include <LocationAPI.h>
#include <LocIpc.h>

using namespace loc_util;

// forward declaration
class LocationApiService;

/******************************************************************************
LocHalDaemonClientHandler
******************************************************************************/
class LocHalDaemonClientHandler
{
public:
    inline LocHalDaemonClientHandler(LocationApiService* service, const std::string& clientname,
                                     ClientType clientType) :
                mService(service),
                mName(clientname),
                mClientType(clientType),
                mCapabilityMask(0),
                mTracking(false),
                mBatching(false),
                mSessionId(0),
                mBatchingId(0),
                mBatchingMode(BATCHING_MODE_NO_AUTO_REPORT),
                mLocationApi(nullptr),
                mCallbacks{},
                mPendingMessages(),
                mGfPendingMessages(),
                mSubscriptionMask(0),
                mEngineInfoRequestMask(0),
                mGeofenceIds(nullptr),
                mIpcSender(createSender(clientname.c_str())) {


        if (mClientType == LOCATION_CLIENT_API) {
            updateSubscription(E_LOC_CB_GNSS_LOCATION_INFO_BIT);
            mLocationApi = LocationAPI::createInstance(mCallbacks);
        }
        updateSubscription(0);
    }

    static shared_ptr<LocIpcSender> createSender(const string socket);
    void cleanup();

    // public APIs
    void updateSubscription(uint32_t mask);
    // when client stops the location session, then all callbacks
    // related to location session need to be unsubscribed
    void unsubscribeLocationSessionCb();
    uint32_t startTracking();
    uint32_t startTracking(LocationOptions & locOptions);
    void stopTracking();
    void updateTrackingOptions(LocationOptions & locOptions);
    void onGnssEnergyConsumedInfoAvailable(LocAPIGnssEnergyConsumedIndMsg &msg);
    void onControlResponseCb(LocationError err, ELocMsgID msgId);
    void onGnssConfigCb(ELocMsgID configMsgId, const GnssConfig & gnssConfig);
    bool hasPendingEngineInfoRequest(uint32_t mask);
    void addEngineInfoRequst(uint32_t mask);

    uint32_t startBatching(uint32_t minInterval, uint32_t minDistance, BatchingMode batchMode);
    void stopBatching();
    void updateBatchingOptions(uint32_t minInterval, uint32_t minDistance, BatchingMode batchMode);

    uint32_t* addGeofences(size_t count, GeofenceOption*, GeofenceInfo*);
    void removeGeofences(size_t count, uint32_t* ids);
    void modifyGeofences(size_t count, uint32_t* ids, GeofenceOption* options);
    void pauseGeofences(size_t count, uint32_t* ids);
    void resumeGeofences(size_t count, uint32_t* ids);

    //other API
    void setGeofenceIds(size_t count, uint32_t* clientIds, uint32_t* sessionIds);
    void eraseGeofenceIds(size_t count, uint32_t* clientIds);
    uint32_t* getSessionIds(size_t count, uint32_t* clientIds);
    uint32_t* getClientIds(size_t count, uint32_t* sessionIds);
    // send terrestrial fix to the requesting LCA client
    void sendTerrestrialFix(LocationError error, const Location& location);

    void pingTest();

    bool mTracking;
    bool mBatching;
    BatchingMode mBatchingMode;
    std::queue<ELocMsgID> mPendingMessages;
    std::queue<ELocMsgID> mGfPendingMessages;

private:
    inline ~LocHalDaemonClientHandler() {}

    // Location API callback functions
    void onCapabilitiesCallback(LocationCapabilitiesMask capabilitiesMask);
    void onResponseCb(LocationError err, uint32_t id);
    void onCollectiveResponseCallback(size_t count, LocationError *errs, uint32_t *ids);

    void onTrackingCb(Location location);
    void onBatchingCb(size_t count, Location* location, BatchingOptions batchOptions);
    void onBatchingStatusCb(BatchingStatusInfo batchingStatus,
            std::list<uint32_t>& listOfCompletedTrips);
    void onGnssLocationInfoCb(GnssLocationInfoNotification gnssLocationInfoNotification);
    void onGeofenceBreachCb(GeofenceBreachNotification geofenceBreachNotification);
    void onEngLocationsInfoCb(uint32_t count,
                              GnssLocationInfoNotification* engLocationsInfoNotification);
    void onGnssNiCb(uint32_t id, GnssNiNotification gnssNiNotification);
    void onGnssSvCb(GnssSvNotification gnssSvNotification);
    void onGnssNmeaCb(GnssNmeaNotification);
    void onGnssDataCb(GnssDataNotification gnssDataNotification);
    void onGnssMeasurementsCb(GnssMeasurementsNotification gnssMeasurementsNotification);
    void onLocationSystemInfoCb(LocationSystemInfo);
    void onLocationApiDestroyCompleteCb();

    // send ipc message to this client for general use
    template <typename MESSAGE>
    bool sendMessage(const MESSAGE& msg) {
        return sendMessage(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
    }

    // send ipc message to this client for serialized payload
    bool sendMessage(const uint8_t* pmsg, size_t msglen) {
        return LocIpc::send(*mIpcSender, pmsg, msglen);
    }

    uint32_t getSupportedTbf (uint32_t tbfMsec);

    // pointer to parent service
    LocationApiService* mService;

    // name of this client
    const std::string mName;
    ClientType mClientType;

    // LocationAPI interface
    LocationCapabilitiesMask mCapabilityMask;
    uint32_t mSessionId;
    uint32_t mBatchingId;
    LocationAPI* mLocationApi;
    LocationCallbacks mCallbacks;
    TrackingOptions mOptions;
    BatchingOptions mBatchOptions;

    // bitmask to hold this client's subscription
    uint32_t mSubscriptionMask;
    // bitmask to hold this client's request to engine info related subscription
    uint32_t mEngineInfoRequestMask;

    uint32_t* mGeofenceIds;
    shared_ptr<LocIpcSender> mIpcSender;
    std::unordered_map<uint32_t, uint32_t> mGfIdsMap; //geofence ID map, clientId-->session
};

#endif //LOCHAL_CLIENT_HANDLER_H

