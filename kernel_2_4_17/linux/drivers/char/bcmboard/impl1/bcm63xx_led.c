/*
<:copyright-gpl 
 Copyright 2002 Broadcom Corp. All Rights Reserved. 
 
 This program is free software; you can distribute it and/or modify it 
 under the terms of the GNU General Public License (Version 2) as 
 published by the Free Software Foundation. 
 
 This program is distributed in the hope it will be useful, but WITHOUT 
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 for more details. 
 
 You should have received a copy of the GNU General Public License along 
 with this program; if not, write to the Free Software Foundation, Inc., 
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA. 
:>
*/
/***************************************************************************
 * File Name  : bcm63xx_led.c
 *
 * Description: 
 *
 *    This file contains bcm963xx board led control API functions. 
 *
 *    To use it, do the following
 *
 *    1). define in the board.c the following led mappping (this is for 6345GW board):
 *        const LED_MAP_PAIR cLedMapping45GW[] =
 *        {   // led name     Initial state       physical pin (ledMask)
 *            {kLedUsb,       kLedStateOff,       GPIO_LED_PIN_7}, 
 *            {kLedAdsl,      kLedStateOff,       GPIO_LED_PIN_8},
 *            {kLedPPP,       kLedStateOff,       GPIO_LED_PIN_9},    // PPP and WanData share PIN_9
 *            {kLedWanData,   kLedStateOff,       GPIO_LED_PIN_9},
 *            {kLedWireless,  kLedStateOff,       GPIO_LED_PIN_10},
 *            {kLedEnd,       kLedStateOff,       0              } // NOTE: kLedEnd has to be at the end.
 *
 *    2). };To initialize led API and initial state of the leds, call the following function with the mapping 
 *        pointer from the above struct
 *
 *        boardLedInit((PLED_MAP_PAIR) &cLedMapping45R);
 *
 *    3). Sample call for kernel mode:
 *
 *        kerSysLedCtrl(kLedAdsl, kLedStateBlinkOnce);        // kLedxxx defines in board.h
 *
 *    4). Sample call for user mode
 *
 *        sysLedCtrl(kLedAdsl, kLedStateBlinkOnce);           // kLedxxx defines in board_api.h
 *
 *
 * Created on :  10/28/2002  seanl
 *
 ***************************************************************************/

/* Includes. */
#include <linux/fs.h>
#include <linux/iobuf.h>
#include <linux/capability.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <asm/uaccess.h>

#if defined(CONFIG_BCM96348)
#include <6348_map_part.h>
#elif defined(CONFIG_BCM96345)
#include <6345_map_part.h>
#elif defined(CONFIG_BCM96338)
#include <6338_map_part.h>
#endif


#include <board.h>

#define k100ms              ((100 * HZ) / 1000)     // ~100 ms
#define kFastBlinkCount     0                       // ~100ms
#define kSlowBlinkCount     5                       // ~600ms

#define MAX_HW_HANDLERS     6

// uncomment // for debug led
//#define DEBUG_LED

// global variables:
struct timer_list gLedTimer;
int gTimerOn = FALSE;
int gLedCount = 0;
PLED_MAP_PAIR gLedMapPtr;

typedef struct ledinfo
{
    unsigned short ledMask;         // mask for led: ie. giop 10 = 0x0400
    unsigned short ledActiveLow;    // GPIO bit reset to turn on LED
    unsigned short ledMaskFail;     // mask for led: ie. giop 10 = 0x0400
    unsigned short ledActiveLowFail;// GPIO bit reset to turn on LED
    BOARD_LED_STATE ledState;       // current led state
    BOARD_LED_STATE savedLedState;  // used in blink once for restore to the orignal ledState
    int blinkCountDown;             // if == 0, do blink (toggle).  Is assgined value and dec by 1 at each timer.
} LED_INFO, *PLED_INFO;

typedef struct ledhwinfo
{
    BOARD_LED_NAME ledName;         // name of LED
    HANDLE_LED_FUNC ledHwFunc;      // function to handle hardware LED
    int ledFailType;                // 1 = failure LED, 0 = normal LED
} LED_HW_INFO, *PLED_HW_INFO;

static PLED_INFO gLed = NULL;              // content sample: {0x0100, kLedStateOff, 0, 0},
static LED_HW_INFO gLedHwInfo[MAX_HW_HANDLERS] = {{0}};
static int gLedHwInfoCount = 0;
#if defined(CONFIG_BCM96348)
static int gLedOffInBridgeMode = 1;
#elif defined(CONFIG_BCM96345)
static int gLedOffInBridgeMode = 0;
#endif

void ledTimerExpire(void);
int initLedInfo( PLED_MAP_PAIR pCurMap, PLED_INFO pCurLed );
int CallHwLedHandler( BOARD_LED_NAME ledName, BOARD_LED_STATE ledState );

//**************************************************************************************
// LED operations
//**************************************************************************************

// return the led info ptr associated with ledName
PLED_INFO ledMapping(BOARD_LED_NAME ledName)
{
    PLED_MAP_PAIR pLedMap = gLedMapPtr;
    int i;

    while (pLedMap->ledName != kLedEnd)
    {

        if (pLedMap->ledName == ledName)
            break;
        pLedMap++;
    }

    for (i = 0; i < gLedCount; i++)
    {
        if ((gLed+i)->ledMask == pLedMap->ledMask /* || */
/*             (gLed+i)->ledMaskFail == pLedMap->ledMaskFail */)
            return (gLed+i);
    }

    return (PLED_INFO) NULL;        // should not come here
}


// turn led on and set the ledState
void ledOn(PLED_INFO pLed)
{
    if( pLed->ledMask )
    {
        GPIO->GPIODir |= pLed->ledMask;         // turn on the direction bit in case was turned off by some one
        if( pLed->ledActiveLow )
            GPIO->GPIOio  &= ~pLed->ledMask;    // turn on the led
        else
            GPIO->GPIOio  |= pLed->ledMask;     // turn on the led
        pLed->ledState = pLed->savedLedState = kLedStateOn;
    }
}


// turn led off and set the ledState
void ledOff(PLED_INFO pLed)
{
    if( pLed->ledMask )
    {
        GPIO->GPIODir |= pLed->ledMask;         // turn on the direction bit in case was turned off by some one
        if( pLed->ledActiveLow )
            GPIO->GPIOio  |= pLed->ledMask;     // turn off the led
        else
            GPIO->GPIOio  &= ~pLed->ledMask;    // turn off the led
        pLed->ledState = pLed->savedLedState = kLedStateOff;
    }
}

// turn led on and set the ledState
void ledOnFail(PLED_INFO pLed)
{
    if( pLed->ledMaskFail )
    {
        GPIO->GPIODir |= pLed->ledMaskFail;     // turn on the direction bit in case was turned off by some one
        if( pLed->ledActiveLowFail )
            GPIO->GPIOio  &= ~pLed->ledMaskFail;// turn on the led
        else
            GPIO->GPIOio  |= pLed->ledMaskFail; // turn on the led
        pLed->ledState = pLed->savedLedState = kLedStateFail;
    }
}


// turn led off and set the ledState
void ledOffFail(PLED_INFO pLed)
{
    if( pLed->ledMaskFail )
    {
        GPIO->GPIODir |= pLed->ledMaskFail;     // turn on the direction bit in case was turned off by some one
        if( pLed->ledActiveLowFail )
            GPIO->GPIOio  |= pLed->ledMaskFail; // turn off the led
        else
            GPIO->GPIOio  &= ~pLed->ledMaskFail;// turn off the led
        pLed->ledState = pLed->savedLedState = kLedStateOff;
    }
}


// toggle the led and return the current ledState
BOARD_LED_STATE ledToggle(PLED_INFO pLed)
{
    GPIO->GPIODir |= pLed->ledMask;         // turn on the direction bit in case was turned off by some one
    if (GPIO->GPIOio & pLed->ledMask)
    {
        GPIO->GPIOio &= ~(pLed->ledMask);
        return( (pLed->ledActiveLow) ? kLedStateOn : kLedStateOff );
    }
    else
    {
        GPIO->GPIOio |= pLed->ledMask;
        return( (pLed->ledActiveLow) ? kLedStateOff : kLedStateOn );
    }
}   


// led timer.  Will return if timer is already on
void ledTimerStart(void)
{
    if (gTimerOn)
        return;

#if defined(DEBUG_LED)
    printk("led: add_timer\n");
#endif
    gTimerOn = TRUE;
    init_timer(&gLedTimer);
    gLedTimer.function = (void*)ledTimerExpire;
    gLedTimer.expires = jiffies + k100ms;        // timer expires in ~100ms
    add_timer (&gLedTimer);

} 


// led timer expire kicks in about ~100ms and perform the led operation according to the ledState and
// restart the timer according to ledState
void ledTimerExpire(void)
{
    int i;
    PLED_INFO pCurLed;
    // CMO -- mesures de protection -- UN PEU BOURRIN, A REVOIR PLUS PRECISEMENT
    unsigned long flags ;

    save_flags(flags) ; cli() ;

    gTimerOn = FALSE;

    for (i = 0, pCurLed = gLed; i < gLedCount; i++, pCurLed++)
    {
#if defined(DEBUG_LED)
        printk("led[%d]: Mask=0x%04x, State = %d, blcd=%d\n", i, pCurLed->ledMask, pCurLed->ledState, pCurLed->blinkCountDown);
#endif
        switch (pCurLed->ledState)
        {
            case kLedStateOn:
            case kLedStateOff:
            case kLedStateFail:
                pCurLed->blinkCountDown = 0;            // reset the blink count down
                break;

            case kLedStateBlinkOnce:
                ledToggle(pCurLed);
                pCurLed->blinkCountDown = 0;                      // reset to 0
                pCurLed->ledState = pCurLed->savedLedState;
                if (pCurLed->ledState == kLedStateSlowBlinkContinues || 
                    pCurLed->ledState == kLedStateFastBlinkContinues)
                    ledTimerStart();                  // start timer if in blinkContinues stats
                break;

            case kLedStateSlowBlinkContinues:
                if (pCurLed->blinkCountDown-- == 0)
                {
                    pCurLed->blinkCountDown = kSlowBlinkCount;
                    ledToggle(pCurLed);
                }
                ledTimerStart();
                break;

            case kLedStateFastBlinkContinues:
                if (pCurLed->blinkCountDown-- == 0)
                {
                    pCurLed->blinkCountDown = kFastBlinkCount;
                    ledToggle(pCurLed);
                }
                ledTimerStart();
                break;

            default:
                printk("Invalid state = %d\n", pCurLed->ledState);
        }
    }
    restore_flags(flags);
}

// initialize the gLedCount and allocate and fill gLed struct
void __init boardLedInit(PLED_MAP_PAIR cLedMapping)
{
    PLED_MAP_PAIR p1, p2;
    PLED_INFO pCurLed;
    int needTimer = FALSE;
    int alreadyUsed = 0;

#if defined(CONFIG_BCM96348)
    /* Set blink rate for BCM6348 hardware LEDs. */
    GPIO->LEDCtrl &= ~LED_INTERVAL_SET_MASK;
    GPIO->LEDCtrl |= LED_INTERVAL_SET_80MS;
#endif

    memset( gLedHwInfo, 0x00, sizeof(gLedHwInfo) );

    gLedMapPtr = cLedMapping;
    gLedCount = 0;

    // Check for multiple LED names and multiple LED GPIO pins that share the
    // same physical board LED.
    for( p1 = cLedMapping; p1->ledName != kLedEnd; p1++ )
    {
        alreadyUsed = 0;
        for( p2 = cLedMapping; p2 != p1; p2++ )
        {
            if( (p1->ledMask && p1->ledMask == p2->ledMask) /*  || */
/*                 (p1->ledMaskFail && p1->ledMaskFail == p2->ledMaskFail) */ )
            {
                alreadyUsed = 1;
                break;
            }
        }

        if( alreadyUsed == 0  )
            gLedCount++;
    }

    gLed = (PLED_INFO) kmalloc((gLedCount * sizeof(LED_INFO)), GFP_KERNEL);
    if( gLed == NULL )
    {
        printk( "LED memory allocation error.\n" );
        return;
    }

    memset( gLed, 0x00, gLedCount * sizeof(LED_INFO) );

    // initial the gLed with unique ledMask and initial state. If more than 1 ledNames share the physical led 
    // (ledMask) the first defined led's ledInitState will be used.
    pCurLed = gLed;
    for( p1 = cLedMapping; p1->ledName != kLedEnd; p1++ )
    {     
        alreadyUsed = 0;
	printk("New led %d, mask : 0x%08x\n",p1->ledName, p1->ledMask);
        for( p2 = cLedMapping; p2 != p1; p2++ )
        {
            if( p1->ledMask == p2->ledMask /*  || */
/*                 p1->ledMaskFail == p2->ledMaskFail  */)
            {	      
                alreadyUsed = 1;
		printk("INV -- ALREADY USED\n");
                break;
            }
        }

        if( alreadyUsed == 0 )
        {
            // Initialize the board LED for the first time.
            needTimer = initLedInfo( p1, pCurLed );
            pCurLed++;
        }
        else
        {
	  
            PLED_INFO pLed;
	    printk("INV -- led deja initialisee, mask : 0x%08x\n",p1->ledMask);
            for( pLed = gLed; pLed != pCurLed; pLed++ )
            {
                // Find the LED_INFO structure that has already been initialized.
                if((pLed->ledMask && pLed->ledMask == p1->ledMask) ||
                   (pLed->ledMaskFail && pLed->ledMaskFail==p1->ledMaskFail))
                {
                    // The board LED has already been initialized but possibly
                    // not completely initialized.
                    if( p1->ledMask )
                    {
                        pLed->ledMask = p1->ledMask;
                        pLed->ledActiveLow = p1->ledActiveLow;
                    }
                    if( p1->ledMaskFail )
                    {
                        pLed->ledMaskFail = p1->ledMaskFail;
                        pLed->ledActiveLowFail = p1->ledActiveLowFail;
                    }
                    break;
                }
            }
        }
    }

    if (needTimer)
        ledTimerStart();

#if defined(DEBUG_LED)
    int i;
    for (i=0; i < gLedCount; i++)
        printk("initLed: led[%d]: mask=0x%04x, state=%d\n", i,(gLed+i)->ledMask, (gLed+i)->ledState);
#endif

}

// Initialize a structure that contains information about a physical board LED
// control.  The board LED may contain more than one GPIO pin to control a
// normal condition (green) or a failure condition (red).
int initLedInfo( PLED_MAP_PAIR pCurMap, PLED_INFO pCurLed )
{
    int needTimer = FALSE;
    pCurLed->ledState = pCurLed->savedLedState = pCurMap->ledInitState;
    pCurLed->ledMask = pCurMap->ledMask;
    pCurLed->ledActiveLow = pCurMap->ledActiveLow;
    pCurLed->ledMaskFail = pCurMap->ledMaskFail;
    pCurLed->ledActiveLowFail = pCurMap->ledActiveLowFail;

    switch (pCurLed->ledState)
    {
        case kLedStateOn:
            pCurLed->blinkCountDown = 0;            // reset the blink count down
            ledOn(pCurLed);
            break;
        case kLedStateOff:
            pCurLed->blinkCountDown = 0;            // reset the blink count down
            ledOff(pCurLed);
            break;
        case kLedStateFail:
            pCurLed->blinkCountDown = 0;            // reset the blink count down
            ledOnFail(pCurLed);
            break;
        case kLedStateBlinkOnce:
            pCurLed->blinkCountDown = 1;
            needTimer = TRUE;
            break;
        case kLedStateSlowBlinkContinues:
            pCurLed->blinkCountDown = kSlowBlinkCount;
            needTimer = TRUE;
            break;
        case kLedStateFastBlinkContinues:
            pCurLed->blinkCountDown = kFastBlinkCount;
            needTimer = TRUE;
            break;
        default:
            printk("Invalid state = %d\n", pCurLed->ledState);
    }

    return( needTimer );
}

// Determines if there is at least one interface in bridge mode.  Bridge mode
// is determined by the cfm convention of naming bridge interfaces nas17
// through nas24.
static int isBridgedProtocol(void)
{
    extern int dev_get(const char *name);
    const int firstBridgeId = 17;
    const int lastBridgeId = 24;
    int i;
    int ret = FALSE;
    char name[16];

    for( i = firstBridgeId; i <= lastBridgeId; i++ )
    {
        sprintf( name, "nas%d", i );

        if( dev_get(name) )
        {
            ret = TRUE;
            break;
        }
    }

    return(ret);
}

// led ctrl.  Maps the ledName to the corresponding ledInfoPtr and perform the led operation
void boardLedCtrl(BOARD_LED_NAME ledName, BOARD_LED_STATE ledState)
{
    PLED_INFO ledInfoPtr;
//dli modif start
    unsigned long flags ;

#if 0
    // CMO -- pour faire une release dans laquelle les leds ne sont pas actives 
    //       (pour débugguer avec un oscillo)
    return ;
#else 
 
    save_flags(flags) ; 
    cli() ;

//    printk("INV -- called boardLedCtrl, ledName : %d \n",ledName);

    // do the mapping from virtual to physical led
    ledInfoPtr = ledMapping(ledName);

//    printk("INV -- ledInfoPtr : %p\n",ledInfoPtr);
//    if (ledInfoPtr) printk("INV -- ledInfoPtr -> ledMask : 0x%08x \n",ledInfoPtr -> ledMask );

    // First, see if there is a registed function that handles this LED.
    if(CallHwLedHandler( ledName, ledState ) == 1)
    {
//      printk("INV -- HW handler\n");
        // This LED was handled by a registered function.  Turn off the
        // complimentary (normal or failure) LED GPIO.
        if( ledInfoPtr )
        {
            if( ledState == kLedStateFail )
                ledOff(ledInfoPtr);
            else
                ledOffFail(ledInfoPtr);
        }
	restore_flags(flags) ;
        return;
    }

    if (ledInfoPtr == NULL) {
        restore_flags(flags) ;
        return;
    }
#if 0 
    // Do not blink the WAN Data LED if at least one interface is in bridge mode.
    if(gLedOffInBridgeMode == 1 && ledName == kLedWanData || ledName == kLedPPP)
    {
        static int BridgedProtocol = -1;

        if( BridgedProtocol == -1 )
            BridgedProtocol = isBridgedProtocol();

        if( BridgedProtocol == TRUE )
            return;
    }
#endif

    // If the state is kLedStateFail and there is not a failure LED defined
    // in the board parameters, change the state to kLedStateFastBlinkContinues.
    if( ledState == kLedStateFail && ledInfoPtr->ledMaskFail == 0 )
        ledState = kLedStateFastBlinkContinues;

    switch (ledState)
    {
        case kLedStateOn:
            // First, turn off the complimentary (failure) LED GPIO.
            if( ledInfoPtr->ledMaskFail )
                ledOffFail(ledInfoPtr);
            else
                CallHwLedHandler( ledName, kLedStateOff );

            // Next, turn on the specified LED GPIO.
            ledOn(ledInfoPtr);
            break;

        case kLedStateOff: 
            // First, turn off the complimentary (failure) LED GPIO.
            if( ledInfoPtr->ledMaskFail )
                ledOffFail(ledInfoPtr);
            else
                CallHwLedHandler( ledName, kLedStateOff );

            // Next, turn off the specified LED GPIO.
            ledOff(ledInfoPtr);
            break;

        case kLedStateFail:
            // First, turn off the complimentary (normal) LED GPIO.
            if( ledInfoPtr->ledMask )
                ledOff(ledInfoPtr);
            else
                CallHwLedHandler( ledName, kLedStateOff );

            // Next, turn on (red) the specified LED GPIO.
            ledOnFail(ledInfoPtr);
            break;

        case kLedStateBlinkOnce:
            // skip blinkOnce if it is already in Slow/Fast blink continues state
            if (ledInfoPtr->savedLedState == kLedStateSlowBlinkContinues ||
                ledInfoPtr->savedLedState == kLedStateFastBlinkContinues)
                ;
            else
            {
                if (ledInfoPtr->blinkCountDown == 0)  // skip the call if it is 1
                {
                    ledToggle(ledInfoPtr);
                    ledInfoPtr->blinkCountDown = 1;  // it will be reset to 0 when timer expires
                    ledInfoPtr->ledState = kLedStateBlinkOnce;
                    ledTimerStart();
                }
            }
            break;

        case kLedStateSlowBlinkContinues:
            ledInfoPtr->blinkCountDown = kSlowBlinkCount;
            ledInfoPtr->ledState = kLedStateSlowBlinkContinues;
            ledInfoPtr->savedLedState = kLedStateSlowBlinkContinues;
            ledTimerStart();
            break;

        case kLedStateFastBlinkContinues:
            ledInfoPtr->blinkCountDown = kFastBlinkCount;
            ledInfoPtr->ledState = kLedStateFastBlinkContinues;
            ledInfoPtr->savedLedState = kLedStateFastBlinkContinues;
            ledTimerStart();
            break;

        default:
            printk("Invalid led state\n");
    }
    restore_flags(flags) ;
#endif
}

// This function is called for an LED that is controlled by hardware.
void kerSysLedRegisterHwHandler( BOARD_LED_NAME ledName,
    HANDLE_LED_FUNC ledHwFunc, int ledFailType )
{
    if( gLedHwInfoCount < MAX_HW_HANDLERS )
    {
        PLED_HW_INFO pHwInfo = &gLedHwInfo[gLedHwInfoCount++];

        pHwInfo->ledName = ledName;
        pHwInfo->ledHwFunc = ledHwFunc;
        pHwInfo->ledFailType = ledFailType;
    }
}

// This function determines if there is a registered function to handle
// the specified LED.
int CallHwLedHandler( BOARD_LED_NAME ledName, BOARD_LED_STATE ledState )
{
    int ledFailType = (ledState == kLedStateFail) ? 1 : 0;
    int ret = 0;
    int i;
    PLED_HW_INFO pHwInfo;

    for( i = 0, pHwInfo = gLedHwInfo; i < gLedHwInfoCount; i++, pHwInfo++ )
    {
        if( pHwInfo->ledName == ledName && pHwInfo->ledFailType == ledFailType )
        {
            (*pHwInfo->ledHwFunc) (ledName, ledState);
            ret = 1;
            break;
        }
    }

    return( ret );
}

