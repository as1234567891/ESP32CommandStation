/**********************************************************************
DCC++ BASE STATION FOR ESP32

COPYRIGHT (c) 2017 Mike Dunston

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses
**********************************************************************/

#include "DCCppESP32.h"
#include "Outputs.h"

/**********************************************************************

DCC++ESP32 BASE STATION supports optional OUTPUT control of any unused pins for
custom purposes. Pins can be activited or de-activated. The default is to set
ACTIVE pins HIGH and INACTIVE pins LOW. However, this default behavior can be
inverted for any pin in which case ACTIVE=LOW and INACTIVE=HIGH.

Definitions and state (ACTIVE/INACTIVE) for pins are retained on the ESP32 and
restored on power-up. The default is to set each defined pin to active or
inactive according to its restored state. However, the default behavior can be
modified so that any pin can be forced to be either active or inactive upon
power-up regardless of its previous state before power-down.

To have this sketch utilize one or more Arduino pins as custom outputs, first
define/edit/delete output definitions using the following variation of the "Z"
command:

  <Z ID PIN IFLAG>: creates a new output ID, with specified PIN and IFLAG values.
                    if output ID already exists, it is updated with specificed
                    PIN and IFLAG.
                    Note: output state will be immediately set to ACTIVE/INACTIVE
                    and pin will be set to HIGH/LOW according to IFLAG value
                    specifcied (see below).
        returns: <O> if successful and <X> if unsuccessful (e.g. out of memory)

  <Z ID>:           deletes definition of output ID
        returns: <O> if successful and <X> if unsuccessful (e.g. ID does not exist)

  <Z>:              lists all defined output pins
        returns: <Y ID PIN IFLAG STATE> for each defined output pin or <X> if no
        output pins defined

where

  ID: the numeric ID (0-32767) of the output
  PIN: the pin number to use for the output
  STATE: the state of the output (0=INACTIVE / 1=ACTIVE)
  IFLAG: defines the operational behavior of the output based on bits 0, 1, and
  2 as follows:

    IFLAG, bit 0:   0 = forward operation (ACTIVE=HIGH / INACTIVE=LOW)
                    1 = inverted operation (ACTIVE=LOW / INACTIVE=HIGH)

    IFLAG, bit 1:   0 = state of pin restored on power-up to either ACTIVE or
                        INACTIVE depending on state before power-down; state of
                        pin set to INACTIVE when first created.
                    1 = state of pin set on power-up, or when first created, to
                        either ACTIVE of INACTIVE depending on IFLAG, bit 2.

    IFLAG, bit 2:   0 = state of pin set to INACTIVE upon power-up or when
                        first created.
                    1 = state of pin set to ACTIVE upon power-up or when
                        first created.

Once all outputs have been properly defined, use the <E> command to store their
definitions. If you later make edits/additions/deletions to the output
definitions, you must invoke the <E> command if you want those new definitions
updated on the ESP32.  You can also clear everything stored on the ESP32 by
invoking the <e> command.

To change the state of outputs that have been defined use:

  <Z ID STATE>:     sets output ID to either ACTIVE or INACTIVE state
                    returns: <Y ID STATE>, or <X> if turnout ID does not exist
where
  ID: the numeric ID (0-32767) of the turnout to control
  STATE: the state of the output (0=INACTIVE / 1=ACTIVE)

When controlled as such, the ESP32 updates and stores the direction of each
output so that it is retained even without power. A list of the current states
of each output in the form <Y ID STATE> is generated by this sketch whenever the
<s> status command is invoked.  This provides an efficient way of initializing
the state of any outputs being monitored or controlled by a separate interface
or GUI program.

**********************************************************************/
LinkedList<Output *> outputs([](Output *output) {delete output; });

void OutputManager::init() {
  log_i("Initializing outputs");
  uint16_t outputCount = configStore.getUShort("OutputCount", 0);
  log_i("Found %d outputs", outputCount);
  for(int index = 0; index < outputCount; index++) {
    outputs.add(new Output(index));
  }
}

void OutputManager::clear() {
  configStore.putUShort("OutputCount", 0);
  outputs.free();
}

uint16_t OutputManager::store() {
  uint16_t outputStoredCount = 0;
  for (const auto& output : outputs) {
    output->store(outputStoredCount++);
  }
  configStore.putUShort("OutputCount", outputStoredCount);
  return outputStoredCount;
}

bool OutputManager::set(uint16_t id, bool active) {
  for (const auto& output : outputs) {
    if(output->getID() == id) {
      output->set(active);
      return true;
    }
  }
  return false;
}

bool OutputManager::toggle(uint16_t id) {
  for (const auto& output : outputs) {
    if(output->getID() == id) {
      output->set(!output->isActive());
      return true;
    }
  }
  return false;
}

void OutputManager::getState(JsonArray & array) {
  for (const auto& output : outputs) {
    JsonObject &outputJson = array.createNestedObject();
    outputJson[F("id")] = output->getID();
    outputJson[F("pin")] = output->getPin();
    String flagsString = "";
    if(bitRead(output->getFlags(), OUTPUT_IFLAG_INVERT)) {
      flagsString += "activeLow";
    } else {
      flagsString += "activeHigh";
    }
    if(bitRead(output->getFlags(), OUTPUT_IFLAG_RESTORE_STATE)) {
      if(bitRead(output->getFlags(), OUTPUT_IFLAG_FORCE_STATE)) {
        flagsString += ",force(on)";
      } else {
        flagsString += ",force(off)";
      }
    } else {
      flagsString += ",restoreState";
    }
    outputJson[F("flags")] = flagsString;
    if(output->isActive()) {
      outputJson[F("active")] = "On";
    } else {
      outputJson[F("active")] = "Off";
    }
  }
}

void OutputManager::showStatus() {
  for (const auto& output : outputs) {
    output->showStatus();
  }
}

bool OutputManager::createOrUpdate(const uint16_t id, const uint8_t pin, const uint8_t flags) {
  for (const auto& output : outputs) {
    if(output->getID() == id) {
      output->update(pin, flags);
      return true;
    }
  }
  if(std::find(restrictedPins.begin(), restrictedPins.end(), pin) != restrictedPins.end()) {
    return false;
  }
  outputs.add(new Output(id, pin, flags));
  return true;
}

bool OutputManager::remove(const uint16_t id) {
  Output *outputToRemove = nullptr;
  for (const auto& output : outputs) {
    if(output->getID() == id) {
      outputToRemove = output;
    }
  }
  if(outputToRemove != nullptr) {
    outputs.remove(outputToRemove);
    return true;
  }
  return false;
}

Output::Output(uint16_t id, uint8_t pin, uint8_t flags) : _id(id), _pin(pin), _flags(flags), _active(false) {
  String flagsString = "";
  if(bitRead(_flags, OUTPUT_IFLAG_INVERT)) {
    flagsString += "activeLow";
  } else {
    flagsString += "activeHigh";
  }
  if(bitRead(_flags, OUTPUT_IFLAG_RESTORE_STATE)) {
    if(bitRead(_flags, OUTPUT_IFLAG_FORCE_STATE)) {
      flagsString += ",force(on)";
      set(true, false);
    } else {
      flagsString += ",force(off)";
      set(false, false);
    }
  } else {
    flagsString += ",restoreState";
    set(false, false);
  }
  log_i("Output(%d) on pin %d created, flags: %s", _id, _pin, flagsString.c_str());
  pinMode(_pin, OUTPUT);
}

Output::Output(uint16_t index) {
  String outputIDKey = String("O_") + String(index);
  String outputPinKey = outputIDKey + String("_p");
  String outputFlagsKey = outputIDKey + String("_f");
  _id = configStore.getUShort(outputIDKey.c_str(), index);
  _pin = configStore.getUChar(outputPinKey.c_str(), 0);
  _flags = configStore.getUChar(outputFlagsKey.c_str(), 0);
  String flagsString = "";
  if(bitRead(_flags, OUTPUT_IFLAG_INVERT)) {
    flagsString += "activeLow";
  } else {
    flagsString += "activeHigh";
  }
  if(bitRead(_flags, OUTPUT_IFLAG_RESTORE_STATE)) {
    if(bitRead(_flags, OUTPUT_IFLAG_FORCE_STATE)) {
      flagsString += ",force(on)";
      set(true, false);
    } else {
      flagsString += ",force(off)";
      set(false, false);
    }
  } else {
    String outputStateKey = outputIDKey + String("_s");
    if(configStore.getBool(outputStateKey.c_str(), false)) {
      flagsString += ",restoreState(on)";
      set(true, false);
    } else {
      flagsString += ",restoreState(off)";
      set(false, false);
    }
  }
  log_i("Output(%d) on pin %d loaded, flags: %s", _id, _pin, flagsString.c_str());
  pinMode(_pin, OUTPUT);
}

void Output::set(bool active, bool announce) {
  _active = active;
  digitalWrite(_pin, _active);
  log_i("Output(%d) set to %s", _id, _active ? "ON" : "OFF");
  if(announce) {
    wifiInterface.printf(F("<Y %d %d>"), _id, !_active);
  }
}

void Output::update(uint8_t pin, uint8_t flags) {
  _pin = pin;
  _flags = flags;
  String flagsString = "";
  if(bitRead(_flags, OUTPUT_IFLAG_INVERT)) {
    flagsString += "activeLow";
  } else {
    flagsString += "activeHigh";
  }
  if(!bitRead(_flags, OUTPUT_IFLAG_RESTORE_STATE)) {
    flagsString += ",restoreState";
    set(false, false);
  } else {
    if(bitRead(_flags, OUTPUT_IFLAG_FORCE_STATE)) {
      flagsString += ",force(on)";
      set(true, false);
    } else {
      flagsString += ",force(off)";
      set(false, false);
    }
  }
  log_i("Output(%d) on pin %d updated, flags: %s", _id, _pin, flagsString.c_str());
  pinMode(_pin, OUTPUT);
}

void Output::store(uint16_t index) {
  String outputIDKey = String("O_") + String(index);
  String outputPinKey = outputIDKey + String("_p");
  String outputFlagsKey = outputIDKey + String("_f");
  String outputStateKey = outputIDKey + String("_s");
  configStore.putUShort(outputIDKey.c_str(), _id);
  configStore.putUChar(outputPinKey.c_str(), _pin);
  configStore.putUChar(outputFlagsKey.c_str(), _flags);
  configStore.putBool(outputStateKey.c_str(), _active);
}

void Output::showStatus() {
  wifiInterface.printf(F("<Y %d %d %d %d>"), _id, _pin, _flags, !_active);
}

void OutputCommandAdapter::process(const std::vector<String> arguments) {
  if(arguments.empty()) {
    // list all outputs
    OutputManager::showStatus();
  } else {
    uint16_t outputID = arguments[0].toInt();
    if (arguments.size() == 1 && OutputManager::remove(outputID)) {
      // delete output
      wifiInterface.printf(F("<O>"));
    } else if (arguments.size() == 2 && OutputManager::set(outputID, arguments[1].toInt() == 1)) {
      // set output state
    } else if (arguments.size() == 3) {
      // create output
      OutputManager::createOrUpdate(outputID, arguments[1].toInt(), arguments[2].toInt());
      wifiInterface.printf(F("<O>"));
    } else {
      wifiInterface.printf(F("<X>"));
    }
  }
}
