/**
 * @file
 * @brief Handler for Command Class User Code.
 * @copyright 2018 Silicon Laboratories Inc.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_TransportLayer.h>

#include "config_app.h"
#include <CC_UserCode.h>
#include <string.h>

//#define DEBUGPRINT
#include "DebugPrint.h"
#include <ZAF_TSE.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

typedef struct
{
  uint8_t userIdentifier;
  uint8_t userIdStatus;
  uint8_t userCode[10];
}
user_code_report_t;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

s_CC_userCode_data_t userCodeData;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


received_frame_status_t
CC_UserCode_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER * pTxBuf = &(TxBuf.appTxBuf);
  switch (pCmd->ZW_Common.cmd)
  {
    case USER_CODE_GET:
      if(true == Check_not_legal_response_job(rxOpt))
      {
        /*Get/Report do not support endpoint bit-addressing */
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

      uint8_t len = 0;
      TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
      RxToTxOptions(rxOpt, &pTxOptionsEx);

      pTxBuf->ZW_UserCodeReport1byteFrame.cmdClass = COMMAND_CLASS_USER_CODE;
      pTxBuf->ZW_UserCodeReport1byteFrame.cmd = USER_CODE_REPORT;
      pTxBuf->ZW_UserCodeReport1byteFrame.userIdentifier = pCmd->ZW_UserCodeGetFrame.userIdentifier;

      CC_UserCode_Report_handler(
          pCmd->ZW_UserCodeGetFrame.userIdentifier,
          &(pTxBuf->ZW_UserCodeReport1byteFrame.userCode1),
          &len,
          &(pTxBuf->ZW_UserCodeReport1byteFrame.userIdStatus),
          rxOpt->destNode.endpoint);

      if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                  (uint8_t *)pTxBuf,
                  sizeof(ZW_USER_CODE_REPORT_1BYTE_FRAME) + len - 1,
                  pTxOptionsEx,
                  NULL))
      {
        return RECEIVED_FRAME_STATUS_FAIL; /*failing*/
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;

      break;

    case USER_CODE_SET:
      {
        bool allUserCodesRemoved = false;
        e_cmd_handler_return_code_t return_code = CC_UserCode_Set_handler(pCmd->ZW_UserCodeSet1byteFrame.userIdentifier,
                                              pCmd->ZW_UserCodeSet1byteFrame.userIdStatus,
                                              &(pCmd->ZW_UserCodeSet1byteFrame.userCode1),
                                              cmdLength - 4,
                                              &allUserCodesRemoved);

        if (E_CMD_HANDLER_RETURN_CODE_HANDLED == return_code)
        {
            if (allUserCodesRemoved)
            {
                memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

                TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
                RxToTxOptions(rxOpt, &pTxOptionsEx);

                pTxBuf->ZW_UserCodeReport1byteFrame.cmdClass = COMMAND_CLASS_USER_CODE;
                pTxBuf->ZW_UserCodeReport1byteFrame.cmd = USER_CODE_REPORT;
                pTxBuf->ZW_UserCodeReport1byteFrame.userIdentifier = 0;
                pTxBuf->ZW_UserCodeReport1byteFrame.userIdStatus = USER_ID_AVAILBLE;

                Transport_SendResponseEP(
                                 (uint8_t *)pTxBuf,
                                 sizeof(ZW_USER_CODE_REPORT_1BYTE_FRAME) - 1,
                                 pTxOptionsEx,
                                 NULL);
            }

            return RECEIVED_FRAME_STATUS_SUCCESS;
        }

        return RECEIVED_FRAME_STATUS_FAIL;
      }
      break;

    case USERS_NUMBER_GET:
      if(false == Check_not_legal_response_job(rxOpt))
      {
    	  memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_UsersNumberReportV2Frame.cmdClass = COMMAND_CLASS_USER_CODE;
        pTxBuf->ZW_UsersNumberReportV2Frame.cmd = USERS_NUMBER_REPORT;
        const uint16_t supportedUsers = CC_UserCode_UsersNumberReport_handler( rxOpt->destNode.endpoint );
        pTxBuf->ZW_UsersNumberReportV2Frame.supportedUsers = supportedUsers > 0xFF ? 0xFF : (uint8_t)supportedUsers;
        pTxBuf->ZW_UsersNumberReportV2Frame.extendedSupportedUsers1 = supportedUsers >> 8;
        pTxBuf->ZW_UsersNumberReportV2Frame.extendedSupportedUsers2 = supportedUsers & 0xFF;

        if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                      (uint8_t *)pTxBuf,
                      sizeof(ZW_USERS_NUMBER_REPORT_V2_FRAME),
                      pTxOptionsEx,
                      NULL))
        {
          /*Job failed */
          return RECEIVED_FRAME_STATUS_FAIL;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case USER_CODE_CAPABILITIES_GET_V2:
      if(false == Check_not_legal_response_job(rxOpt))
      {
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        const uint8_t supportedStatusesBitmask[] = SUPPORTED_STATUSES;
        const uint8_t keypadModesBitmask[] = SUPPORTED_KEYPAD_MODES;

        uint8_t keysBitmaskLength = 0;
        uint8_t keysBitmask[128 / 8];
        memset(keysBitmask, 0, sizeof(keysBitmask));
        for (const char* s = SUPPORTED_KEYS; *s; ++s)
        {
            if (*s > 0)
            {
                const uint8_t byte = *s / 8;
                const uint8_t bit = *s % 8;
                keysBitmask[byte] |= 1 << bit;
                if (byte >= keysBitmaskLength)
                  keysBitmaskLength = byte + 1;
            }
        }

        uint8_t* buf = (uint8_t*)pTxBuf;
        buf[0] = COMMAND_CLASS_USER_CODE;
        buf[1] = USER_CODE_CAPABILITIES_REPORT_V2;
        buf += 2;
        {
          buf[0] = ( ( MASTER_CODE_SUPPORTED ? 1 : 0 ) << 7 ) |
                   ( ( MASTER_CODE_DEACTIVATION_SUPPORTED ? 1 : 0 ) << 6 ) |
                   sizeof(supportedStatusesBitmask);
          memcpy(buf + 1, &supportedStatusesBitmask, sizeof(supportedStatusesBitmask));
          buf += 1 + sizeof(supportedStatusesBitmask);

          buf[0] = ((CHECKSUM_SUPPORTED ? 1 : 0) << 7) |
                   ((MULTIPLE_REPORT_SUPPORTED ? 1 : 0) << 6) |
                   ((MULTIPLE_SET_SUPPORTED ? 1 : 0) << 5) |
                   sizeof(keypadModesBitmask);
          memcpy(buf + 1, &keypadModesBitmask, sizeof(keypadModesBitmask));
          buf += 1 + sizeof(keypadModesBitmask);

          buf[0] = keysBitmaskLength;
          memcpy(buf + 1, &keysBitmask, keysBitmaskLength);
          buf += 1 + keysBitmaskLength;
        }

        if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                      (uint8_t *)pTxBuf,
                      buf - (uint8_t *)pTxBuf,
                      pTxOptionsEx,
                      NULL))
        {
          /*Job failed */
          return RECEIVED_FRAME_STATUS_FAIL;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case USER_CODE_KEYPAD_MODE_GET_V2:
      if(false == Check_not_legal_response_job(rxOpt))
      {
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_UserCodeKeypadModeReportV2Frame.cmdClass = COMMAND_CLASS_USER_CODE;
        pTxBuf->ZW_UserCodeKeypadModeReportV2Frame.cmd = USER_CODE_KEYPAD_MODE_REPORT_V2;
        pTxBuf->ZW_UserCodeKeypadModeReportV2Frame.keypadMode = CC_UserCode_KeypadModeReport_handler();

        if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                      (uint8_t *)pTxBuf,
                      sizeof(ZW_USER_CODE_KEYPAD_MODE_REPORT_V2_FRAME),
                      pTxOptionsEx,
                      NULL))
        {
          /*Job failed */
          return RECEIVED_FRAME_STATUS_FAIL;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case USER_CODE_KEYPAD_MODE_SET_V2:
      {
        CC_UserCode_KeypadModeSet_handler(pCmd->ZW_UserCodeKeypadModeSetV2Frame.keypadMode);
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      break;

    case MASTER_CODE_GET_V2:
      if(false == Check_not_legal_response_job(rxOpt))
      {
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_MasterCodeReport1byteV2Frame.cmdClass = COMMAND_CLASS_USER_CODE;
        pTxBuf->ZW_MasterCodeReport1byteV2Frame.cmd = MASTER_CODE_REPORT_V2;

        uint8_t len = 0;
        CC_UserCode_MasterCodeReport_handler(&pTxBuf->ZW_MasterCodeReport1byteV2Frame.masterCode1, &len);

        pTxBuf->ZW_MasterCodeReport1byteV2Frame.properties1 = len;
        if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                      (uint8_t *)pTxBuf,
                      3 + len,
                      pTxOptionsEx,
                      NULL))
        {
          /*Job failed */
          return RECEIVED_FRAME_STATUS_FAIL;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case MASTER_CODE_SET_V2:
      {
        CC_UserCode_MasterCodeSet_handler(&pCmd->ZW_MasterCodeSet1byteV2Frame.masterCode1, pCmd->ZW_MasterCodeSet1byteV2Frame.properties1 & 0x0F);
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      break;

    case USER_CODE_CHECKSUM_GET_V2:
      if(false == Check_not_legal_response_job(rxOpt))
      {
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_UserCodeChecksumReportV2Frame.cmdClass = COMMAND_CLASS_USER_CODE;
        pTxBuf->ZW_UserCodeChecksumReportV2Frame.cmd = USER_CODE_CHECKSUM_REPORT_V2;

        const uint16_t checksum = CC_UserCode_ChecksumReport_handler();

        pTxBuf->ZW_UserCodeChecksumReportV2Frame.userCodeChecksum1 = checksum >> 8;
        pTxBuf->ZW_UserCodeChecksumReportV2Frame.userCodeChecksum2 = checksum & 0xFF;

        if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                      (uint8_t *)pTxBuf,
                      sizeof(ZW_USER_CODE_CHECKSUM_REPORT_V2_FRAME),
                      pTxOptionsEx,
                      NULL))
        {
          /*Job failed */
          return RECEIVED_FRAME_STATUS_FAIL;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case EXTENDED_USER_CODE_GET_V2:
      if(false == Check_not_legal_response_job(rxOpt))
      {
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        const uint16_t userId = (pCmd->ZW_ExtendedUserCodeGetV2Frame.userIdentifier1 << 8) | pCmd->ZW_ExtendedUserCodeGetV2Frame.userIdentifier2;
        const bool reportMore = pCmd->ZW_ExtendedUserCodeGetV2Frame.properties1 & 1;

        uint8_t* buf = (uint8_t*)pTxBuf;

        uint16_t dataLen = 0;

        buf[0] = COMMAND_CLASS_USER_CODE;
        buf[1] = EXTENDED_USER_CODE_REPORT_V2;

        CC_UserCode_ExtendedReport_handler(userId, buf + 2, &dataLen, 33, reportMore);

        if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                      (uint8_t *)pTxBuf,
                      dataLen + 2,
                      pTxOptionsEx,
                      NULL))
        {
          /*Job failed */
          return RECEIVED_FRAME_STATUS_FAIL;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case EXTENDED_USER_CODE_SET_V2:
      CC_UserCode_ExtendedSet_handler(((uint8_t*)pCmd) + 2, cmdLength - 2);
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}


JOB_STATUS CC_UserCode_SupportReport(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  uint8_t userIdentifier,
  uint8_t userIdStatus,
  uint8_t* pUserCode,
  uint8_t userCodeLen,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  CMD_CLASS_GRP cmdGrp = {COMMAND_CLASS_USER_CODE, USER_CODE_REPORT};
  user_code_report_t user_code_report;

  if ((0 == userIdentifier) || IS_NULL(pUserCode) ||
      (userCodeLen > USERCODE_MAX_LEN) || (userCodeLen < USERCODE_MIN_LEN) ||
      (5 < userIdStatus))
  {
    return JOB_STATUS_BUSY;
  }

  user_code_report.userIdentifier = userIdentifier;
  user_code_report.userIdStatus = userIdStatus;
  memcpy(user_code_report.userCode, pUserCode, userCodeLen);

  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      (uint8_t*)&user_code_report,
      sizeof(user_code_report_t) + userCodeLen - USERCODE_MAX_LEN,
      false,
      pCallback);
}

void CC_UserCode_report_stx(
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions,
    s_CC_userCode_data_t* pData)
{
  DPRINTF("* %s() *\n"
      "\ttxOpt.src = %d\n"
      "\ttxOpt.options %#02x\n"
      "\ttxOpt.secOptions %#02x\n",
      __func__, txOptions.sourceEndpoint, txOptions.txOptions, txOptions.txSecOptions);

  /* Prepare payload for report */
  uint8_t len = 0;
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

  pTxBuf->ZW_UserCodeReport1byteFrame.cmdClass = COMMAND_CLASS_USER_CODE;
  pTxBuf->ZW_UserCodeReport1byteFrame.cmd = USER_CODE_REPORT;
  pTxBuf->ZW_UserCodeReport1byteFrame.userIdentifier = pData->userIdentifier;

  if(false == CC_UserCode_Report_handler(
      pData->userIdentifier,
      &(pTxBuf->ZW_UserCodeReport1byteFrame.userCode1),
      &len,
      &(pTxBuf->ZW_UserCodeReport1byteFrame.userIdStatus),
      pData->rxOptions.destNode.endpoint))
  {
    /*Job failed */
    DPRINTF("%s(): CC_UserCode_Report_handler() failed. \n", __func__);
    return;
  }

  if (EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendRequestEP((uint8_t *)pTxBuf,
                                                                  sizeof(ZW_USER_CODE_REPORT_1BYTE_FRAME) + len - 1,
                                                                  &txOptions,
                                                                  ZAF_TSE_TXCallback))
  {
    //sending request failed
    DPRINTF("%s(): Transport_SendRequestEP() failed. \n", __func__);
  }
}

REGISTER_CC(COMMAND_CLASS_USER_CODE, USER_CODE_VERSION_V2, CC_UserCode_handler);
