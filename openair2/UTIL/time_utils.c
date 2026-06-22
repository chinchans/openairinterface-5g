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

#include "time_utils.h"

#include <stdio.h>
#include <time.h>

void get_current_timestamp(char *buffer, size_t buffer_size)
{
  if (buffer == NULL || buffer_size == 0) {
    return;
  }

  time_t now = time(NULL);
  struct tm tm_buf;
  if (localtime_r(&now, &tm_buf) != NULL) {
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &tm_buf);
  } else {
    snprintf(buffer, buffer_size, "%ld", (long)now);
  }
}
