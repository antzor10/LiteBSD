/*
 * MRF24WG Universal Driver Global Includes
 *
 * This module contains include files needed by the Universal Driver
 */
#ifndef __WF_GLOBAL_INCLUDES_H
#define __WF_GLOBAL_INCLUDES_H

#include "wf_stubs.h"
#include "wf_events.h"
#include "wf_connection_event_codes.h"
#include "wf_mgmt_msg.h"
#include "wf_ud_state.h"
#include "wf_raw.h"
#include "wf_registers.h"
#include <sys/param.h>
#include <sys/systm.h>

/*
 * Connection profile functions.
 */
u_int8_t GetCpid(void);

void SetHiddenSsid(bool hiddenSsid);
void SetAdHocMode(u_int8_t mode);

t_wpaKeyInfo * GetWpsPassPhraseInfo(void);

/*
 * Data Tx/Rx definitions
 */
#define SNAP_VAL        ((u_int8_t)0xaa)
#define SNAP_CTRL_VAL   ((u_int8_t)0x03)
#define SNAP_TYPE_VAL   ((u_int8_t)0x00)

#define SNAP_SIZE       (6)

void SignalPacketRx(void);
bool isPacketRx(void);
void ClearPacketRx(void);
void WF_ProcessWiFiRxData(void);
void RxPacketCheck(void);

/*
 * External interrupt definitions
 */
void InterruptCheck(void);

/*
 * Event queue defintions.
 */
#define MAX_EVENTS  (10)

typedef struct eventStruct
{
    u_int8_t  eventType;
    u_int32_t eventData;
} t_event;

typedef struct eventQueueStruct
{
   u_int8_t writeIndex;
   u_int8_t readIndex;
   t_event  event[MAX_EVENTS + 1]; // one unused slot
} t_wfEventQueue;

/*
 * Parameter message defintions
 */
#define MSG_PARAM_START_DATA_INDEX          (6)
#define MULTICAST_ADDRESS                   (6)
#define ADDRESS_FILTER_DEACTIVATE           (0)

#define ENABLE_MRF24WB0M                    (1)

void WFEnableMRF24WB0MMode(void);
u_int8_t GetFactoryMax(void);
void YieldPassPhraseToHost(void);
void SetPSK(u_int8_t *psk);

/*
 * Defintions for the PLL work-around
 */
void ResetPll(void);

/*
 * Definitions for power control
 */
enum {
    WF_LOW_POWER_MODE_OFF = 0,
    WF_LOW_POWER_MODE_ON  = 1,
};

void WFConfigureLowPowerMode(u_int8_t action);
void EnsureWFisAwake(void);
bool isPsPollNeedReactivate(void);
void ClearPsPollReactivate(void);

/*
 * SPI message type definitions
 */

/* SPI Tx Message Types */
#define WF_DATA_REQUEST_TYPE            ((u_int8_t)1)
#define WF_MGMT_REQUEST_TYPE            ((u_int8_t)2)

/* SPI Rx Message Types */
#define WF_DATA_TX_CONFIRM_TYPE         ((u_int8_t)1)
#define WF_MGMT_CONFIRM_TYPE            ((u_int8_t)2)
#define WF_DATA_RX_INDICATE_TYPE        ((u_int8_t)3)
#define WF_MGMT_INDICATE_TYPE           ((u_int8_t)4)

/* SPI Tx/Rx Data Message Subtypes */
#define WF_STD_DATA_MSG_SUBTYPE         ((u_int8_t)1)
#define WF_NULL_DATA_MSG_SUBTYPE        ((u_int8_t)2)
/* reserved value                       ((u_int8_t)3) */
#define WF_UNTAMPERED_DATA_MSG_SUBTYPE  ((u_int8_t)4)

#define WF_TX_DATA_MSG_PREAMBLE_LENGTH  ((u_int8_t)3)

#endif /* __WF_GLOBAL_INCLUDES_H */
