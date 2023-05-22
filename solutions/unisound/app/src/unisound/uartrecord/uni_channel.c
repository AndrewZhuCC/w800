/**************************************************************************
 * Copyright (C) 2020-2020  Unisound
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : uni_channel.c
 * Author      : shangjinlong.unisound.com
 * Date        : 2020.03.10
 *
 **************************************************************************/
#include "uni_channel.h"
#include "uni_log.h"
#include "uni_communication.h"
#include "uni_lasr_service.h"

#define TAG     "channel"

int ChnlChallengePackRequest(ChnlChallengePackReq *request) {
  CommAttribute attr;
  attr.reliable = true;
  return CommProtocolPacketAssembleAndSend(request->type,
                                           (char *)request,
                                           sizeof(*request),
                                           &attr);
}

int ChnlChallengePackResponse(ChnlChallengePackAck *response) {
  CommAttribute attr;
  attr.reliable = true;
  return CommProtocolPacketAssembleAndSend(response->type,
                                           (char *)response,
                                           sizeof(*response),
                                           &attr);
}

int ChnlNetworkStatusRequest(ChnlNetworkStatusReq *request) {
  CommAttribute attr;
  attr.reliable = true;
  return CommProtocolPacketAssembleAndSend(request->type,
                                           (char *)request,
                                           sizeof(*request),
                                           &attr);
}

int ChnlNetworkStatusResponse(ChnlNetworkStatusResp *response) {
  CommAttribute attr;
  attr.reliable = true;
  return CommProtocolPacketAssembleAndSend(response->type,
                                           (char *)response,
                                           sizeof(*response),
                                           &attr);
}

int ChnlAwakenRequest(ChnlAwakenReq *request) {
  CommAttribute attr;
  attr.reliable = true;
  return CommProtocolPacketAssembleAndSend(request->type,
                                           (char *)request,
                                           sizeof(*request),
                                           &attr);
}

int ChnlRecognizeRequest(ChnlRecognizeReq *request) {
  CommAttribute attr;
  attr.reliable = true;
  return CommProtocolPacketAssembleAndSend(request->type,
                                           (char *)request,
                                           sizeof(*request),
                                           &attr);
}

int ChnlPullNoiseReductionDataRequest(ChnlPullNoiseReductionDataReq *request) {
  CommAttribute attr;
  attr.reliable = true;
  return CommProtocolPacketAssembleAndSend(request->type,
                                           (char *)request,
                                           sizeof(*request),
                                           &attr);
}

int ChnlNoiseReductionPcmDataPush(ChnlNoiseReductionPcmData *request) {
  CommAttribute attr;
  attr.reliable = true;
  return CommProtocolPacketAssembleAndSend(request->type,
                                           (char *)request,
                                           sizeof(*request),
                                           &attr);
}

int ChnlOnlineAsrResultResponse(ChnlOnlineAsrResult *response) {
  CommAttribute attr;
  attr.reliable = true;
  return CommProtocolPacketAssembleAndSend(response->type,
                                           (char *)response,
                                           sizeof(*response),
                                           &attr);
}

static int _challenge_pack(char *packet, int len) {
  LOGT(TAG, "receive challenge pack");
  return 0;
}

static int _challenge_pack_ack(char *packet, int len) {
  LOGT(TAG, "receive challenge ack pack");
  return 0;
}

static int _network_status_request(char *packet, int len) {
  LOGT(TAG, "receive network request");
  return 0;
}

static int _network_status_response(char *packet, int len) {
  LOGT(TAG, "receive network response");
  return 0;
}

static int _noise_reduction_raw_data(char *packet, int len) {
  LOGT(TAG, "receive noise reduction raw data");
  return 0;
}

static int _rasr_result(char *packet, int len) {
  LOGT(TAG, "receive rasr result");
  return 0;
}

static int _lasr_awaken_request(char *packet, int len) {
  ChnlAwakenReq *awaken = (ChnlAwakenReq *)packet;
  LOGT(TAG, "receive awaken request, session_id=%d, content=%s",
       awaken->session_id, awaken->content);

  return 0;
}

static int _recognize_request(char *packet, int len) {
  ChnlRecognizeReq *recogn = (ChnlRecognizeReq *)packet;
  LOGT(TAG, "receive recogn request, mode=%d", recogn->mode);
  //LasrAsrRecognSet(recogn->mode);
  return 0;
}

static int _pull_noise_reduction_data_request(char *packet, int len) {
  ChnlPullNoiseReductionDataReq *pull = (ChnlPullNoiseReductionDataReq *)packet;
  LOGT(TAG, "receive pull data request, mode=%d", pull->mode);
  LasrPullRawData(pull->mode);
  return 0;
}

/* interrupt callback, donnot do anything time consuming process */
void ChnlReceiveCommProtocolPacket(CommPacket *packet) {
  switch (packet->cmd) {
    case CHNL_MESSAGE_CHALLENGE_PACK:
      _challenge_pack(packet->payload, packet->payload_len);
      break;
    case CHNL_MESSAGE_CHALLENGE_PACK_ACK:
      _challenge_pack_ack(packet->payload, packet->payload_len);
      break;
    case CHNL_MESSAGE_NETWORK_REQUEST:
      _network_status_request(packet->payload, packet->payload_len);
      break;
    case CHNL_MESSAGE_NETWORK_RESPONSE:
      _network_status_response(packet->payload, packet->payload_len);
      break;
    case CHNL_MESSAGE_NOISE_REDUCTION_RAW_DATA:
      _noise_reduction_raw_data(packet->payload, packet->payload_len);
      break;
    case CHNL_MESSAGE_RASR_RESULT:
      _rasr_result(packet->payload, packet->payload_len);
      break;
    case CHNL_MESSAGE_AWAKEN_REQUEST:
      _lasr_awaken_request(packet->payload, packet->payload_len);
      break;
    case CHNL_MESSAGE_RECOGNIZE_REQUEST:
      _recognize_request(packet->payload, packet->payload_len);
      break;
    case CHNL_MESSAGE_PULL_NOISE_REDUCTION_DATA_REQUEST:
      _pull_noise_reduction_data_request(packet->payload, packet->payload_len);
    break;
    default:
      LOGD(TAG, "unsupport message, type=%d", packet->cmd);
      break;
  }
}

