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
 * Internal wire codec for Rel-18 LTM F1AP IEs (TS 38.473 9.2.2.1/9.2.2.2).
 * TODO: replace with native ASN.1 after F1AP Rel-18 codegen upgrade.
 */

#include "f1ap_ltm_wire_codec.h"

#include <string.h>
#include "f1ap_lib_common.h"
#include "common/utils/assertions.h"
#include "common/utils/utils.h"
#include "common/utils/ds/byte_array.h"

static bool ltm_eq_ba_ptr(const byte_array_t *a, const byte_array_t *b)
{
  if (!a && !b)
    return true;
  if (!a || !b)
    return false;
  return eq_byte_array(a, b);
}

typedef struct ltm_wire_writer_s {
  uint8_t *buf;
  size_t cap;
  size_t pos;
} ltm_wire_writer_t;

typedef struct ltm_wire_reader_s {
  const uint8_t *buf;
  size_t len;
  size_t pos;
} ltm_wire_reader_t;

static bool ltm_wire_writer_ensure(ltm_wire_writer_t *w, size_t need)
{
  if (w->pos + need <= w->cap)
    return true;
  size_t new_cap = w->cap ? w->cap : 256;
  while (new_cap < w->pos + need)
    new_cap *= 2;
  uint8_t *nb = realloc(w->buf, new_cap);
  if (!nb)
    return false;
  w->buf = nb;
  w->cap = new_cap;
  return true;
}

static bool ltm_wire_write_u8(ltm_wire_writer_t *w, uint8_t v)
{
  if (!ltm_wire_writer_ensure(w, 1))
    return false;
  w->buf[w->pos++] = v;
  return true;
}

static bool ltm_wire_write_u16(ltm_wire_writer_t *w, uint16_t v)
{
  if (!ltm_wire_writer_ensure(w, 2))
    return false;
  w->buf[w->pos++] = (uint8_t)(v & 0xff);
  w->buf[w->pos++] = (uint8_t)((v >> 8) & 0xff);
  return true;
}

static bool ltm_wire_write_u32(ltm_wire_writer_t *w, uint32_t v)
{
  if (!ltm_wire_writer_ensure(w, 4))
    return false;
  w->buf[w->pos++] = (uint8_t)(v & 0xff);
  w->buf[w->pos++] = (uint8_t)((v >> 8) & 0xff);
  w->buf[w->pos++] = (uint8_t)((v >> 16) & 0xff);
  w->buf[w->pos++] = (uint8_t)((v >> 24) & 0xff);
  return true;
}

static bool ltm_wire_write_u64(ltm_wire_writer_t *w, uint64_t v)
{
  if (!ltm_wire_write_u32(w, (uint32_t)(v & 0xffffffff)))
    return false;
  return ltm_wire_write_u32(w, (uint32_t)(v >> 32));
}

static bool ltm_wire_write_bytes(ltm_wire_writer_t *w, const uint8_t *data, uint32_t len)
{
  if (!ltm_wire_write_u32(w, len))
    return false;
  if (len == 0)
    return true;
  if (!ltm_wire_writer_ensure(w, len))
    return false;
  memcpy(w->buf + w->pos, data, len);
  w->pos += len;
  return true;
}

static bool ltm_wire_read_u8(ltm_wire_reader_t *r, uint8_t *v)
{
  if (r->pos + 1 > r->len)
    return false;
  *v = r->buf[r->pos++];
  return true;
}

static bool ltm_wire_read_u16(ltm_wire_reader_t *r, uint16_t *v)
{
  if (r->pos + 2 > r->len)
    return false;
  *v = (uint16_t)r->buf[r->pos] | ((uint16_t)r->buf[r->pos + 1] << 8);
  r->pos += 2;
  return true;
}

static bool ltm_wire_read_u32(ltm_wire_reader_t *r, uint32_t *v)
{
  if (r->pos + 4 > r->len)
    return false;
  *v = (uint32_t)r->buf[r->pos] | ((uint32_t)r->buf[r->pos + 1] << 8) | ((uint32_t)r->buf[r->pos + 2] << 16)
       | ((uint32_t)r->buf[r->pos + 3] << 24);
  r->pos += 4;
  return true;
}

static bool ltm_wire_read_u64(ltm_wire_reader_t *r, uint64_t *v)
{
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (!ltm_wire_read_u32(r, &lo) || !ltm_wire_read_u32(r, &hi))
    return false;
  *v = ((uint64_t)hi << 32) | lo;
  return true;
}

static bool ltm_wire_read_bytes(ltm_wire_reader_t *r, byte_array_t *out)
{
  uint32_t len = 0;
  if (!ltm_wire_read_u32(r, &len))
    return false;
  if (r->pos + len > r->len)
    return false;
  if (len == 0) {
    *out = (byte_array_t){0};
    return true;
  }
  *out = create_byte_array(len, (uint8_t *)r->buf + r->pos);
  r->pos += len;
  return true;
}

static bool ltm_wire_write_ba(ltm_wire_writer_t *w, const byte_array_t *ba)
{
  if (!ba)
    return ltm_wire_write_bytes(w, NULL, 0);
  return ltm_wire_write_bytes(w, ba->buf, (uint32_t)ba->len);
}

static bool ltm_wire_read_ba_ptr(ltm_wire_reader_t *r, byte_array_t **out)
{
  byte_array_t tmp = {0};
  if (!ltm_wire_read_bytes(r, &tmp))
    return false;
  if (tmp.len == 0) {
    *out = NULL;
    return true;
  }
  *out = malloc_or_fail(sizeof(**out));
  **out = tmp;
  return true;
}

static bool ltm_wire_encode_reference_configuration_information(ltm_wire_writer_t *w,
                                                                  const f1ap_reference_configuration_information_t *rci)
{
  if (!rci) {
    if (!ltm_wire_write_u8(w, 0))
      return false;
    if (!ltm_wire_write_u8(w, 0))
      return false;
    return true;
  }
  if (!ltm_wire_write_u8(w, rci->cellGroupConfig ? 1 : 0))
    return false;
  if (rci->cellGroupConfig && !ltm_wire_write_ba(w, rci->cellGroupConfig))
    return false;
  if (!ltm_wire_write_u8(w, rci->measurementTimingConfiguration ? 1 : 0))
    return false;
  if (rci->measurementTimingConfiguration && !ltm_wire_write_ba(w, rci->measurementTimingConfiguration))
    return false;
  return true;
}

static bool ltm_wire_decode_reference_configuration_information(ltm_wire_reader_t *r,
                                                                  f1ap_reference_configuration_information_t *rci)
{
  uint8_t cgc_present = 0;
  uint8_t mtc_present = 0;
  if (!ltm_wire_read_u8(r, &cgc_present))
    return false;
  if (cgc_present) {
    if (!ltm_wire_read_ba_ptr(r, &rci->cellGroupConfig))
      return false;
  }
  if (!ltm_wire_read_u8(r, &mtc_present))
    return false;
  if (mtc_present) {
    if (!ltm_wire_read_ba_ptr(r, &rci->measurementTimingConfiguration))
      return false;
  }
  return true;
}

static bool ltm_wire_encode_reference_configuration(ltm_wire_writer_t *w, const f1ap_reference_configuration_t *rc)
{
  if (!rc) {
    if (!ltm_wire_write_u8(w, 0))
      return false;
    return true;
  }
  if (!ltm_wire_write_u8(w, 1))
    return false;
  if (!ltm_wire_write_u8(w, rc->request_for_lower_layer_configuration_present ? 1 : 0))
    return false;
  if (rc->request_for_lower_layer_configuration_present) {
    if (!ltm_wire_write_u8(w, rc->request_for_lower_layer_configuration ? 1 : 0))
      return false;
  }
  if (!ltm_wire_write_u8(w, rc->reference_configuration_information ? 1 : 0))
    return false;
  if (rc->reference_configuration_information) {
    if (!ltm_wire_encode_reference_configuration_information(w, rc->reference_configuration_information))
      return false;
  }
  return true;
}

static bool ltm_wire_decode_reference_configuration(ltm_wire_reader_t *r, f1ap_reference_configuration_t **out)
{
  uint8_t present = 0;
  if (!ltm_wire_read_u8(r, &present))
    return false;
  if (!present) {
    *out = NULL;
    return true;
  }
  *out = calloc_or_fail(1, sizeof(**out));
  uint8_t rflc_present = 0;
  if (!ltm_wire_read_u8(r, &rflc_present))
    return false;
  (*out)->request_for_lower_layer_configuration_present = rflc_present != 0;
  if (rflc_present) {
    uint8_t val = 0;
    if (!ltm_wire_read_u8(r, &val))
      return false;
    (*out)->request_for_lower_layer_configuration = val != 0;
  }
  uint8_t rci_present = 0;
  if (!ltm_wire_read_u8(r, &rci_present))
    return false;
  if (rci_present) {
    (*out)->reference_configuration_information = calloc_or_fail(1, sizeof(*(*out)->reference_configuration_information));
    if (!ltm_wire_decode_reference_configuration_information(r, (*out)->reference_configuration_information))
      return false;
  }
  return true;
}

static bool ltm_wire_encode_csi_resource_configuration(ltm_wire_writer_t *w, const f1ap_csi_resource_configuration_t *crc)
{
  return ltm_wire_write_ba(w, crc ? crc->configuration : NULL);
}

static bool ltm_wire_decode_csi_resource_configuration(ltm_wire_reader_t *r, f1ap_csi_resource_configuration_t **out)
{
  byte_array_t *ba = NULL;
  if (!ltm_wire_read_ba_ptr(r, &ba))
    return false;
  if (!ba) {
    *out = NULL;
    return true;
  }
  *out = calloc_or_fail(1, sizeof(**out));
  (*out)->configuration = ba;
  return true;
}

static bool ltm_wire_encode_ltm_configuration(ltm_wire_writer_t *w, const f1ap_ltm_configuration_t *lc)
{
  if (!lc) {
    if (!ltm_wire_write_u8(w, 0))
      return false;
    return true;
  }
  if (!ltm_wire_write_u8(w, 1))
    return false;
  if (!ltm_wire_write_u8(w, lc->reference_configuration ? 1 : 0))
    return false;
  if (lc->reference_configuration) {
    if (!ltm_wire_encode_reference_configuration(w, lc->reference_configuration))
      return false;
  }
  if (!ltm_wire_write_u8(w, lc->csi_resource_configuration ? 1 : 0))
    return false;
  if (lc->csi_resource_configuration) {
    if (!ltm_wire_encode_csi_resource_configuration(w, lc->csi_resource_configuration))
      return false;
  }
  return true;
}

static bool ltm_wire_decode_ltm_configuration(ltm_wire_reader_t *r, f1ap_ltm_configuration_t **out)
{
  uint8_t present = 0;
  if (!ltm_wire_read_u8(r, &present))
    return false;
  if (!present) {
    *out = NULL;
    return true;
  }
  *out = calloc_or_fail(1, sizeof(**out));
  uint8_t rc_present = 0;
  if (!ltm_wire_read_u8(r, &rc_present))
    return false;
  if (rc_present) {
    if (!ltm_wire_decode_reference_configuration(r, &(*out)->reference_configuration))
      return false;
  }
  uint8_t crc_present = 0;
  if (!ltm_wire_read_u8(r, &crc_present))
    return false;
  if (crc_present) {
    if (!ltm_wire_decode_csi_resource_configuration(r, &(*out)->csi_resource_configuration))
      return false;
  }
  return true;
}

static bool ltm_wire_encode_configuration_id_mapping_list(ltm_wire_writer_t *w,
                                                          const f1ap_ltm_configuration_id_mapping_list_t *list)
{
  if (!list || list->len == 0) {
    if (!ltm_wire_write_u16(w, 0))
      return false;
    return true;
  }
  if (list->len > F1AP_MAX_LTM_CONFIG_ID_MAPPING_LIST)
    return false;
  if (!ltm_wire_write_u16(w, (uint16_t)list->len))
    return false;
  for (int i = 0; i < list->len; ++i) {
    const f1ap_ltm_configuration_id_mapping_item_t *item = &list->items[i];
    if (!ltm_wire_write_u8(w, item->ltm_configuration_id))
      return false;
    if (!ltm_wire_write_u8(w, item->candidate_cell_id ? 1 : 0))
      return false;
    if (item->candidate_cell_id) {
      if (!ltm_wire_write_u64(w, *item->candidate_cell_id))
        return false;
    }
    if (!ltm_wire_encode_ltm_configuration(w, &item->ltm_configuration))
      return false;
  }
  return true;
}

static bool ltm_wire_decode_configuration_id_mapping_list(ltm_wire_reader_t *r,
                                                          f1ap_ltm_configuration_id_mapping_list_t **out)
{
  uint16_t count = 0;
  if (!ltm_wire_read_u16(r, &count))
    return false;
  if (count == 0) {
    *out = NULL;
    return true;
  }
  if (count > F1AP_MAX_LTM_CONFIG_ID_MAPPING_LIST)
    return false;
  *out = calloc_or_fail(1, sizeof(**out));
  (*out)->len = count;
  (*out)->items = calloc_or_fail(count, sizeof(*(*out)->items));
  for (int i = 0; i < count; ++i) {
    if (!ltm_wire_read_u8(r, &(*out)->items[i].ltm_configuration_id))
      return false;
    uint8_t candidate_present = 0;
    if (!ltm_wire_read_u8(r, &candidate_present))
      return false;
    if (candidate_present) {
      uint64_t cell_id = 0;
      if (!ltm_wire_read_u64(r, &cell_id))
        return false;
      (*out)->items[i].candidate_cell_id = malloc_or_fail(sizeof(*(*out)->items[i].candidate_cell_id));
      *(*out)->items[i].candidate_cell_id = cell_id;
    }
    f1ap_ltm_configuration_t *lc = NULL;
    if (!ltm_wire_decode_ltm_configuration(r, &lc))
      return false;
    if (lc) {
      (*out)->items[i].ltm_configuration = *lc;
      free(lc);
    }
  }
  return true;
}

static bool ltm_wire_encode_information_setup(ltm_wire_writer_t *w, const f1ap_ltm_information_setup_t *setup)
{
  if (!setup) {
    if (!ltm_wire_write_u8(w, 0))
      return false;
    return true;
  }
  if (!ltm_wire_write_u8(w, 1))
    return false;
  return ltm_wire_write_u8(w, setup->setup_indication);
}

static bool ltm_wire_decode_information_setup(ltm_wire_reader_t *r, f1ap_ltm_information_setup_t **out)
{
  uint8_t present = 0;
  if (!ltm_wire_read_u8(r, &present))
    return false;
  if (!present) {
    *out = NULL;
    return true;
  }
  *out = calloc_or_fail(1, sizeof(**out));
  return ltm_wire_read_u8(r, &(*out)->setup_indication);
}

static bool ltm_wire_encode_early_ul_sync_configuration(ltm_wire_writer_t *w,
                                                        const f1ap_early_ul_sync_configuration_t *cfg)
{
  if (!cfg)
    return false;
  if (!ltm_wire_write_u8(w, cfg->prachConfigurationIndex))
    return false;
  return ltm_wire_write_u16(w, cfg->prachFrequencyOffset);
}

static bool ltm_wire_decode_early_ul_sync_configuration(ltm_wire_reader_t *r, f1ap_early_ul_sync_configuration_t **out)
{
  if (!out)
    return false;
  *out = calloc_or_fail(1, sizeof(**out));
  if (!ltm_wire_read_u8(r, &(*out)->prachConfigurationIndex))
    return false;
  return ltm_wire_read_u16(r, &(*out)->prachFrequencyOffset);
}

static byte_array_t *ltm_wire_finalize(ltm_wire_writer_t *w, uint16_t num_tags)
{
  ltm_wire_writer_t hdr = {0};
  if (!ltm_wire_write_u32(&hdr, F1AP_LTM_WIRE_MAGIC))
    goto fail;
  if (!ltm_wire_write_u16(&hdr, F1AP_LTM_WIRE_VERSION))
    goto fail;
  if (!ltm_wire_write_u16(&hdr, num_tags))
    goto fail;
  size_t total = hdr.pos + w->pos;
  uint8_t *buf = malloc_or_fail(total);
  memcpy(buf, hdr.buf, hdr.pos);
  memcpy(buf + hdr.pos, w->buf, w->pos);
  free(hdr.buf);
  free(w->buf);
  byte_array_t *out = malloc_or_fail(sizeof(*out));
  out->buf = buf;
  out->len = total;
  return out;
fail:
  free(hdr.buf);
  free(w->buf);
  return NULL;
}

static bool ltm_wire_parse_header(ltm_wire_reader_t *r, uint16_t *num_tags)
{
  uint32_t magic = 0;
  uint16_t version = 0;
  if (!ltm_wire_read_u32(r, &magic) || magic != F1AP_LTM_WIRE_MAGIC)
    return false;
  if (!ltm_wire_read_u16(r, &version) || version != F1AP_LTM_WIRE_VERSION)
    return false;
  return ltm_wire_read_u16(r, num_tags);
}

bool f1ap_ltm_wire_is_ltm_container(const byte_array_t *ba)
{
  if (!ba || ba->len < 8)
    return false;
  uint32_t magic = (uint32_t)ba->buf[0] | ((uint32_t)ba->buf[1] << 8) | ((uint32_t)ba->buf[2] << 16)
                   | ((uint32_t)ba->buf[3] << 24);
  return magic == F1AP_LTM_WIRE_MAGIC;
}

byte_array_t *f1ap_ltm_wire_encode_ue_ctx_setup_req_ltm(const f1ap_ue_context_setup_req_t *req)
{
  if (!req)
    return NULL;
  if (!req->ltm_information_setup && !req->ltm_configuration_id_mapping_list)
    return NULL;

  ltm_wire_writer_t payload = {0};
  uint16_t num_tags = 0;

  if (req->ltm_information_setup) {
    if (!ltm_wire_write_u16(&payload, F1AP_LTM_WIRE_TAG_INFORMATION_SETUP))
      goto fail;
    ltm_wire_writer_t ie = {0};
    if (!ltm_wire_encode_information_setup(&ie, req->ltm_information_setup))
      goto fail_ie;
    if (!ltm_wire_write_u32(&payload, (uint32_t)ie.pos))
      goto fail_ie;
    if (!ltm_wire_writer_ensure(&payload, ie.pos))
      goto fail_ie;
    memcpy(payload.buf + payload.pos, ie.buf, ie.pos);
    payload.pos += ie.pos;
    free(ie.buf);
    num_tags++;
  }

  if (req->ltm_configuration_id_mapping_list) {
    if (!ltm_wire_write_u16(&payload, F1AP_LTM_WIRE_TAG_CONFIGURATION_ID_MAPPING_LIST))
      goto fail;
    ltm_wire_writer_t ie = {0};
    if (!ltm_wire_encode_configuration_id_mapping_list(&ie, req->ltm_configuration_id_mapping_list))
      goto fail_ie;
    if (!ltm_wire_write_u32(&payload, (uint32_t)ie.pos))
      goto fail_ie;
    if (!ltm_wire_writer_ensure(&payload, ie.pos))
      goto fail_ie;
    memcpy(payload.buf + payload.pos, ie.buf, ie.pos);
    payload.pos += ie.pos;
    free(ie.buf);
    num_tags++;
  }

  return ltm_wire_finalize(&payload, num_tags);
fail_ie:
  free(payload.buf);
fail:
  return NULL;
}

bool f1ap_ltm_wire_decode_ue_ctx_setup_req_ltm(const byte_array_t *ba, f1ap_ue_context_setup_req_t *req)
{
  if (!ba || !req || !f1ap_ltm_wire_is_ltm_container(ba))
    return false;

  ltm_wire_reader_t r = {.buf = ba->buf, .len = ba->len};
  uint16_t num_tags = 0;
  if (!ltm_wire_parse_header(&r, &num_tags))
    return false;

  for (uint16_t t = 0; t < num_tags; ++t) {
    uint16_t tag = 0;
    uint32_t ie_len = 0;
    if (!ltm_wire_read_u16(&r, &tag) || !ltm_wire_read_u32(&r, &ie_len))
      return false;
    if (r.pos + ie_len > r.len)
      return false;
    ltm_wire_reader_t ie = {.buf = r.buf + r.pos, .len = ie_len};
    r.pos += ie_len;

    switch (tag) {
      case F1AP_LTM_WIRE_TAG_INFORMATION_SETUP:
        if (!ltm_wire_decode_information_setup(&ie, &req->ltm_information_setup))
          return false;
        break;
      case F1AP_LTM_WIRE_TAG_CONFIGURATION_ID_MAPPING_LIST:
        if (!ltm_wire_decode_configuration_id_mapping_list(&ie, &req->ltm_configuration_id_mapping_list))
          return false;
        break;
      default:
        break;
    }
  }
  return true;
}

byte_array_t *f1ap_ltm_wire_encode_ue_ctx_setup_resp_ltm(const f1ap_ue_context_setup_resp_t *resp)
{
  if (!resp)
    return NULL;
  if (!resp->ltm_configuration && !resp->early_ul_sync_configuration && !resp->requested_target_cell_id)
    return NULL;

  ltm_wire_writer_t payload = {0};
  uint16_t num_tags = 0;

  if (resp->ltm_configuration) {
    if (!ltm_wire_write_u16(&payload, F1AP_LTM_WIRE_TAG_LTM_CONFIGURATION))
      goto fail;
    ltm_wire_writer_t ie = {0};
    if (!ltm_wire_encode_ltm_configuration(&ie, resp->ltm_configuration))
      goto fail_ie;
    if (!ltm_wire_write_u32(&payload, (uint32_t)ie.pos))
      goto fail_ie;
    if (!ltm_wire_writer_ensure(&payload, ie.pos))
      goto fail_ie;
    memcpy(payload.buf + payload.pos, ie.buf, ie.pos);
    payload.pos += ie.pos;
    free(ie.buf);
    num_tags++;
  }

  if (resp->early_ul_sync_configuration) {
    if (!ltm_wire_write_u16(&payload, F1AP_LTM_WIRE_TAG_EARLY_UL_SYNC_CONFIGURATION))
      goto fail;
    ltm_wire_writer_t ie = {0};
    if (!ltm_wire_encode_early_ul_sync_configuration(&ie, resp->early_ul_sync_configuration))
      goto fail_ie;
    if (!ltm_wire_write_u32(&payload, (uint32_t)ie.pos))
      goto fail_ie;
    if (!ltm_wire_writer_ensure(&payload, ie.pos))
      goto fail_ie;
    memcpy(payload.buf + payload.pos, ie.buf, ie.pos);
    payload.pos += ie.pos;
    free(ie.buf);
    num_tags++;
  }

  if (resp->requested_target_cell_id) {
    if (!ltm_wire_write_u16(&payload, F1AP_LTM_WIRE_TAG_REQUESTED_TARGET_CELL_ID))
      goto fail;
    ltm_wire_writer_t ie = {0};
    if (!ltm_wire_write_u64(&ie, *resp->requested_target_cell_id))
      goto fail_ie;
    if (!ltm_wire_write_u32(&payload, (uint32_t)ie.pos))
      goto fail_ie;
    if (!ltm_wire_writer_ensure(&payload, ie.pos))
      goto fail_ie;
    memcpy(payload.buf + payload.pos, ie.buf, ie.pos);
    payload.pos += ie.pos;
    free(ie.buf);
    num_tags++;
  }

  return ltm_wire_finalize(&payload, num_tags);
fail_ie:
  free(payload.buf);
fail:
  return NULL;
}

bool f1ap_ltm_wire_decode_ue_ctx_setup_resp_ltm(const byte_array_t *ba, f1ap_ue_context_setup_resp_t *resp)
{
  if (!ba || !resp || !f1ap_ltm_wire_is_ltm_container(ba))
    return false;

  ltm_wire_reader_t r = {.buf = ba->buf, .len = ba->len};
  uint16_t num_tags = 0;
  if (!ltm_wire_parse_header(&r, &num_tags))
    return false;

  for (uint16_t t = 0; t < num_tags; ++t) {
    uint16_t tag = 0;
    uint32_t ie_len = 0;
    if (!ltm_wire_read_u16(&r, &tag) || !ltm_wire_read_u32(&r, &ie_len))
      return false;
    if (r.pos + ie_len > r.len)
      return false;
    ltm_wire_reader_t ie = {.buf = r.buf + r.pos, .len = ie_len};
    r.pos += ie_len;

    switch (tag) {
      case F1AP_LTM_WIRE_TAG_LTM_CONFIGURATION:
        if (!ltm_wire_decode_ltm_configuration(&ie, &resp->ltm_configuration))
          return false;
        break;
      case F1AP_LTM_WIRE_TAG_EARLY_UL_SYNC_CONFIGURATION:
        if (!ltm_wire_decode_early_ul_sync_configuration(&ie, &resp->early_ul_sync_configuration))
          return false;
        break;
      case F1AP_LTM_WIRE_TAG_REQUESTED_TARGET_CELL_ID: {
        uint64_t cell_id = 0;
        if (!ltm_wire_read_u64(&ie, &cell_id))
          return false;
        resp->requested_target_cell_id = malloc_or_fail(sizeof(*resp->requested_target_cell_id));
        *resp->requested_target_cell_id = cell_id;
        } break;
      default:
        break;
    }
  }
  return true;
}

void f1ap_ltm_free_reference_configuration_information(f1ap_reference_configuration_information_t *rci)
{
  if (!rci)
    return;
  FREE_OPT_BYTE_ARRAY(rci->cellGroupConfig);
  FREE_OPT_BYTE_ARRAY(rci->measurementTimingConfiguration);
}

f1ap_reference_configuration_information_t cp_f1ap_ltm_reference_configuration_information(
    const f1ap_reference_configuration_information_t *orig)
{
  f1ap_reference_configuration_information_t cp = {0};
  if (orig) {
    CP_OPT_BYTE_ARRAY(cp.cellGroupConfig, orig->cellGroupConfig);
    CP_OPT_BYTE_ARRAY(cp.measurementTimingConfiguration, orig->measurementTimingConfiguration);
  }
  return cp;
}

bool eq_f1ap_ltm_reference_configuration_information(const f1ap_reference_configuration_information_t *a,
                                                     const f1ap_reference_configuration_information_t *b)
{
  return ltm_eq_ba_ptr(a ? a->cellGroupConfig : NULL, b ? b->cellGroupConfig : NULL)
         && ltm_eq_ba_ptr(a ? a->measurementTimingConfiguration : NULL, b ? b->measurementTimingConfiguration : NULL);
}

void f1ap_ltm_free_reference_configuration(f1ap_reference_configuration_t *rc)
{
  if (!rc)
    return;
  if (rc->reference_configuration_information)
    f1ap_ltm_free_reference_configuration_information(rc->reference_configuration_information);
  free(rc->reference_configuration_information);
  free(rc);
}

f1ap_reference_configuration_t *cp_f1ap_ltm_reference_configuration(const f1ap_reference_configuration_t *orig)
{
  if (!orig)
    return NULL;
  f1ap_reference_configuration_t *cp = calloc_or_fail(1, sizeof(*cp));
  cp->request_for_lower_layer_configuration_present = orig->request_for_lower_layer_configuration_present;
  cp->request_for_lower_layer_configuration = orig->request_for_lower_layer_configuration;
  if (orig->reference_configuration_information) {
    cp->reference_configuration_information = calloc_or_fail(1, sizeof(*cp->reference_configuration_information));
    *cp->reference_configuration_information =
        cp_f1ap_ltm_reference_configuration_information(orig->reference_configuration_information);
  }
  return cp;
}

bool eq_f1ap_ltm_reference_configuration(const f1ap_reference_configuration_t *a,
                                         const f1ap_reference_configuration_t *b)
{
  if (!a && !b)
    return true;
  if (!a || !b)
    return false;
  if (a->request_for_lower_layer_configuration_present != b->request_for_lower_layer_configuration_present)
    return false;
  if (a->request_for_lower_layer_configuration_present
      && a->request_for_lower_layer_configuration != b->request_for_lower_layer_configuration)
    return false;
  if (!!a->reference_configuration_information != !!b->reference_configuration_information)
    return false;
  if (a->reference_configuration_information
      && !eq_f1ap_ltm_reference_configuration_information(a->reference_configuration_information,
                                                        b->reference_configuration_information))
    return false;
  return true;
}

void f1ap_ltm_free_csi_resource_configuration(f1ap_csi_resource_configuration_t *crc)
{
  if (!crc)
    return;
  FREE_OPT_BYTE_ARRAY(crc->configuration);
  free(crc);
}

f1ap_csi_resource_configuration_t *cp_f1ap_ltm_csi_resource_configuration(const f1ap_csi_resource_configuration_t *orig)
{
  if (!orig)
    return NULL;
  f1ap_csi_resource_configuration_t *cp = calloc_or_fail(1, sizeof(*cp));
  CP_OPT_BYTE_ARRAY(cp->configuration, orig->configuration);
  return cp;
}

bool eq_f1ap_ltm_csi_resource_configuration(const f1ap_csi_resource_configuration_t *a,
                                            const f1ap_csi_resource_configuration_t *b)
{
  return ltm_eq_ba_ptr(a ? a->configuration : NULL, b ? b->configuration : NULL);
}

void f1ap_ltm_free_ltm_configuration(f1ap_ltm_configuration_t *lc)
{
  if (!lc)
    return;
  f1ap_ltm_free_reference_configuration(lc->reference_configuration);
  f1ap_ltm_free_csi_resource_configuration(lc->csi_resource_configuration);
  free(lc);
}

f1ap_ltm_configuration_t *cp_f1ap_ltm_ltm_configuration(const f1ap_ltm_configuration_t *orig)
{
  if (!orig)
    return NULL;
  f1ap_ltm_configuration_t *cp = calloc_or_fail(1, sizeof(*cp));
  cp->reference_configuration = cp_f1ap_ltm_reference_configuration(orig->reference_configuration);
  cp->csi_resource_configuration = cp_f1ap_ltm_csi_resource_configuration(orig->csi_resource_configuration);
  return cp;
}

bool eq_f1ap_ltm_ltm_configuration(const f1ap_ltm_configuration_t *a, const f1ap_ltm_configuration_t *b)
{
  return eq_f1ap_ltm_reference_configuration(a ? a->reference_configuration : NULL, b ? b->reference_configuration : NULL)
         && eq_f1ap_ltm_csi_resource_configuration(a ? a->csi_resource_configuration : NULL,
                                                   b ? b->csi_resource_configuration : NULL);
}

void f1ap_ltm_free_configuration_id_mapping_list(f1ap_ltm_configuration_id_mapping_list_t *list)
{
  if (!list)
    return;
  for (int i = 0; i < list->len; ++i) {
    f1ap_ltm_configuration_t *lc = &list->items[i].ltm_configuration;
    f1ap_ltm_free_reference_configuration(lc->reference_configuration);
    lc->reference_configuration = NULL;
    f1ap_ltm_free_csi_resource_configuration(lc->csi_resource_configuration);
    lc->csi_resource_configuration = NULL;
    free(list->items[i].candidate_cell_id);
    list->items[i].candidate_cell_id = NULL;
  }
  free(list->items);
  free(list);
}

f1ap_ltm_configuration_id_mapping_list_t *cp_f1ap_ltm_configuration_id_mapping_list(
    const f1ap_ltm_configuration_id_mapping_list_t *orig)
{
  if (!orig || orig->len == 0)
    return NULL;
  f1ap_ltm_configuration_id_mapping_list_t *cp = calloc_or_fail(1, sizeof(*cp));
  cp->len = orig->len;
  cp->items = calloc_or_fail(orig->len, sizeof(*cp->items));
  for (int i = 0; i < orig->len; ++i) {
    cp->items[i].ltm_configuration_id = orig->items[i].ltm_configuration_id;
    if (orig->items[i].candidate_cell_id)
      _F1_MALLOC(cp->items[i].candidate_cell_id, *orig->items[i].candidate_cell_id);
    f1ap_ltm_configuration_t *lc = cp_f1ap_ltm_ltm_configuration(&orig->items[i].ltm_configuration);
    if (lc) {
      cp->items[i].ltm_configuration = *lc;
      free(lc);
    }
  }
  return cp;
}

bool eq_f1ap_ltm_configuration_id_mapping_list(const f1ap_ltm_configuration_id_mapping_list_t *a,
                                               const f1ap_ltm_configuration_id_mapping_list_t *b)
{
  if (!a && !b)
    return true;
  if (!a || !b || a->len != b->len)
    return false;
  for (int i = 0; i < a->len; ++i) {
    if (a->items[i].ltm_configuration_id != b->items[i].ltm_configuration_id)
      return false;
    _F1_EQ_CHECK_OPTIONAL_IE(&a->items[i], &b->items[i], candidate_cell_id, _F1_EQ_CHECK_LONG);
    if (!eq_f1ap_ltm_ltm_configuration(&a->items[i].ltm_configuration, &b->items[i].ltm_configuration))
      return false;
  }
  return true;
}

void f1ap_ltm_free_information_setup(f1ap_ltm_information_setup_t *setup)
{
  free(setup);
}

f1ap_ltm_information_setup_t *cp_f1ap_ltm_information_setup(const f1ap_ltm_information_setup_t *orig)
{
  if (!orig)
    return NULL;
  f1ap_ltm_information_setup_t *cp = calloc_or_fail(1, sizeof(*cp));
  *cp = *orig;
  return cp;
}

bool eq_f1ap_ltm_information_setup(const f1ap_ltm_information_setup_t *a, const f1ap_ltm_information_setup_t *b)
{
  if (!a && !b)
    return true;
  if (!a || !b)
    return false;
  return a->setup_indication == b->setup_indication;
}

void f1ap_ltm_free_early_ul_sync_configuration(f1ap_early_ul_sync_configuration_t *cfg)
{
  free(cfg);
}

f1ap_early_ul_sync_configuration_t *cp_f1ap_ltm_early_ul_sync_configuration(const f1ap_early_ul_sync_configuration_t *orig)
{
  if (!orig)
    return NULL;
  f1ap_early_ul_sync_configuration_t *cp = calloc_or_fail(1, sizeof(*cp));
  cp->prachConfigurationIndex = orig->prachConfigurationIndex;
  cp->prachFrequencyOffset = orig->prachFrequencyOffset;
  return cp;
}

bool eq_f1ap_ltm_early_ul_sync_configuration(const f1ap_early_ul_sync_configuration_t *a,
                                             const f1ap_early_ul_sync_configuration_t *b)
{
  if (!a && !b)
    return true;
  if (!a || !b)
    return false;
  return a->prachConfigurationIndex == b->prachConfigurationIndex && a->prachFrequencyOffset == b->prachFrequencyOffset;
}

void f1ap_ltm_free_ue_context_setup_req_ltm(f1ap_ue_context_setup_req_t *req)
{
  if (!req)
    return;
  f1ap_ltm_free_information_setup(req->ltm_information_setup);
  req->ltm_information_setup = NULL;
  f1ap_ltm_free_configuration_id_mapping_list(req->ltm_configuration_id_mapping_list);
  req->ltm_configuration_id_mapping_list = NULL;
}

void f1ap_ltm_free_ue_context_setup_resp_ltm(f1ap_ue_context_setup_resp_t *resp)
{
  if (!resp)
    return;
  f1ap_ltm_free_ltm_configuration(resp->ltm_configuration);
  resp->ltm_configuration = NULL;
  f1ap_ltm_free_early_ul_sync_configuration(resp->early_ul_sync_configuration);
  resp->early_ul_sync_configuration = NULL;
  free(resp->requested_target_cell_id);
  resp->requested_target_cell_id = NULL;
}
