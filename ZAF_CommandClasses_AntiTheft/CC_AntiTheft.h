#ifndef _ANTI_THEFT_H_
#define _ANTI_THEFT_H_

#include <ZAF_types.h>
#include <ZW_classcmd.h>
#include <CC_Common.h>
#include <ZW_TransportEndpoint.h>

#include <stdint.h>
#include <stdbool.h>

typedef struct SAntiTheftInfo {
  bool locked : 1;
  bool restricted : 1;
  uint16_t manufacturerId;
  uint16_t lockingId;
  uint8_t hintLength;
  uint8_t hint[10];
  uint8_t magicCodeLength;
  uint8_t magicCode[10];
} SAntiTheftInfo;

received_frame_status_t CC_AntiTheft_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);

void setAntiTheftInfo( const SAntiTheftInfo* info );

bool getAntiTheftInfo( SAntiTheftInfo* info );

#endif
