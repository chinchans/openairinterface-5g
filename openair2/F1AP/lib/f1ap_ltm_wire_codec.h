/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * Internal wire codec for Rel-18 LTM F1AP IEs not yet present in ASN.1 codegen.
 * Transported via ResourceCoordinationTransferContainer (TS 38.473).
 * TODO: replace with native ASN.1 encode/decode after F1AP Rel-18 upgrade.
 */

#ifndef F1AP_LTM_WIRE_CODEC_H_
#define F1AP_LTM_WIRE_CODEC_H_

#include <stdbool.h>
#include <stdint.h>
#include "f1ap_messages_types.h"

#define F1AP_LTM_WIRE_MAGIC 0x314D544C /* "LTM1" little-endian */
#define F1AP_LTM_WIRE_VERSION 3

#define F1AP_LTM_WIRE_TAG_INFORMATION_SETUP 1
#define F1AP_LTM_WIRE_TAG_CONFIGURATION_ID_MAPPING_LIST 2
#define F1AP_LTM_WIRE_TAG_LTM_CONFIGURATION 3
#define F1AP_LTM_WIRE_TAG_EARLY_UL_SYNC_CONFIGURATION 4
#define F1AP_LTM_WIRE_TAG_REQUESTED_TARGET_CELL_ID 5

bool f1ap_ltm_wire_is_ltm_container(const byte_array_t *ba);
byte_array_t *f1ap_ltm_wire_encode_ue_ctx_setup_req_ltm(const f1ap_ue_context_setup_req_t *req);
bool f1ap_ltm_wire_decode_ue_ctx_setup_req_ltm(const byte_array_t *ba, f1ap_ue_context_setup_req_t *req);
byte_array_t *f1ap_ltm_wire_encode_ue_ctx_setup_resp_ltm(const f1ap_ue_context_setup_resp_t *resp);
bool f1ap_ltm_wire_decode_ue_ctx_setup_resp_ltm(const byte_array_t *ba, f1ap_ue_context_setup_resp_t *resp);

void f1ap_ltm_free_reference_configuration_information(f1ap_reference_configuration_information_t *rci);
f1ap_reference_configuration_information_t cp_f1ap_ltm_reference_configuration_information(
    const f1ap_reference_configuration_information_t *orig);
bool eq_f1ap_ltm_reference_configuration_information(const f1ap_reference_configuration_information_t *a,
                                                     const f1ap_reference_configuration_information_t *b);

void f1ap_ltm_free_reference_configuration(f1ap_reference_configuration_t *rc);
f1ap_reference_configuration_t *cp_f1ap_ltm_reference_configuration(const f1ap_reference_configuration_t *orig);
bool eq_f1ap_ltm_reference_configuration(const f1ap_reference_configuration_t *a,
                                         const f1ap_reference_configuration_t *b);

void f1ap_ltm_free_csi_resource_configuration(f1ap_csi_resource_configuration_t *crc);
f1ap_csi_resource_configuration_t *cp_f1ap_ltm_csi_resource_configuration(const f1ap_csi_resource_configuration_t *orig);
bool eq_f1ap_ltm_csi_resource_configuration(const f1ap_csi_resource_configuration_t *a,
                                            const f1ap_csi_resource_configuration_t *b);

void f1ap_ltm_free_ltm_configuration(f1ap_ltm_configuration_t *lc);
f1ap_ltm_configuration_t *cp_f1ap_ltm_ltm_configuration(const f1ap_ltm_configuration_t *orig);
bool eq_f1ap_ltm_ltm_configuration(const f1ap_ltm_configuration_t *a, const f1ap_ltm_configuration_t *b);

void f1ap_ltm_free_configuration_id_mapping_list(f1ap_ltm_configuration_id_mapping_list_t *list);
f1ap_ltm_configuration_id_mapping_list_t *cp_f1ap_ltm_configuration_id_mapping_list(
    const f1ap_ltm_configuration_id_mapping_list_t *orig);
bool eq_f1ap_ltm_configuration_id_mapping_list(const f1ap_ltm_configuration_id_mapping_list_t *a,
                                               const f1ap_ltm_configuration_id_mapping_list_t *b);

void f1ap_ltm_free_information_setup(f1ap_ltm_information_setup_t *setup);
f1ap_ltm_information_setup_t *cp_f1ap_ltm_information_setup(const f1ap_ltm_information_setup_t *orig);
bool eq_f1ap_ltm_information_setup(const f1ap_ltm_information_setup_t *a, const f1ap_ltm_information_setup_t *b);

void f1ap_ltm_free_early_ul_sync_configuration(f1ap_early_ul_sync_configuration_t *cfg);
f1ap_early_ul_sync_configuration_t *cp_f1ap_ltm_early_ul_sync_configuration(const f1ap_early_ul_sync_configuration_t *orig);
bool eq_f1ap_ltm_early_ul_sync_configuration(const f1ap_early_ul_sync_configuration_t *a,
                                             const f1ap_early_ul_sync_configuration_t *b);

void f1ap_ltm_free_ue_context_setup_req_ltm(f1ap_ue_context_setup_req_t *req);
void f1ap_ltm_free_ue_context_setup_resp_ltm(f1ap_ue_context_setup_resp_t *resp);

#endif /* F1AP_LTM_WIRE_CODEC_H_ */
