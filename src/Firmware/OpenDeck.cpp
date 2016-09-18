/*
    OpenDeck MIDI platform firmware
    Copyright (C) 2015, 2016 Igor Petrovic

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "init/Init.h"

#define FIRMWARE_VERSION_STRING     0x56
#define HARDWARE_VERSION_STRING     0x42
#define REBOOT_STRING               0x7F
#define FACTORY_RESET_STRING        0x44

bool onCustom(uint8_t value) {

    switch(value)   {

        case FIRMWARE_VERSION_STRING:
        sysEx.addToResponse(getSWversion(swVersion_major));
        sysEx.addToResponse(getSWversion(swVersion_minor));
        sysEx.addToResponse(getSWversion(swVersion_revision));
        return true;

        case HARDWARE_VERSION_STRING:
        sysEx.addToResponse(hardwareVersion.major);
        sysEx.addToResponse(hardwareVersion.minor);
        sysEx.addToResponse(hardwareVersion.revision);
        return true;

        case REBOOT_STRING:
        leds.setFadeTime(1);
        leds.allOff();
        wait(1500);
        reboot();
        return true; //pointless, but we're making compiler happy

        case FACTORY_RESET_STRING:
        leds.setFadeTime(1);
        leds.allOff();
        wait(1500);
        configuration.factoryReset(factoryReset_partial);
        reboot();
        return true;

    }   return false;

}

sysExParameter_t onGet(uint8_t block, uint8_t section, uint16_t index) {

    switch(block)   {

        case CONF_BLOCK_LED:
        if (section == ledStateSection)    {

            return leds.getState(index);

        } else {

            return configuration.readParameter(block, section, index);

        }
        break;

        default:
        return configuration.readParameter(block, section, index);

    }

}

bool onSet(uint8_t block, uint8_t section, uint16_t index, sysExParameter_t newValue)   {

    bool returnValue = true;
    //don't write led states to eeprom
    if (block != CONF_BLOCK_LED)
        returnValue = configuration.writeParameter(block, section, index, newValue);

    if (returnValue)    {

        //special checks
        switch(block)   {

            case CONF_BLOCK_ANALOG:
            if (section == analogTypeSection)
                analog.debounceReset(index);
            break;

            case CONF_BLOCK_MIDI:
            if (section == midiFeatureSection)  {

                if (index == midiFeatureRunningStatus)
                    newValue ? midi.enableRunningStatus() : midi.disableRunningStatus();
                else if (index == midiFeatureStandardNoteOff)
                    newValue ? midi.setNoteOffMode(noteOffType_standardNoteOff) : midi.setNoteOffMode(noteOffType_noteOnZeroVel);

            }
            break;

            case CONF_BLOCK_LED:
            if (section == ledStateSection)  {

                switch ((ledStatesHardwareParameter)newValue)   {

                    case ledStateOff:
                    leds.setState(index, colorOff, false);
                    break;

                    case ledStateConstantWhite:
                    leds.setState(index, colorWhite, false);
                    break;

                    case ledStateConstantCyan:
                    leds.setState(index, colorCyan, false);
                    break;

                    case ledStateConstantMagenta:
                    leds.setState(index, colorMagenta, false);
                    break;

                    case ledStateConstantRed:
                    leds.setState(index, colorRed, false);
                    break;

                    case ledStateConstantBlue:
                    leds.setState(index, colorBlue, false);
                    break;

                    case ledStateConstantYellow:
                    leds.setState(index, colorYellow, false);
                    break;

                    case ledStateConstantGreen:
                    leds.setState(index, colorGreen, false);
                    break;

                    case ledStateBlinkWhite:
                    leds.setState(index, colorWhite, true);
                    break;

                    case ledStateBlinkCyan:
                    leds.setState(index, colorCyan, true);
                    break;

                    case ledStateBlinkMagenta:
                    leds.setState(index, colorMagenta, true);
                    break;

                    case ledStateBlinkRed:
                    leds.setState(index, colorRed, true);
                    break;

                    case ledStateBlinkBlue:
                    leds.setState(index, colorBlue, true);
                    break;

                    case ledStateBlinkYellow:
                    leds.setState(index, colorYellow, true);
                    break;

                    case ledStateBlinkGreen:
                    leds.setState(index, colorGreen, true);
                    break;

                    default:
                    return false;
                    break;

                }

            } else  {

                if (section == ledHardwareParameterSection) {

                    switch(index)    {

                        case ledHwParameterBlinkTime:
                        if ((newValue < BLINK_TIME_MIN) || (newValue > BLINK_TIME_MAX))
                        return false;
                        leds.setBlinkTime(newValue);
                        break;

                        case ledHwParameterFadeTime:
                        if ((newValue < FADE_TIME_MIN) || (newValue > FADE_TIME_MAX))
                        return false;
                        leds.setFadeTime(newValue);
                        break;

                        case ledHwParameterStartUpSwitchTime:
                        if ((newValue < START_UP_SWITCH_TIME_MIN) || (newValue > START_UP_SWITCH_TIME_MAX))
                        return false;
                        break;

                        case ledHwParameterStartUpRoutine:
                        if (newValue > NUMBER_OF_START_UP_ANIMATIONS)
                        return false;
                        break;

                    }

                }

                configuration.writeParameter(block, section, index, newValue);

            }
            break;

            default:
            break;

        }

    }

    return returnValue;

}

int main()  {

    globalInit();

    sysEx.setHandleGet(onGet);
    sysEx.setHandleSet(onSet);
    sysEx.setHandleCustomRequest(onCustom);

    sysEx.addCustomRequest(FIRMWARE_VERSION_STRING);
    sysEx.addCustomRequest(HARDWARE_VERSION_STRING);
    sysEx.addCustomRequest(REBOOT_STRING);
    sysEx.addCustomRequest(FACTORY_RESET_STRING);

    while(1)    {

        if (midi.read(usbInterface))   {   //new message on usb

            midiMessageType_t messageType = midi.getType(usbInterface);
            uint8_t data1 = midi.getData1(usbInterface);
            uint8_t data2 = midi.getData2(usbInterface);

            switch(messageType) {

                case midiMessageSystemExclusive:
                sysEx.handleSysEx(midi.getSysExArray(usbInterface), midi.getSysExArrayLength(usbInterface));
                break;

                case midiMessageNoteOff:
                case midiMessageNoteOn:
                //we're using received note data to control LEDs
                leds.noteToLEDstate(data1, data2);
                break;

                default:
                break;

            }

        }

        //check for incoming MIDI messages on USART
        if (midi.read(dinInterface))    {

            midiMessageType_t messageType = midi.getType(dinInterface);
            uint8_t data1 = midi.getData1(dinInterface);
            uint8_t data2 = midi.getData2(dinInterface);

            if (!configuration.readParameter(CONF_BLOCK_MIDI, midiFeatureSection, midiFeatureUSBconvert))  {

                switch(messageType) {

                    case midiMessageNoteOff:
                    case midiMessageNoteOn:
                    leds.noteToLEDstate(data1, data2);
                    break;

                    default:
                    break;

                }

            }   else {

                //dump everything from MIDI in to USB MIDI out
                uint8_t inChannel = configuration.readParameter(CONF_BLOCK_MIDI, midiChannelSection, inputChannel);
                switch(messageType) {

                    case midiMessageNoteOff:
                    midi.sendNoteOff(data1, data2, inChannel);
                    break;

                    case midiMessageNoteOn:
                    midi.sendNoteOn(data1, data2, inChannel);
                    break;

                    case midiMessageControlChange:
                    midi.sendControlChange(data1, data2, inChannel);
                    break;

                    case midiMessageProgramChange:
                    midi.sendProgramChange(data1, inChannel);
                    break;

                    case midiMessageSystemExclusive:
                    midi.sendSysEx(midi.getSysExArrayLength(dinInterface), midi.getSysExArray(dinInterface), true);
                    break;

                    case midiMessageAfterTouchChannel:
                    midi.sendAfterTouch(data1, inChannel);
                    break;

                    case midiMessageAfterTouchPoly:
                    midi.sendPolyPressure(data1, data2, inChannel);
                    break;

                    default:
                    break;

                }

            }

        }

        buttons.update();
        analog.update();
        encoders.update();

    }

}