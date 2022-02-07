/*
* Copyright (c) 2017-2018, 2020 The Linux Foundation. All rights reserved.
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

#define __CLASS__ "HWColorManagerDRM"

#include <array>
#include <map>
#include <cstring>
#include <vector>

#ifdef PP_DRM_ENABLE
#include <drm/msm_drm_pp.h>
#endif
#include <utils/debug.h>
#include "hw_color_manager_drm.h"

#ifdef PP_DRM_ENABLE
static const uint32_t kPgcDataMask = 0x3FF;
static const uint32_t kPgcShift = 16;

static const uint32_t kIgcDataMask = 0xFFF;
static const uint32_t kIgcShift = 16;

#ifdef DRM_MSM_PA_HSIC
static const uint32_t kPAHueMask = (1 << 12);
static const uint32_t kPASatMask = (1 << 13);
static const uint32_t kPAValMask = (1 << 14);
static const uint32_t kPAContrastMask = (1 << 15);
#endif

#ifdef DRM_MSM_SIXZONE
static const uint32_t kSixZoneP0Mask = 0x0FFF;
static const uint32_t kSixZoneP1Mask = 0x0FFF0FFF;
static const uint32_t kSixZoneHueMask = (1 << 16);
static const uint32_t kSixZoneSatMask = (1 << 17);
static const uint32_t kSixZoneValMask = (1 << 18);
#endif

#ifdef DRM_MSM_MEMCOL
static const uint32_t kMemColorProtHueMask = (1 << 0);
static const uint32_t kMemColorProtSatMask = (1 << 1);
static const uint32_t kMemColorProtValMask = (1 << 2);
static const uint32_t kMemColorProtContMask = (1 << 3);
static const uint32_t kMemColorProtSixZoneMask = (1 << 4);
static const uint32_t kMemColorProtBlendMask = (1 << 5);

static const uint32_t kMemColorProtMask = \
  (kMemColorProtHueMask | kMemColorProtSatMask | kMemColorProtValMask | \
    kMemColorProtContMask | kMemColorProtSixZoneMask | kMemColorProtBlendMask);

static const uint32_t kMemColorSkinMask = (1 << 19);
static const uint32_t kMemColorSkyMask = (1 << 20);
static const uint32_t kMemColorFolMask = (1 << 21);

static const uint32_t kSourceFeatureV5 = 5;
#endif
#endif

namespace sdm {

typedef std::map<uint32_t, std::vector<DRMPPFeatureID>> DrmPPFeatureMap;

static const DrmPPFeatureMap g_dspp_map = {
  {kGlobalColorFeaturePcc,      {kFeaturePcc}},
  {kGlobalColorFeatureIgc,      {kFeatureIgc}},
  {kGlobalColorFeaturePgc,      {kFeaturePgc}},
  {kMixerColorFeatureGc,        {kFeatureMixerGc}},
  {kGlobalColorFeaturePaV2,     {kFeaturePAHsic,
                                 kFeaturePASixZone,
                                 kFeaturePAMemColSkin,
                                 kFeaturePAMemColSky,
                                 kFeaturePAMemColFoliage,
                                 kFeaturePAMemColProt}},
  {kGlobalColorFeatureDither,   {kFeatureDither}},
  {kGlobalColorFeatureGamut,    {kFeatureGamut}},
  {kGlobalColorFeaturePADither, {kFeaturePADither}},
};

static const DrmPPFeatureMap g_vig_map = {
  {kSourceColorFeatureIgc,      {kFeatureVigIgc}},
  {kSourceColorFeatureGamut,    {kFeatureVigGamut}},
};

static const DrmPPFeatureMap g_dgm_map = {
  {kSourceColorFeatureIgc,      {kFeatureDgmIgc}},
  {kSourceColorFeatureGc,       {kFeatureDgmGc}},
};

static const std::array<const DrmPPFeatureMap *,
                        static_cast<int>(kPPBlockMax)> feature_map_list = {
  {&g_dspp_map, &g_vig_map, &g_dgm_map}
};

DisplayError (*HWColorManagerDrm::pp_features_[])(const PPFeatureInfo &,
                                                    DRMPPFeatureInfo *) = {
  [kFeaturePcc] = &HWColorManagerDrm::GetDrmPCC,
  [kFeatureIgc] = &HWColorManagerDrm::GetDrmIGC,
  [kFeaturePgc] = &HWColorManagerDrm::GetDrmPGC,
  [kFeatureMixerGc] = &HWColorManagerDrm::GetDrmMixerGC,
  [kFeaturePaV2] = NULL,
  [kFeatureDither] = &HWColorManagerDrm::GetDrmDither,
  [kFeatureGamut] = &HWColorManagerDrm::GetDrmGamut,
  [kFeaturePADither] = &HWColorManagerDrm::GetDrmPADither,
  [kFeaturePAHsic] = &HWColorManagerDrm::GetDrmPAHsic,
  [kFeaturePASixZone] = &HWColorManagerDrm::GetDrmPASixZone,
  [kFeaturePAMemColSkin] = &HWColorManagerDrm::GetDrmPAMemColSkin,
  [kFeaturePAMemColSky] = &HWColorManagerDrm::GetDrmPAMemColSky,
  [kFeaturePAMemColFoliage] = &HWColorManagerDrm::GetDrmPAMemColFoliage,
  [kFeaturePAMemColProt] = &HWColorManagerDrm::GetDrmPAMemColProt,
  [kFeatureDgmIgc] = &HWColorManagerDrm::GetDrmIGC,
  [kFeatureDgmGc] = &HWColorManagerDrm::GetDrmPGC,
  [kFeatureVigIgc] = &HWColorManagerDrm::GetDrmIGC,
  [kFeatureVigGamut] = &HWColorManagerDrm::GetDrmGamut,
};

uint32_t HWColorManagerDrm::GetFeatureVersion(const DRMPPFeatureInfo &feature) {
  uint32_t version = PPFeatureVersion::kSDEPpVersionInvalid;

  switch (feature.id) {
    case kFeaturePcc:
      if (feature.version == 1) {
        version = PPFeatureVersion::kSDEPccV17;
      } else if (feature.version == 4) {
        version = PPFeatureVersion::kSDEPccV4;
      }
      break;
    case kFeatureIgc:
      if (feature.version == 3)
        version = PPFeatureVersion::kSDEIgcV30;
      break;
    case kFeaturePgc:
      if (feature.version == 1)
        version = PPFeatureVersion::kSDEPgcV17;
      break;
    case kFeatureMixerGc:
        version = PPFeatureVersion::kSDEPgcV17;
      break;
    case kFeaturePAHsic:
    case kFeaturePASixZone:
    case kFeaturePAMemColSkin:
    case kFeaturePAMemColSky:
    case kFeaturePAMemColFoliage:
    case kFeaturePAMemColProt:
      if (feature.version == 1)
        version = PPFeatureVersion::kSDEPaV17;
      break;
    case kFeatureDither:
        version = PPFeatureVersion::kSDEDitherV17;
      break;
    case kFeatureGamut:
      if (feature.version == 1)
        version = PPFeatureVersion::kSDEGamutV17;
      else if (feature.version == 4)
        version = PPFeatureVersion::kSDEGamutV4;
      break;
    case kFeaturePADither:
        version = PPFeatureVersion::kSDEPADitherV17;
      break;
    default:
      break;
  }
  return version;
}

DisplayError HWColorManagerDrm::ToDrmFeatureId(PPBlock block, uint32_t id,
  std::vector<DRMPPFeatureID> *drm_id) {
  if (block < kDSPP || block >= kPPBlockMax) {
    DLOGE("Invalid input parameter, block = %d", (int) block);
    return kErrorParameters;
  }

  if (!drm_id) {
    DLOGE("Invalid output parameter, block = %d", (int) block);
    return kErrorParameters;
  }

  auto map = feature_map_list[block];
  auto drm_features = map->find(id);
  if (drm_features == map->end()) {
    return kErrorParameters;
  }
  for (DRMPPFeatureID fid : drm_features->second)
    drm_id->push_back(fid);

  return kErrorNone;
}

DisplayError HWColorManagerDrm::GetDrmFeature(PPFeatureInfo *in_data,
                                              DRMPPFeatureInfo *out_data,
                                              bool force_disable) {
  DisplayError ret = kErrorParameters;
  uint32_t flags = 0;

  if (!in_data || !out_data) {
    DLOGE("Invalid input parameter, in_data or out_data is NULL");
    return ret;
  }

  /* Cache and override the enable_flags_ if force_disable is requested */
  if (force_disable) {
    flags = in_data->enable_flags_;
    in_data->enable_flags_ = kOpsDisable;
  }

  if (pp_features_[out_data->id])
    ret = pp_features_[out_data->id](*in_data, out_data);


  /* Restore the original enable_flags_ */
  if (force_disable) {
    in_data->enable_flags_ = flags;
  }

  return ret;
}

void HWColorManagerDrm::FreeDrmFeatureData(DRMPPFeatureInfo *feature) {
  if (feature && feature->payload) {
#ifdef PP_DRM_ENABLE
    void *ptr = feature->payload;
#endif

    switch (feature->id) {
      case kFeaturePcc: {
#ifdef PP_DRM_ENABLE
        drm_msm_pcc *pcc = reinterpret_cast<drm_msm_pcc *>(ptr);
        delete[] pcc;
#endif
        break;
      }
      case kFeatureIgc:
      case kFeatureDgmIgc:
      case kFeatureVigIgc: {
#ifdef PP_DRM_ENABLE
        drm_msm_igc_lut *igc = reinterpret_cast<drm_msm_igc_lut *>(ptr);
        delete[] igc;
#endif
        break;
      }
      case kFeaturePgc:
      case kFeatureDgmGc: {
#ifdef PP_DRM_ENABLE
        drm_msm_pgc_lut *pgc = reinterpret_cast<drm_msm_pgc_lut *>(ptr);
        delete[] pgc;
#endif
        break;
      }
      case kFeatureDither: {
#ifdef PP_DRM_ENABLE
        drm_msm_dither *dither = reinterpret_cast<drm_msm_dither *>(ptr);
        delete dither;
#endif
        break;
      }
      case kFeatureGamut:
      case kFeatureVigGamut: {
#ifdef PP_DRM_ENABLE
        drm_msm_3d_gamut *gamut = reinterpret_cast<drm_msm_3d_gamut *>(ptr);
        delete[] gamut;
#endif
        break;
      }
      case kFeaturePADither: {
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_PA_DITHER)
        drm_msm_pa_dither *pa_dither = reinterpret_cast<drm_msm_pa_dither *>(ptr);
        delete pa_dither;
#endif
        break;
      }
      case kFeaturePAHsic: {
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_PA_HSIC)
        drm_msm_pa_hsic *hsic = reinterpret_cast<drm_msm_pa_hsic *>(ptr);
        delete[] hsic;
#endif
        break;
      }
      case kFeaturePASixZone: {
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_SIXZONE)
        drm_msm_sixzone *sixzone = reinterpret_cast<drm_msm_sixzone *>(ptr);
        delete[] sixzone;
#endif
        break;
      }
      case kFeaturePAMemColSkin:
      case kFeaturePAMemColSky:
      case kFeaturePAMemColFoliage:
      case kFeaturePAMemColProt: {
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
        drm_msm_memcol *memcol = reinterpret_cast<drm_msm_memcol *>(ptr);
        delete[] memcol;
#endif
        break;
      }
      case kFeatureMixerGc:
      case kFeaturePaV2:
      default: {
        DLOGE("Invalid feature: %d\n", feature->id);
        return;
      }
    }
    feature->payload = nullptr;
  }
}

DisplayError HWColorManagerDrm::GetDrmPCC(const PPFeatureInfo &in_data,
                                          DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  struct SDEPccV4Cfg *sde_pcc = NULL;
  struct SDEPccV4Coeff *sde_pcc_coeffs = NULL;
  struct drm_msm_pcc *mdp_pcc = NULL;
  struct drm_msm_pcc_coeff *mdp_pcc_coeffs = NULL;
  uint32_t i = 0;

  switch (in_data.feature_version_) {
  case PPFeatureVersion::kSDEPccV4:
    sde_pcc = (struct SDEPccV4Cfg *) in_data.GetConfigData();
    break;
  default:
    DLOGE("Unsupported pcc feature version: %d", in_data.feature_version_);
    return kErrorParameters;
  }

  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = num_configs * sizeof(struct drm_msm_pcc);

  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_pcc = new struct drm_msm_pcc[num_configs];
  if (!mdp_pcc) {
    DLOGE("Failed to allocate memory for pcc");
    return kErrorMemory;
  }

  struct drm_msm_pcc *curr_pcc = mdp_pcc;
  struct SDEPccV4Cfg *input_pcc = sde_pcc;
  for (uint32_t num = 0; num < num_configs; num++) {
    if (num_configs > 1 && (!num)) { // set config number to the first pcc data
      curr_pcc->flags |= ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
    } else {
      curr_pcc->flags |= 0;
    }

    for (i = 0; i < kMaxPCCChanel; i++) {
      switch (i) {
      case 0:
        sde_pcc_coeffs = &input_pcc->red;
        mdp_pcc_coeffs = &curr_pcc->r;
        curr_pcc->r_rr = sde_pcc_coeffs->rr;
        curr_pcc->r_gg = sde_pcc_coeffs->gg;
        curr_pcc->r_bb = sde_pcc_coeffs->bb;
        break;
      case 1:
        sde_pcc_coeffs = &input_pcc->green;
        mdp_pcc_coeffs = &curr_pcc->g;
        curr_pcc->g_rr = sde_pcc_coeffs->rr;
        curr_pcc->g_gg = sde_pcc_coeffs->gg;
        curr_pcc->g_bb = sde_pcc_coeffs->bb;
        break;
      case 2:
        sde_pcc_coeffs = &input_pcc->blue;
        mdp_pcc_coeffs = &curr_pcc->b;
        curr_pcc->b_rr = sde_pcc_coeffs->rr;
        curr_pcc->b_gg = sde_pcc_coeffs->gg;
        curr_pcc->b_bb = sde_pcc_coeffs->bb;
        break;
      }
      mdp_pcc_coeffs->c = sde_pcc_coeffs->c;
      mdp_pcc_coeffs->r = sde_pcc_coeffs->r;
      mdp_pcc_coeffs->g = sde_pcc_coeffs->g;
      mdp_pcc_coeffs->b = sde_pcc_coeffs->b;
      mdp_pcc_coeffs->rg = sde_pcc_coeffs->rg;
      mdp_pcc_coeffs->gb = sde_pcc_coeffs->gb;
      mdp_pcc_coeffs->rb = sde_pcc_coeffs->rb;
      mdp_pcc_coeffs->rgb = sde_pcc_coeffs->rgb;
    }
    curr_pcc++;
    input_pcc++;
  }
  out_data->payload = mdp_pcc;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmIGC(const PPFeatureInfo &in_data,
                                          DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  SDEIgcV30LUTWrapper *sde_igc_wrapper = NULL;
  struct drm_msm_igc_lut *mdp_igc;
  uint32_t *c0_c1_data_ptr = NULL;
  uint32_t *c2_data_ptr = NULL;

  switch (in_data.feature_version_) {
  case PPFeatureVersion::kSDEIgcV30:
  case kSourceFeatureV5:
    sde_igc_wrapper = (SDEIgcV30LUTWrapper *)in_data.GetConfigData();
    break;
  default:
    DLOGE("Unsupported igc feature version: %d", in_data.feature_version_);
    return kErrorParameters;
  }

  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = num_configs * sizeof(struct drm_msm_igc_lut);

  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_igc = new drm_msm_igc_lut[num_configs];
  if (!mdp_igc) {
    DLOGE("Failed to allocate memory for igc");
    return kErrorMemory;
  }

  struct drm_msm_igc_lut *curr_igc = mdp_igc;
  struct SDEIgcV30LUTData *input_igc = (struct SDEIgcV30LUTData *) sde_igc_wrapper;
  for (uint32_t num = 0; num < num_configs; num++) {
    curr_igc->flags = IGC_DITHER_ENABLE;
    curr_igc->strength = input_igc->strength;
    if (num_configs > 1 && (!num)) {
      curr_igc->flags |= ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
    } else {
      curr_igc->flags |= 0;
    }

    c0_c1_data_ptr = reinterpret_cast<uint32_t*>(input_igc->c0_c1_data);
    c2_data_ptr = reinterpret_cast<uint32_t*>(input_igc->c2_data);

    if (!c0_c1_data_ptr || !c2_data_ptr) {
      DLOGE("Invaid igc data pointer");
      delete mdp_igc;
      curr_igc = NULL;
      input_igc = NULL;
      out_data->payload = NULL;
      return kErrorParameters;
    }

    for (int i = 0; i < IGC_TBL_LEN; i++) {
      curr_igc->c0[i] = c0_c1_data_ptr[i] & kIgcDataMask;
      curr_igc->c1[i] = (c0_c1_data_ptr[i] >> kIgcShift) & kIgcDataMask;
      curr_igc->c2[i] = c2_data_ptr[i] & kIgcDataMask;
    }
    curr_igc++;
    sde_igc_wrapper++;
    input_igc = (struct SDEIgcV30LUTData *) sde_igc_wrapper;
  }
  out_data->payload = mdp_igc;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPGC(const PPFeatureInfo &in_data,
                                          DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  SDEPgcLUTWrapper *sde_pgc_wrapper = NULL;
  struct drm_msm_pgc_lut *mdp_pgc;

  sde_pgc_wrapper = (SDEPgcLUTWrapper *)in_data.GetConfigData();
  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = num_configs * sizeof(struct drm_msm_pgc_lut);

  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_pgc = new drm_msm_pgc_lut[num_configs];
  if (!mdp_pgc) {
    DLOGE("Failed to allocate memory for pgc");
    return kErrorMemory;
  }

  struct drm_msm_pgc_lut *curr_pgc = mdp_pgc;
  struct SDEPgcLUTData *input_pgc = (struct SDEPgcLUTData *)sde_pgc_wrapper;;
  for (uint32_t num = 0; num < num_configs; num++) {
    if (num_configs > 1 && (!num)) {
      curr_pgc->flags |= ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
    } else {
      curr_pgc->flags |= 0;
    }

    for (int i = 0, j = 0; i < PGC_TBL_LEN; i++, j += 2) {
      curr_pgc->c0[i] = (input_pgc->c0_data[j] & kPgcDataMask) |
        (input_pgc->c0_data[j + 1] & kPgcDataMask) << kPgcShift;
      curr_pgc->c1[i] = (input_pgc->c1_data[j] & kPgcDataMask) |
        (input_pgc->c1_data[j + 1] & kPgcDataMask) << kPgcShift;
      curr_pgc->c2[i] = (input_pgc->c2_data[j] & kPgcDataMask) |
        (input_pgc->c2_data[j + 1] & kPgcDataMask) << kPgcShift;
    }
    curr_pgc++;
    sde_pgc_wrapper++;
    input_pgc = (struct SDEPgcLUTData *)sde_pgc_wrapper;
  }
  out_data->payload = mdp_pgc;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmMixerGC(const PPFeatureInfo &in_data,
                                              DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  out_data->id = kPPFeaturesMax;
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAHsic(const PPFeatureInfo &in_data,
                                           DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_PA_HSIC)
  struct SDEPaData *sde_pa;
  SDEPaCfgWrapper *sde_pa_wrapper = NULL;
  struct drm_msm_pa_hsic *mdp_hsic;
  sde_pa_wrapper = (SDEPaCfgWrapper *)in_data.GetConfigData();
  sde_pa = (struct SDEPaData *) sde_pa_wrapper;
  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for pa hsic");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & (kPAHueMask | kPASatMask |
                        kPAValMask | kPAContrastMask))) {
    /* PA HSIC feature disable case, but other PA features active */
    return ret;
  }

  mdp_hsic = new drm_msm_pa_hsic[num_configs];
  if (!mdp_hsic) {
    DLOGE("Failed to allocate memory for pa hsic");
    return kErrorMemory;
  }

  struct drm_msm_pa_hsic *curr_hsic = mdp_hsic;
  struct SDEPaData *input_pa = sde_pa;

  for (uint32_t num = 0; num < num_configs; num++) {
    curr_hsic->flags = 0;
    if (in_data.enable_flags_ & kPaHueEnable) {
      curr_hsic->flags |= PA_HSIC_HUE_ENABLE;
      curr_hsic->hue = input_pa->hue_adj;
    }
    if (in_data.enable_flags_ & kPaSatEnable) {
      curr_hsic->flags |= PA_HSIC_SAT_ENABLE;
      curr_hsic->saturation = input_pa->sat_adj;
    }
    if (in_data.enable_flags_ & kPaValEnable) {
      curr_hsic->flags |= PA_HSIC_VAL_ENABLE;
      curr_hsic->value = input_pa->val_adj;
    }
    if (in_data.enable_flags_ & kPaContEnable) {
      curr_hsic->flags |= PA_HSIC_CONT_ENABLE;
      curr_hsic->contrast = input_pa->cont_adj;
    }
    if (num_configs > 1 && (!num)) {
      curr_hsic->flags |= ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
    } else {
      curr_hsic->flags |= ((uint64_t)0 << 60);
    }
    curr_hsic++;
    sde_pa_wrapper++;
    input_pa = (struct SDEPaData *) sde_pa_wrapper;
  }
  if (mdp_hsic->flags) {
    out_data->payload = mdp_hsic;
    out_data->payload_size = num_configs * sizeof(struct drm_msm_pa_hsic);
  } else {
    /* PA HSIC configuration unchanged, no better return code available */
    delete mdp_hsic;
    ret = kErrorPermission;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPASixZone(const PPFeatureInfo &in_data,
                                                DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_SIXZONE)
  struct SDEPaData *sde_pa;
  SDEPaCfgWrapper *sde_pa_wrapper = NULL;

  sde_pa_wrapper = (SDEPaCfgWrapper *)in_data.GetConfigData();
  sde_pa = (struct SDEPaData *) sde_pa_wrapper;
  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for six zone");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & (kSixZoneHueMask | kSixZoneSatMask | kSixZoneValMask))) {
    /* PA SixZone feature disable case, but other PA features active */
    return ret;
  }

  if (in_data.enable_flags_ & kPaSixZoneEnable) {
    struct drm_msm_sixzone *mdp_sixzone = NULL;

    if ((!sde_pa->six_zone_curve_p0 || !sde_pa->six_zone_curve_p1) ||
        (sde_pa->six_zone_len != SIXZONE_LUT_SIZE)) {
        DLOGE("Invaid sixzone curve");
        return kErrorParameters;
    }

    mdp_sixzone = new drm_msm_sixzone[num_configs];
    if (!mdp_sixzone) {
      DLOGE("Failed to allocate memory for six zone");
      return kErrorMemory;
    }
    struct drm_msm_sixzone *curr_sixzone = mdp_sixzone;
    struct SDEPaData *input_pa = sde_pa;
    for (uint32_t num = 0; num < num_configs; num++) {
      curr_sixzone->flags = 0;
      if (num_configs > 1 && (!num)) {
        curr_sixzone->flags = ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
      } else {
        curr_sixzone->flags = 0;
      }

      if (input_pa->mode & kSixZoneHueMask) {
        curr_sixzone->flags |= SIXZONE_HUE_ENABLE;
      }
      if (input_pa->mode & kSixZoneSatMask) {
        curr_sixzone->flags |= SIXZONE_SAT_ENABLE;
      }
      if (input_pa->mode & kSixZoneValMask) {
        curr_sixzone->flags |= SIXZONE_VAL_ENABLE;
      }

      curr_sixzone->threshold = input_pa->six_zone_thresh;
      curr_sixzone->adjust_p0 = input_pa->six_zone_adj_p0;
      curr_sixzone->adjust_p1 = input_pa->six_zone_adj_p1;
      curr_sixzone->sat_hold = input_pa->six_zone_sat_hold;
      curr_sixzone->val_hold = input_pa->six_zone_val_hold;

      for (int i = 0; i < SIXZONE_LUT_SIZE; i++) {
        curr_sixzone->curve[i].p0 = input_pa->six_zone_curve_p0[i] & kSixZoneP0Mask;
        curr_sixzone->curve[i].p1 = input_pa->six_zone_curve_p1[i] & kSixZoneP1Mask;
      }
      curr_sixzone++;
      sde_pa_wrapper++;
      input_pa = (struct SDEPaData *) sde_pa_wrapper;
    }
    out_data->payload = mdp_sixzone;
    out_data->payload_size = num_configs * sizeof(struct drm_msm_sixzone);
  } else {
    /* PA SixZone configuration unchanged, no better return code available */
    ret = kErrorPermission;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAMemColSkin(const PPFeatureInfo &in_data,
                                                   DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
  struct SDEPaData *sde_pa;
  SDEPaCfgWrapper *sde_pa_wrapper = NULL;

  sde_pa_wrapper = (SDEPaCfgWrapper *)in_data.GetConfigData();
  sde_pa = (struct SDEPaData *) sde_pa_wrapper;
  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for memory color skin");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & kMemColorSkinMask)) {
    /* PA MemColSkin feature disable case, but other PA features active */
    return ret;
  }

  if (in_data.enable_flags_ & kPaSkinEnable) {
    struct drm_msm_memcol *mdp_memcol = NULL;
    struct SDEPaMemColorData *pa_memcol = &sde_pa->skin;

    mdp_memcol = new drm_msm_memcol[num_configs];
    if (!mdp_memcol) {
      DLOGE("Failed to allocate memory for memory color skin");
      return kErrorMemory;
    }

    struct drm_msm_memcol *curr_memcol = mdp_memcol;
    struct SDEPaMemColorData *input_memcol = pa_memcol;
    struct SDEPaData *input_pa = sde_pa;
    for (uint32_t num = 0; num < num_configs; num++) {
      if (num_configs > 1 && (!num)) {
        curr_memcol->flags |= ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
      } else {
        curr_memcol->flags |= ((uint64_t)0 << 60);
      }
      curr_memcol->prot_flags = 0;
      curr_memcol->color_adjust_p0 = input_memcol->adjust_p0;
      curr_memcol->color_adjust_p1 = input_memcol->adjust_p1;
      curr_memcol->color_adjust_p2 = input_memcol->adjust_p2;
      curr_memcol->blend_gain = input_memcol->blend_gain;
      curr_memcol->sat_hold = input_memcol->sat_hold;
      curr_memcol->val_hold = input_memcol->val_hold;
      curr_memcol->hue_region = input_memcol->hue_region;
      curr_memcol->sat_region = input_memcol->sat_region;
      curr_memcol->val_region = input_memcol->val_region;
      curr_memcol++;
      sde_pa_wrapper++;
      input_pa = (struct SDEPaData *) sde_pa_wrapper;
      input_memcol = &input_pa->skin;
    }
    out_data->payload = mdp_memcol;
    out_data->payload_size = num_configs * sizeof(struct drm_msm_memcol);
  } else {
    /* PA MemColSkin configuration unchanged, no better return code available */
    ret = kErrorPermission;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAMemColSky(const PPFeatureInfo &in_data,
                                                  DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
  struct SDEPaData *sde_pa;
  SDEPaCfgWrapper *sde_pa_wrapper = NULL;

  sde_pa_wrapper = (SDEPaCfgWrapper *)in_data.GetConfigData();
  sde_pa = (struct SDEPaData *) sde_pa_wrapper;
  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for memory color sky");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & kMemColorSkyMask)) {
    /* PA MemColSky feature disable case, but other PA features active */
    return ret;
  }

  if (in_data.enable_flags_ & kPaSkyEnable) {
    struct drm_msm_memcol *mdp_memcol = NULL;
    struct SDEPaMemColorData *pa_memcol = &sde_pa->sky;

    mdp_memcol = new drm_msm_memcol[num_configs];
    if (!mdp_memcol) {
      DLOGE("Failed to allocate memory for memory color sky");
      return kErrorMemory;
    }
    struct drm_msm_memcol *curr_memcol = mdp_memcol;
    struct SDEPaMemColorData *input_memcol = pa_memcol;
    struct SDEPaData *input_pa = sde_pa;
    for (uint32_t num = 0; num < num_configs; num++) {
      if (num_configs > 1 && (!num)) {
        curr_memcol->flags |= ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
      } else {
        curr_memcol->flags |= ((uint64_t)0 << 60);
      }
      curr_memcol->prot_flags = 0;
      curr_memcol->color_adjust_p0 = input_memcol->adjust_p0;
      curr_memcol->color_adjust_p1 = input_memcol->adjust_p1;
      curr_memcol->color_adjust_p2 = input_memcol->adjust_p2;
      curr_memcol->blend_gain = input_memcol->blend_gain;
      curr_memcol->sat_hold = input_memcol->sat_hold;
      curr_memcol->val_hold = input_memcol->val_hold;
      curr_memcol->hue_region = input_memcol->hue_region;
      curr_memcol->sat_region = input_memcol->sat_region;
      curr_memcol->val_region = input_memcol->val_region;
      curr_memcol++;
      sde_pa_wrapper++;
      input_pa = (struct SDEPaData *) sde_pa_wrapper;
      input_memcol = &input_pa->skin;
    }
    out_data->payload = mdp_memcol;
    out_data->payload_size = num_configs * sizeof(struct drm_msm_memcol);
  } else {
    /* PA MemColSky configuration unchanged, no better return code available */
    ret = kErrorPermission;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAMemColFoliage(const PPFeatureInfo &in_data,
                                                      DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
  struct SDEPaData *sde_pa;
  SDEPaCfgWrapper *sde_pa_wrapper = NULL;

  sde_pa_wrapper = (SDEPaCfgWrapper *)in_data.GetConfigData();
  sde_pa = (struct SDEPaData *) sde_pa_wrapper;
  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = 0;
  out_data->payload = NULL;

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    DLOGE("Invalid ops for memory color foliage");
    return kErrorParameters;
  }

  if (!(sde_pa->mode & kMemColorFolMask)) {
    /* PA MemColFoliage feature disable case, but other PA features active */
    return ret;
  }

  if (in_data.enable_flags_ & kPaFoliageEnable) {
    struct drm_msm_memcol *mdp_memcol = NULL;
    struct SDEPaMemColorData *pa_memcol = &sde_pa->foliage;

    mdp_memcol = new drm_msm_memcol[num_configs];
    if (!mdp_memcol) {
      DLOGE("Failed to allocate memory for memory color foliage");
      return kErrorMemory;
    }
    struct drm_msm_memcol *curr_memcol = mdp_memcol;
    struct SDEPaMemColorData *input_memcol = pa_memcol;
    struct SDEPaData *input_pa = sde_pa;
    for (uint32_t num = 0; num < num_configs; num++) {
      if (num_configs > 1 && (!num)) {
        curr_memcol->flags |= ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
      } else {
        curr_memcol->flags |= ((uint64_t)0 << 60);
      }
      curr_memcol->prot_flags = 0;
      curr_memcol->color_adjust_p0 = input_memcol->adjust_p0;
      curr_memcol->color_adjust_p1 = input_memcol->adjust_p1;
      curr_memcol->color_adjust_p2 = input_memcol->adjust_p2;
      curr_memcol->blend_gain = input_memcol->blend_gain;
      curr_memcol->sat_hold = input_memcol->sat_hold;
      curr_memcol->val_hold = input_memcol->val_hold;
      curr_memcol->hue_region = input_memcol->hue_region;
      curr_memcol->sat_region = input_memcol->sat_region;
      curr_memcol->val_region = input_memcol->val_region;
      curr_memcol++;
      sde_pa_wrapper++;
      input_pa = (struct SDEPaData *) sde_pa_wrapper;
      input_memcol = &input_pa->foliage;
    }
    out_data->payload = mdp_memcol;
    out_data->payload_size = num_configs * sizeof(struct drm_msm_memcol);
  } else {
    /* PA MemColFoliage configuration unchanged, no better return code available */
    ret = kErrorPermission;
  }
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPAMemColProt(const PPFeatureInfo &in_data,
                                                   DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_MEMCOL)
  struct SDEPaData *sde_pa;
  struct drm_msm_memcol *mdp_memcol;
  SDEPaCfgWrapper *sde_pa_wrapper = NULL;

  sde_pa_wrapper = (SDEPaCfgWrapper *)in_data.GetConfigData();
  sde_pa = (struct SDEPaData *) sde_pa_wrapper;
  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = sizeof(struct drm_msm_memcol);

  if (in_data.enable_flags_ & kOpsDisable) {
    /* Complete PA features disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  out_data->payload = NULL;

  mdp_memcol = new drm_msm_memcol[num_configs];
  if (!mdp_memcol) {
    DLOGE("Failed to allocate memory for memory color prot");
    return kErrorMemory;
  }
  struct drm_msm_memcol *curr_memcol = mdp_memcol;
  struct SDEPaData *input_pa = sde_pa;
  for (uint32_t num = 0; num < num_configs; num++) {
    if (num_configs > 1 && (!num)) {
        curr_memcol->flags |= ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
      } else {
        curr_memcol->flags |= ((uint64_t)0 << 60);
    }
    if (!(input_pa->mode & kMemColorProtMask)) {
      /* PA MemColProt feature disable case, but other PA features active */
      return ret;
    }

    curr_memcol->prot_flags = 0;
    if (input_pa->mode & kMemColorProtMask) {
      curr_memcol->prot_flags |= (input_pa->mode & kMemColorProtMask);
    }
    curr_memcol++;
    sde_pa_wrapper++;
    input_pa = (struct SDEPaData *) sde_pa_wrapper;
  }
  out_data->payload = mdp_memcol;
  out_data->payload_size = num_configs * sizeof(struct drm_msm_memcol);
#endif
  return ret;
}


DisplayError HWColorManagerDrm::GetDrmDither(const PPFeatureInfo &in_data,
                                             DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  struct SDEDitherCfg *sde_dither = NULL;
  struct drm_msm_dither *mdp_dither = NULL;

  sde_dither = (struct SDEDitherCfg *)in_data.GetConfigData();
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = sizeof(struct drm_msm_dither);

  if (in_data.enable_flags_ & kOpsDisable) {
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_dither = new drm_msm_dither();
  if (!mdp_dither) {
    DLOGE("Failed to allocate memory for dither");
    return kErrorMemory;
  }

  mdp_dither->flags = 0;
  std::memcpy(mdp_dither->matrix, sde_dither->dither_matrix,
                sizeof(sde_dither->dither_matrix));
  mdp_dither->temporal_en = sde_dither->temporal_en;
  mdp_dither->c0_bitdepth = sde_dither->g_y_depth;
  mdp_dither->c1_bitdepth = sde_dither->b_cb_depth;
  mdp_dither->c2_bitdepth = sde_dither->r_cr_depth;
  mdp_dither->c3_bitdepth = 0;
  out_data->payload = mdp_dither;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmGamut(const PPFeatureInfo &in_data,
                                            DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#ifdef PP_DRM_ENABLE
  SDEGamutCfgWrapper * sde_gamut_wrapper = NULL;
  struct drm_msm_3d_gamut *mdp_gamut = NULL;
  uint32_t size = 0;

  sde_gamut_wrapper = (SDEGamutCfgWrapper *)in_data.GetConfigData();
  uint32_t num_configs = 1;
  if (in_data.num_configs_ > 1) {
    num_configs = in_data.num_configs_;
  }
  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;
  out_data->payload_size = num_configs * sizeof(struct drm_msm_3d_gamut);
  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  mdp_gamut = new drm_msm_3d_gamut[num_configs];
  if (!mdp_gamut) {
    DLOGE("Failed to allocate memory for gamut");
    return kErrorMemory;
  }

  struct drm_msm_3d_gamut *curr_gamut = mdp_gamut;
  struct SDEGamutCfg *input_gamut = (struct SDEGamutCfg *)sde_gamut_wrapper;
  for (uint32_t num = 0; num < num_configs; num++) {
    if (input_gamut->map_en)
      curr_gamut->flags = GAMUT_3D_MAP_EN;
    else
      curr_gamut->flags = 0;
    if (num_configs > 1 && (!num)) {
      curr_gamut->flags |= ((uint64_t)(num_configs) << NUM_STRUCT_BITS);
    } else {
      curr_gamut->flags |= ((uint64_t)(0) << 60);
    }

    switch (input_gamut->mode) {
      case SDEGamutCfgWrapper::GAMUT_FINE_MODE:
        curr_gamut->mode = GAMUT_3D_MODE_17;
        size = GAMUT_3D_MODE17_TBL_SZ;
        break;
      case SDEGamutCfgWrapper::GAMUT_COARSE_MODE:
        curr_gamut->mode = GAMUT_3D_MODE_5;
        size = GAMUT_3D_MODE5_TBL_SZ;
        break;
      case SDEGamutCfgWrapper::GAMUT_COARSE_MODE_13:
        curr_gamut->mode = GAMUT_3D_MODE_13;
        size = GAMUT_3D_MODE13_TBL_SZ;
        break;
      default:
        DLOGE("Invalid gamut mode %d", input_gamut->mode);
        delete mdp_gamut;
        return kErrorParameters;
    }
    if (input_gamut->map_en) {
      std::memcpy(&curr_gamut->scale_off[0][0], input_gamut->scale_off_data[0],
                  sizeof(uint32_t) * GAMUT_3D_SCALE_OFF_SZ);
      std::memcpy(&curr_gamut->scale_off[1][0], input_gamut->scale_off_data[1],
                  sizeof(uint32_t) * GAMUT_3D_SCALE_OFF_SZ);
      std::memcpy(&curr_gamut->scale_off[2][0], input_gamut->scale_off_data[2],
                  sizeof(uint32_t) * GAMUT_3D_SCALE_OFF_SZ);
    }

    for (uint32_t row = 0; row < GAMUT_3D_TBL_NUM; row++) {
      for (uint32_t col = 0; col < size; col++) {
        curr_gamut->col[row][col].c0 = input_gamut->c0_data[row][col];
        curr_gamut->col[row][col].c2_c1 = input_gamut->c1_c2_data[row][col];
      }
    }
    curr_gamut++;
    sde_gamut_wrapper++;
    input_gamut = (struct SDEGamutCfg *)sde_gamut_wrapper;
  }
  out_data->payload = mdp_gamut;
#endif
  return ret;
}

DisplayError HWColorManagerDrm::GetDrmPADither(const PPFeatureInfo &in_data,
                                               DRMPPFeatureInfo *out_data) {
  DisplayError ret = kErrorNone;
#if defined(PP_DRM_ENABLE) && defined(DRM_MSM_PA_DITHER)
  struct SDEPADitherData* sde_dither;
  struct drm_msm_pa_dither* mdp_dither;

  out_data->type = sde_drm::kPropBlob;
  out_data->version = in_data.feature_version_;

  out_data->payload_size = sizeof(struct drm_msm_pa_dither);
  if (in_data.enable_flags_ & kOpsDisable) {
    /* feature disable case */
    out_data->payload = NULL;
    return ret;
  } else if (!(in_data.enable_flags_ & kOpsEnable)) {
    out_data->payload = NULL;
    return kErrorParameters;
  }

  sde_dither = (struct SDEPADitherData *)in_data.GetConfigData();
  if (sde_dither->matrix_size != DITHER_MATRIX_SZ) {
    DLOGE("Invalid dither matrix size %d, exp sz %d",
          sde_dither->matrix_size, DITHER_MATRIX_SZ);
    return kErrorParameters;
  }

  mdp_dither = new drm_msm_pa_dither();
  if (!mdp_dither) {
    DLOGE("Failed to allocate memory for dither");
    return kErrorMemory;
  }

  mdp_dither->flags = 0;
  mdp_dither->strength = sde_dither->strength;
  mdp_dither->offset_en = sde_dither->offset_en;
  std::memcpy(&mdp_dither->matrix[0],
              reinterpret_cast<void*>(sde_dither->matrix_data_addr),
              sizeof(uint32_t) * DITHER_MATRIX_SZ);
  out_data->payload = mdp_dither;
#endif
  return ret;
}

}  // namespace sdm
