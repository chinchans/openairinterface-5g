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

void log_rrc_measurement_report(const gNB_RRC_UE_t *UE, const NR_MeasResults_t *measResults)
{
  if (UE == NULL) {
    return;
  }

  if (measResults == NULL) {
    LOG_W(NR_RRC, UE_LOG_FMT ": MeasurementReport received without MeasResults\n", UE_LOG_ARGS(UE));
    return;
  }

  const NR_MeasQuantityResults_t *quantity = nr_get_serving_cell_quantity_results(measResults);
  if (quantity == NULL) {
    LOG_W(NR_RRC, UE_LOG_FMT ": MeasurementReport without serving-cell quantity results\n", UE_LOG_ARGS(UE));
    return;
  }

  char timestamp[32] = {0};
  get_current_timestamp(timestamp, sizeof(timestamp));

  const long meas_id = measResults->measId;
  long phys_cell_id = -1;
  if (measResults->measResultServingMOList.list.count >= 1) {
    const NR_MeasResultNR_t *serv_cell = &measResults->measResultServingMOList.list.array[0]->measResultServingCell;
    if (serv_cell->physCellId != NULL) {
      phys_cell_id = *serv_cell->physCellId;
    }
  }

  if (quantity->rsrp != NULL) {
    const long rsrp_dbm = nr_rsrp_index_to_dbm(*quantity->rsrp);
    if (!nr_rsrp_dbm_in_range(rsrp_dbm)) {
      LOG_W(NR_RRC,
            UE_LOG_FMT ": serving-cell RSRP %ld dBm out of range [-140..-44]\n",
            UE_LOG_ARGS(UE),
            rsrp_dbm);
    }
    if (quantity->rsrq != NULL) {
      const float rsrq_db = nr_rrq_index_to_db(*quantity->rsrq);
      if (!nr_rrq_db_in_range(rsrq_db)) {
        LOG_W(NR_RRC,
              UE_LOG_FMT ": serving-cell RSRQ %.1f dB out of range [-19..-3]\n",
              UE_LOG_ARGS(UE),
              rsrq_db);
      }
      LOG_I(NR_RRC,
            "MeasurementReport serving cell: " UE_LOG_FMT " measId %ld physCellId %ld RSRP %ld dBm RSRQ %.1f dB timestamp %s\n",
            UE_LOG_ARGS(UE),
            meas_id,
            phys_cell_id,
            rsrp_dbm,
            rsrq_db,
            timestamp);
    } else {
      LOG_I(NR_RRC,
            "MeasurementReport serving cell: " UE_LOG_FMT " measId %ld physCellId %ld RSRP %ld dBm timestamp %s\n",
            UE_LOG_ARGS(UE),
            meas_id,
            phys_cell_id,
            rsrp_dbm,
            timestamp);
    }
    return;
  }

  if (quantity->rsrq != NULL) {
    const float rsrq_db = nr_rrq_index_to_db(*quantity->rsrq);
    if (!nr_rrq_db_in_range(rsrq_db)) {
      LOG_W(NR_RRC,
            UE_LOG_FMT ": serving-cell RSRQ %.1f dB out of range [-19..-3]\n",
            UE_LOG_ARGS(UE),
            rsrq_db);
    }
    LOG_I(NR_RRC,
          "MeasurementReport serving cell: " UE_LOG_FMT " measId %ld physCellId %ld RSRQ %.1f dB timestamp %s\n",
          UE_LOG_ARGS(UE),
          meas_id,
          phys_cell_id,
          rsrq_db,
          timestamp);
    return;
  }

  LOG_W(NR_RRC,
        UE_LOG_FMT ": MeasurementReport serving cell without RSRP or RSRQ (measId %ld physCellId %ld) timestamp %s\n",
        UE_LOG_ARGS(UE),
        meas_id,
        phys_cell_id,
        timestamp);
}
