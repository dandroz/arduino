/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "supla/channel.h"
#include "supla-common/log.h"
#include "supla-common/srpc.h"
#include "tools.h"

namespace Supla {
Channel *Channel::firstPtr = nullptr;

unsigned long Channel::nextCommunicationTimeMs = 0;
TDS_SuplaRegisterDevice_E Channel::reg_dev;

Channel::Channel() {
  valueChanged = false;
  channelNumber = -1;
  if (reg_dev.channel_count < SUPLA_CHANNELMAXCOUNT) {
    channelNumber = reg_dev.channel_count;
    reg_dev.channels[channelNumber].Number = channelNumber;
    reg_dev.channel_count++;
  } else {
// TODO: add status CHANNEL_LIMIT_EXCEEDED
  }

  if (firstPtr == nullptr) {
    firstPtr = this;
  } else {
    last()->nextPtr = this;
  }
  nextPtr = nullptr;
}

Channel *Channel::begin() {
  return firstPtr;
}

Channel *Channel::last() {
  Channel *ptr = firstPtr;
  while (ptr && ptr->nextPtr) {
    ptr = ptr->nextPtr;
  }
  return ptr;
}

int Channel::size() {
  int count = 0;
  Channel *ptr = firstPtr;
  if (ptr) {
    count++;
  }
  while (ptr->nextPtr) {
    count++;
    ptr = ptr->nextPtr;
  }
  return count;
}

void Channel::setNewValue(double dbl) {
  char newValue[SUPLA_CHANNELVALUE_SIZE];
  if (sizeof(double) == 8) {
    memcpy(newValue, &dbl, 8);
  } else if (sizeof(double) == 4) {
    float2DoublePacked(dbl, (uint8_t *)(newValue));
  }
  if (setNewValue(newValue)) {
    supla_log(LOG_DEBUG, "Channel(%d) value changed to %f", channelNumber, dbl);
  }
}

void Channel::setNewValue(double temp, double humi) {
  char newValue[SUPLA_CHANNELVALUE_SIZE];
  long t = temp * 1000.00;
  long h = humi * 1000.00;

  memcpy(newValue, &t, 4);
  memcpy(&(newValue[4]), &h, 4);

  if (setNewValue(newValue)) {
    supla_log(LOG_DEBUG,
              "Channel(%d) value changed to temp(%f), humi(%f)",
              channelNumber,
              temp,
              humi);
  }
}

void Channel::setNewValue(int value) {
  char newValue[SUPLA_CHANNELVALUE_SIZE];

  memset(newValue, 0, SUPLA_CHANNELVALUE_SIZE);

  memcpy(newValue, &value, sizeof(int));
  if (setNewValue(newValue)) {
    supla_log(
        LOG_DEBUG, "Channel(%d) value changed to %d", channelNumber, value);
  }
}

void Channel::setNewValue(bool value) {
  char newValue[SUPLA_CHANNELVALUE_SIZE];

  memset(newValue, 0, SUPLA_CHANNELVALUE_SIZE);

  newValue[0] = value;
  if (setNewValue(newValue)) {
    supla_log(
        LOG_DEBUG, "Channel(%d) value changed to %d", channelNumber, value);
  }
}

void Channel::setNewValue(TElectricityMeter_ExtendedValue &emValue) {
  // Prepare standard channel value
  if (sizeof(TElectricityMeter_Value) <= SUPLA_CHANNELVALUE_SIZE) {
    TElectricityMeter_Measurement *m = nullptr;
    TElectricityMeter_Value v;
    memset(&v, 0, sizeof(TElectricityMeter_Value));

    unsigned _supla_int64_t fae_sum = emValue.total_forward_active_energy[0] +
                                      emValue.total_forward_active_energy[1] +
                                      emValue.total_forward_active_energy[2];

    v.total_forward_active_energy = fae_sum / 1000;

    if (emValue.m_count && emValue.measured_values & EM_VAR_VOLTAGE) {
      m = &emValue.m[emValue.m_count - 1];

      if (m->voltage[0] > 0) {
        v.flags |= EM_VALUE_FLAG_PHASE1_ON;
      }

      if (m->voltage[1] > 0) {
        v.flags |= EM_VALUE_FLAG_PHASE2_ON;
      }

      if (m->voltage[2] > 0) {
        v.flags |= EM_VALUE_FLAG_PHASE3_ON;
      }
    }

    memcpy(reg_dev.channels[channelNumber].value,
           &v,
           sizeof(TElectricityMeter_Value));
    setUpdateReady();
  }
}

bool Channel::setNewValue(char *newValue) {
  if (memcmp(newValue, reg_dev.channels[channelNumber].value, 8) != 0) {
    setUpdateReady();
    memcpy(reg_dev.channels[channelNumber].value, newValue, 8);
    return true;
  }
  return false;
}

void Channel::setType(int type) {
  if (channelNumber >= 0) {
    reg_dev.channels[channelNumber].Type = type;
  }
}

void Channel::setDefault(int value) {
  if (channelNumber >= 0) {
    reg_dev.channels[channelNumber].Default = value;
  }
}

void Channel::setFlag(int flag) {
  if (channelNumber >= 0) {
    reg_dev.channels[channelNumber].Flags |= flag;
  }
}

void Channel::setFuncList(int functions) {
  if (channelNumber >= 0) {
    reg_dev.channels[channelNumber].FuncList = functions;
  }
}

int Channel::getChannelNumber() {
  return channelNumber;
}

void Channel::clearUpdateReady() {
  valueChanged = false;
};

void Channel::sendUpdate(void *srpc) {
  srpc_ds_async_channel_value_changed(
      srpc, channelNumber, reg_dev.channels[channelNumber].value);

  // returns null for non-extended channels
  TSuplaChannelExtendedValue *extValue = getExtValue();
  if (extValue) {
    srpc_ds_async_channel_extendedvalue_changed(srpc, channelNumber, extValue);
  }

  clearUpdateReady();
}

TSuplaChannelExtendedValue *Channel::getExtValue() {
  return nullptr;
}

void Channel::clearAllUpdateReady() {
  for (auto channel = begin(); channel != nullptr; channel = channel->next()) {
    if (!channel->isExtended()) {
      channel->clearUpdateReady();
    }
  }
}

Channel *Channel::next() {
  return nextPtr;
}

void Channel::setUpdateReady() {
  valueChanged = true;
};

bool Channel::isUpdateReady() {
  return valueChanged;
};

bool Channel::isExtended() {
  return false;
}

void Channel::setNewValue(uint8_t red,
                   uint8_t green,
                   uint8_t blue,
                   uint8_t colorBrightness,
                   uint8_t brightness) {
  char newValue[SUPLA_CHANNELVALUE_SIZE];
  memset(newValue, 0, SUPLA_CHANNELVALUE_SIZE);
  newValue[0] = brightness;
  newValue[1] = colorBrightness;
  newValue[2] = blue;
  newValue[3] = green;
  newValue[4] = red;
  if (setNewValue(newValue)) {
    supla_log(LOG_DEBUG, "Channel(%d) value changed to RGB(%d, %d, %d), colBr(%d), bright(%d)", channelNumber, red, green, blue, colorBrightness, brightness);
  }
}

int Channel::getChannelType() {
  if (channelNumber >= 0) {
    return reg_dev.channels[channelNumber].Type;
  }
  return -1;
}

};  // namespace Supla
