/* Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation nor the names of its
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

#ifndef LOCATION_INTEGRATION_API_IMPL_H
#define LOCATION_INTEGRATION_API_IMPL_H

#include <mutex>

#include <LocIpc.h>
#include <LocationDataTypes.h>
#include <ILocationAPI.h>
#include <LocationIntegrationApi.h>
#include <MsgTask.h>
#include <LocationApiMsg.h>

#ifdef NO_UNORDERED_SET_OR_MAP
    #include <map>
#else
    #include <unordered_map>
#endif

using namespace std;
using namespace loc_util;
using namespace location_integration;

namespace location_integration
{
typedef std::unordered_map<LocConfigTypeEnum, int32_t> LocConfigReqCntMap;

typedef struct {
    bool     isValid;
    bool     enable;
    float    tuncThresholdMs; // need to be specified if enable is true
    uint32_t energyBudget;    // need to be specified if enable is true
} TuncConfigInfo;

typedef struct {
    bool isValid;
    bool enable;
} PaceConfigInfo;

typedef struct {
    bool             isValid;
    bool             resetToDeFault;
    GnssSvTypeConfig svTypeConfig;
    GnssSvIdConfig   svIdConfig;
} SVConfigInfo;

typedef struct {
    bool isValid;
    bool enable;
    bool enableForE911;
} RobustLocationConfigInfo;

typedef struct {
    bool isValid;
    bool userConsent;
} GtpUserConsentConfigInfo;

class IpcListener;

class LocationIntegrationApiImpl : public ILocationControlAPI {
    friend IpcListener;
public:
    LocationIntegrationApiImpl(LocIntegrationCbs& integrationCbs);

    void destroy();

    // convenient methods
    inline bool sendMessage(const uint8_t* data, uint32_t length) const {
        return (mIpcSender != nullptr) && LocIpc::send(*mIpcSender, data, length);
    }

    // config API
    virtual uint32_t resetConstellationConfig() override;
    virtual uint32_t configConstellations(const GnssSvTypeConfig& svTypeConfig,
                                          const GnssSvIdConfig&   svIdConfig) override;
    virtual uint32_t configConstrainedTimeUncertainty(
            bool enable, float tuncThreshold, uint32_t energyBudget) override;
    virtual uint32_t configPositionAssistedClockEstimator(bool enable) override;
    virtual uint32_t configLeverArm(const LeverArmConfigInfo& configInfo) override;
    virtual uint32_t configRobustLocation(bool enable, bool enableForE911) override;

    // rest of ILocationController API that are not used in integration API
    virtual uint32_t* gnssUpdateConfig(GnssConfig config) override;
    virtual uint32_t gnssDeleteAidingData(GnssAidingData& data) override;

    uint32_t getRobustLocationConfig();

    uint32_t setUserConsentForTerrestrialPositioning(bool userConsent);

private:
    ~LocationIntegrationApiImpl();
    bool integrationClientAllowed();
    void sendConfigMsgToHalDaemon(LocConfigTypeEnum configType,
                                  uint8_t* pMsg,
                                  size_t msgSize,
                                  bool invokeResponseCb = true);
    void sendClientRegMsgToHalDaemon();
    void processHalReadyMsg();

    void addConfigReq(LocConfigTypeEnum configType);
    void flushConfigReqs();
    void processConfigRespCb(const LocAPIGenericRespMsg* pRespMsg);
    void processGetRobustLocationConfigRespCb(
            const LocConfigGetRobustLocationConfigRespMsg* pRespMsg);

    // internal session parameter
    static mutex             mMutex;
    static bool              mClientRunning; // allow singleton int client
    bool                     mHalRegistered;
    // For client on different processor, socket name will start with
    // defined constant of SOCKET_TO_EXTERANL_AP_LOCATION_CLIENT_BASE.
    // For client on same processor, socket name will start with
    // SOCKET_LOC_CLIENT_DIR + LOC_INTAPI_NAME_PREFIX.
    char                     mSocketName[MAX_SOCKET_PATHNAME_LENGTH];

    // cached configuration to be used when hal daemon crashes
    // and restarts
    TuncConfigInfo           mTuncConfigInfo;
    PaceConfigInfo           mPaceConfigInfo;
    SVConfigInfo             mSVConfigInfo;
    LeverArmConfigInfo       mLeverArmConfigInfo;
    RobustLocationConfigInfo mRobustLocationConfigInfo;
    GtpUserConsentConfigInfo      mGtpUserConsentConfigInfo;

    LocConfigReqCntMap       mConfigReqCntMap;
    LocIntegrationCbs        mIntegrationCbs;

    LocIpc                   mIpc;
    shared_ptr<LocIpcSender> mIpcSender;

    MsgTask*                 mMsgTask;
};

} // namespace location_client

#endif /* LOCATION_INTEGRATION_API_IMPL_H */
