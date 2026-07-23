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
 * RRC IE structures piggy-backed in F1AP LTM ReferenceConfiguration (TS 38.331).
 */

#ifndef RRC_LTM_REFERENCE_CONFIGURATION_H_
#define RRC_LTM_REFERENCE_CONFIGURATION_H_

#include "common/utils/ds/byte_array.h"

/** NR-RRC CellGroupConfig OCTET STRING (TS 38.331 6.2.2) */
typedef byte_array_t rrc_ltm_cell_group_config_t;

/** NR-RRC MeasurementTimingConfiguration OCTET STRING (TS 38.331 6.2.2) */
typedef byte_array_t rrc_ltm_measurement_timing_configuration_t;

typedef struct rrc_ltm_reference_configuration_information_s {
  rrc_ltm_cell_group_config_t *cellGroupConfig;
  rrc_ltm_measurement_timing_configuration_t *measurementTimingConfiguration;
} rrc_ltm_reference_configuration_information_t;

#endif /* RRC_LTM_REFERENCE_CONFIGURATION_H_ */
