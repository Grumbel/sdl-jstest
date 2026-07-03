#pragma once

/* PS5 trigger effect documentation:
   https://controllers.fandom.com/wiki/Sony_DualSense#FFB_Trigger_Modes
*/
typedef struct
{
    Uint8 ucEnableBits1; /* 0 */
    Uint8 ucEnableBits2; /* 1 */
    Uint8 ucRumbleRight; /* 2 */
    Uint8 ucRumbleLeft; /* 3 */
    Uint8 ucHeadphoneVolume; /* 4 */
    Uint8 ucSpeakerVolume; /* 5 */
    Uint8 ucMicrophoneVolume; /* 6 */
    Uint8 ucAudioEnableBits; /* 7 */
    Uint8 ucMicLightMode; /* 8 */
    Uint8 ucAudioMuteBits; /* 9 */
    Uint8 rgucRightTriggerEffect[11]; /* 10 */
    Uint8 rgucLeftTriggerEffect[11]; /* 21 */
    Uint8 rgucUnknown1[6]; /* 32 */
    Uint8 ucLedFlags; /* 38 */
    Uint8 rgucUnknown2[2]; /* 39 */
    Uint8 ucLedAnim; /* 41 */
    Uint8 ucLedBrightness; /* 42 */
    Uint8 ucPadLights; /* 43 */
    Uint8 ucLedRed; /* 44 */
    Uint8 ucLedGreen; /* 45 */
    Uint8 ucLedBlue; /* 46 */
} DS5EffectsState_t;

static int trigger_effect = 0;

/**
 * Taken from https://github.com/libsdl-org/SDL/blob/120c76c84bbce4c1bfed4e9eb74e10678bd83120/test/testgamecontroller.c#L286-L307§
 */
static void CyclePS5TriggerEffect(SDL_GameController* gamepad)
{
    DS5EffectsState_t state;

    // TODO: bring more from https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db
    Uint8 effects[3][11] = {
        /* Clear trigger effect */
        {0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        /* Constant resistance across entire trigger pull */
        {0x01, 0, 110, 0, 0, 0, 0, 0, 0, 0, 0},
        /* Resistance and vibration when trigger is pulled */
        {0x06, 15, 63, 128, 0, 0, 0, 0, 0, 0, 0},
    };

    trigger_effect = (trigger_effect + 1) % SDL_arraysize(effects);

    SDL_zero(state);
    state.ucEnableBits1 |= (0x04 | 0x08); /* Modify right and left trigger effect respectively */
    SDL_memcpy(state.rgucRightTriggerEffect, effects[trigger_effect], sizeof(effects[trigger_effect]));
    SDL_memcpy(state.rgucLeftTriggerEffect, effects[trigger_effect], sizeof(effects[trigger_effect]));
    SDL_GameControllerSendEffect(gamepad, &state, sizeof(state));
}
