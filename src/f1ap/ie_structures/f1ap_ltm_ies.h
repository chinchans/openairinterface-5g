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
 * F1AP Rel-18 LTM IE internal C structures (TS 38.473 9.2.2.1 / 9.2.2.2).
 * Encoded/decoded via f1ap_ltm_wire_codec until native ASN.1 codegen is available.
 */

#ifndef F1AP_LTM_IES_H_
#define F1AP_LTM_IES_H_

#include <stdbool.h>
#include <stdint.h>
#include "common/utils/ds/byte_array.h"
#include "src/rrc/ie_structures/rrc_ltm_reference_configuration.h"

#define F1AP_MAX_LTM_CONFIG_ID_MAPPING_LIST 8

typedef struct f1ap_reference_configuration_information_s {
  rrc_ltm_cell_group_config_t *cellGroupConfig;
  rrc_ltm_measurement_timing_configuration_t *measurementTimingConfiguration;
} f1ap_reference_configuration_information_t;

typedef struct f1ap_reference_configuration_s {
  bool request_for_lower_layer_configuration_present;
  bool request_for_lower_layer_configuration;
  f1ap_reference_configuration_information_t *reference_configuration_information;
} f1ap_reference_configuration_t;

typedef struct f1ap_csi_resource_configuration_s {
  byte_array_t *configuration;
} f1ap_csi_resource_configuration_t;

/** LTMConfiguration IE in UE CONTEXT SETUP RESPONSE (TS 38.473 9.3.1.x) */
typedef struct f1ap_ltm_configuration_s {
  f1ap_reference_configuration_t *reference_configuration;
  f1ap_csi_resource_configuration_t *csi_resource_configuration;
} f1ap_ltm_configuration_t;

typedef struct f1ap_ltm_configuration_id_mapping_item_s {
  uint8_t ltm_configuration_id;
  uint64_t *candidate_cell_id;
  f1ap_ltm_configuration_t ltm_configuration;
} f1ap_ltm_configuration_id_mapping_item_t;

/** LTMConfigurationIDMappingList IE in UE CONTEXT SETUP REQUEST */
typedef struct f1ap_ltm_configuration_id_mapping_list_s {
  int len;
  f1ap_ltm_configuration_id_mapping_item_t *items;
} f1ap_ltm_configuration_id_mapping_list_t;

/** LTMInformation-Setup IE in UE CONTEXT SETUP REQUEST */
typedef struct f1ap_ltm_information_setup_s {
  uint8_t setup_indication;
} f1ap_ltm_information_setup_t;

typedef struct f1ap_early_sync_information_request_s {
  bool request_for_rach_configuration;
} f1ap_early_sync_information_request_t;

typedef struct f1ap_early_ul_sync_configuration_s {
  uint8_t prachConfigurationIndex;
  uint16_t prachFrequencyOffset;
} f1ap_early_ul_sync_configuration_t;

#endif /* F1AP_LTM_IES_H_ */
