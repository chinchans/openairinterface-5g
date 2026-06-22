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

#include "RRC/NR/rrc_gNB_utils.h"

#include <stdbool.h>
#include <string.h>

#include "common/utils/LOG/log.h"
#include "time_utils.h"

/** @brief Convert ASN.1 RSRP-Range index to dBm (TS 38.133 Table 10.1.6.1-1). */
static long nr_rsrp_index_to_dbm(long rsrp_index)
{
  return rsrp_index - 156;
}

/** @brief Convert ASN.1 RSRQ-Range index to dB (TS 38.133 Table 10.1.11.1-1). */
static float nr_rrq_index_to_db(long rsrq_index)
{
  return (float)(rsrq_index - 87) / 2.0f;
}

static bool nr_rsrp_dbm_in_range(long rsrp_dbm)
{
  return rsrp_dbm >= -140 && rsrp_dbm <= -44;
}

static bool nr_rrq_db_in_range(float rsrq_db)
{
  return rsrq_db >= -19.0f && rsrq_db <= -3.0f;
}

static bool nr_meas_id_in_range(long meas_id)
{
  return meas_id >= 1 && meas_id <= 64;
}

static bool nr_phys_cell_id_in_range(long phys_cell_id)
{
  return phys_cell_id >= 0 && phys_cell_id <= 1007;
}

static const NR_MeasQuantityResults_t *nr_get_serving_cell_quantity_results(const NR_MeasResults_t *measResults)
{
  if (measResults == NULL || measResults->measResultServingMOList.list.count < 1) {
    return NULL;
  }

  const NR_MeasResultServMO_t *serv_mo = measResults->measResultServingMOList.list.array[0];
  const struct NR_MeasResultNR__measResult__cellResults *cell_results = &serv_mo->measResultServingCell.measResult.cellResults;

  if (cell_results->resultsSSB_Cell != NULL) {
    return cell_results->resultsSSB_Cell;
  }
  if (cell_results->resultsCSI_RS_Cell != NULL) {
    return cell_results->resultsCSI_RS_Cell;
  }
  return NULL;
}

bool nr_extract_serving_cell_measurements(const NR_MeasResults_t *measResults, nr_meas_serving_cell_log_t *out)
{
  if (measResults == NULL || out == NULL) {
    return false;
  }

  memset(out, 0, sizeof(*out));
  out->measId = measResults->measId;
  out->physCellId = -1;

  if (!nr_meas_id_in_range(out->measId)) {
    return false;
  }

  if (measResults->measResultServingMOList.list.count < 1) {
    return false;
  }

  const NR_MeasResultNR_t *serv_cell = &measResults->measResultServingMOList.list.array[0]->measResultServingCell;
  if (serv_cell->physCellId != NULL) {
    out->physCellId = *serv_cell->physCellId;
    out->has_physCellId = true;
  }

  const NR_MeasQuantityResults_t *quantity = nr_get_serving_cell_quantity_results(measResults);
  if (quantity == NULL) {
    return false;
  }

  if (quantity->rsrp != NULL) {
    out->rsrp_dbm = nr_rsrp_index_to_dbm(*quantity->rsrp);
    out->has_rsrp = true;
  }
  if (quantity->rsrq != NULL) {
    out->rsrq_db = nr_rrq_index_to_db(*quantity->rsrq);
    out->has_rsrq = true;
  }

  return out->has_rsrp || out->has_rsrq;
}

void log_rrc_measurement_report(const gNB_RRC_UE_t *UE, const NR_MeasResults_t *measResults)
{
  if (UE == NULL) {
    return;
  }

  char timestamp[32] = {0};
  get_current_timestamp(timestamp, sizeof(timestamp));

  if (measResults == NULL) {
    LOG_W(NR_RRC, UE_LOG_FMT ": MeasurementReport received without MeasResults timestamp %s\n", UE_LOG_ARGS(UE), timestamp);
    return;
  }

  if (!nr_meas_id_in_range(measResults->measId)) {
    LOG_W(NR_RRC,
          UE_LOG_FMT ": MeasurementReport measId %ld out of range [1..64] timestamp %s\n",
          UE_LOG_ARGS(UE),
          measResults->measId,
          timestamp);
    return;
  }

  nr_meas_serving_cell_log_t serving = {0};
  if (!nr_extract_serving_cell_measurements(measResults, &serving)) {
    LOG_W(NR_RRC,
          UE_LOG_FMT ": MeasurementReport without serving-cell quantity results (measId %ld) timestamp %s\n",
          UE_LOG_ARGS(UE),
          measResults->measId,
          timestamp);
    return;
  }

  const long phys_cell_id = serving.has_physCellId ? serving.physCellId : -1;

  if (serving.has_physCellId && !nr_phys_cell_id_in_range(serving.physCellId)) {
    LOG_W(NR_RRC,
          UE_LOG_FMT ": serving-cell physCellId %ld out of range [0..1007]\n",
          UE_LOG_ARGS(UE),
          serving.physCellId);
  }
  if (serving.has_rsrp && !nr_rsrp_dbm_in_range(serving.rsrp_dbm)) {
    LOG_W(NR_RRC,
          UE_LOG_FMT ": serving-cell RSRP %ld dBm out of range [-140..-44]\n",
          UE_LOG_ARGS(UE),
          serving.rsrp_dbm);
  }
  if (serving.has_rsrq && !nr_rrq_db_in_range(serving.rsrq_db)) {
    LOG_W(NR_RRC,
          UE_LOG_FMT ": serving-cell RSRQ %.1f dB out of range [-19..-3]\n",
          UE_LOG_ARGS(UE),
          serving.rsrq_db);
  }

  if (serving.has_rsrp && serving.has_rsrq) {
    LOG_I(NR_RRC,
          "MeasurementReport serving cell: " UE_LOG_FMT " measId %ld physCellId %ld RSRP %ld dBm RSRQ %.1f dB timestamp %s\n",
          UE_LOG_ARGS(UE),
          serving.measId,
          phys_cell_id,
          serving.rsrp_dbm,
          serving.rsrq_db,
          timestamp);
    return;
  }

  if (serving.has_rsrp) {
    LOG_I(NR_RRC,
          "MeasurementReport serving cell: " UE_LOG_FMT " measId %ld physCellId %ld RSRP %ld dBm timestamp %s\n",
          UE_LOG_ARGS(UE),
          serving.measId,
          phys_cell_id,
          serving.rsrp_dbm,
          timestamp);
    return;
  }

  LOG_I(NR_RRC,
        "MeasurementReport serving cell: " UE_LOG_FMT " measId %ld physCellId %ld RSRQ %.1f dB timestamp %s\n",
        UE_LOG_ARGS(UE),
        serving.measId,
        phys_cell_id,
        serving.rsrq_db,
        timestamp);
}
