#include "../ZAF_CommandClasses_AntiTheft/CC_AntiTheft.h"

#include <ZW_TransportLayer.h>

#include "config_app.h"
#include <string.h>

//#define DEBUGPRINT
#include "DebugPrint.h"
#include <ZAF_TSE.h>
#include <stdlib.h>

void updateNodeInfo();

received_frame_status_t CC_AntiTheft_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
  UNUSED(cmdLength);

  switch (pCmd->ZW_Common.cmd)
  {

    case ANTITHEFT_SET:
      {
        SAntiTheftInfo info;
        if ( !getAntiTheftInfo(&info) ) {
            return RECEIVED_FRAME_STATUS_FAIL;
        }

        const uint8_t* frame = pCmd->data;
        const bool lock = frame[2] >> 7;

        if ( lock && !info.locked ) {
            memset(&info, 0, sizeof(info));

            info.locked = true;
            info.magicCodeLength = frame[2] & 0x7F;

            if ( info.magicCodeLength < 1 || info.magicCodeLength > 10 ) {
                return RECEIVED_FRAME_STATUS_FAIL;
            }

            frame += 3;
            memcpy(info.magicCode, frame, info.magicCodeLength);

            frame += info.magicCodeLength;
            info.manufacturerId = ( frame[ 0 ] << 8 ) | frame[ 1 ];

            frame += 2;
            info.hintLength = frame[ 0 ];

            if ( info.hintLength < 1 || info.hintLength > 10 ) {
                return RECEIVED_FRAME_STATUS_FAIL;
            }

            frame += 1;
            memcpy(info.hint, frame, info.hintLength);

            frame += info.hintLength;
            if ( cmdLength < ( frame + 2 ) - pCmd->data ) {
                return RECEIVED_FRAME_STATUS_FAIL;
            }

            info.lockingId = ( frame[0] << 8 ) | frame[1];

            if ( info.lockingId == 0 ) {
                return RECEIVED_FRAME_STATUS_FAIL;
            }

            setAntiTheftInfo( &info );
        }
        else if ( !lock && info.locked ) {
            const uint8_t codeLen = frame[2] & 0x7F;

            if ( codeLen == info.magicCodeLength && memcmp(info.magicCode, frame + 3, codeLen) == 0 ) {
                memset(&info, 0, sizeof(info));
                setAntiTheftInfo( &info );

                updateNodeInfo();
            }
        }
        else {
            return RECEIVED_FRAME_STATUS_FAIL;
        }

        break;
      }

    case ANTITHEFT_GET:
      {
        SAntiTheftInfo info;
        if ( !getAntiTheftInfo(&info) ) {
            return RECEIVED_FRAME_STATUS_FAIL;
        }

        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        uint8_t* frame = pTxBuf->data;
        frame[0] = COMMAND_CLASS_ANTITHEFT;
        frame[1] = ANTITHEFT_REPORT;
        frame[2] = !info.locked ? 1 : !info.restricted ? 2 : 3;

        frame[3] = info.manufacturerId >> 8;
        frame[4] = info.manufacturerId & 0xFF;

        frame[5] = info.hintLength;
        frame += 6;
        memcpy( frame, info.hint, info.hintLength );
        frame += info.hintLength;

        frame[0] = info.lockingId >> 8;
        frame[1] = info.lockingId & 0xFF;

        frame += 2;

        if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
            pTxBuf->data,
            frame - pTxBuf->data,
            pTxOptionsEx,
            NULL))
        {
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        break;
      }
    default:
      return RECEIVED_FRAME_STATUS_NO_SUPPORT;
  }
  return RECEIVED_FRAME_STATUS_SUCCESS;
}


REGISTER_CC(COMMAND_CLASS_ANTITHEFT, 3, CC_AntiTheft_handler);
