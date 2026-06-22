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
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#ifndef RRC_GNB_UTILS_H
#define RRC_GNB_UTILS_H

#include "NR_MeasurementReport.h"
#include "nr_rrc_defs.h"

/** Normalized serving-cell measurement view extracted from NR_MeasResults (TS 38.331). */
typedef struct nr_meas_serving_cell_log_s {
  long measId;
  long physCellId;
  bool has_physCellId;
  long rsrp_dbm;
  bool has_rsrp;
  float rsrq_db;
  bool has_rsrq;
} nr_meas_serving_cell_log_t;

/**
 * @brief Extract serving-cell RSRP/RSRQ from decoded MeasResults.
 * @return true when at least one quantity (RSRP or RSRQ) is present.
 */
bool nr_extract_serving_cell_measurements(const NR_MeasResults_t *measResults, nr_meas_serving_cell_log_t *out);

/**
 * @brief Log serving-cell RSRP/RSRQ from a MeasurementReport at the gNB RRC layer.
 *        TS 38.331 §5.5.2 — records UE identifier, measurements, and reception timestamp.
 */
void log_rrc_measurement_report(const gNB_RRC_UE_t *UE, const NR_MeasResults_t *measResults);

#endif /* RRC_GNB_UTILS_H */
