/**
 * @file Telemetry.cpp
 * @author Cassandra "ZZ Cat" Robinson (nicad.heli.flier@gmail.com)
 * @brief Telemetry class implementation.
 * @version 0.5.0
 * @date 2023-09-17
 *
 * @copyright Copyright (c) 2023, Cassandra "ZZ Cat" Robinson. All rights reserved.
 *
 * @section License GNU General Public License v3.0
 * This source file is a part of the CRSF for Arduino library.
 * CRSF for Arduino is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CRSF for Arduino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CRSF for Arduino.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

#include "Telemetry.h"

using namespace crsfProtocol;

namespace serialReceiver
{
    Telemetry::Telemetry() :
        CRC(), SerialBuffer(CRSF_FRAME_SIZE_MAX)
    {
        _telemetryFrameScheduleCount = 0;
        memset(_telemetryFrameSchedule, 0, sizeof(_telemetryFrameSchedule));
        memset(&_telemetryData, 0, sizeof(_telemetryData));
    }

    Telemetry::~Telemetry()
    {
    }

    void Telemetry::begin()
    {
        SerialBuffer::reset();

        uint8_t index = 0;
        _telemetryFrameSchedule[index++] = (1 << CRSF_TELEMETRY_FRAME_GPS_INDEX);

        _telemetryFrameScheduleCount = index;
    }

    void Telemetry::end()
    {
        SerialBuffer::reset();
    }

    bool Telemetry::update()
    {
        bool sendFrame = false;

        static uint8_t scheduleIndex = 0;
        const uint8_t currentSchedule = _telemetryFrameSchedule[scheduleIndex];

        if (currentSchedule & (1 << CRSF_TELEMETRY_FRAME_GPS_INDEX))
        {
            _initialiseFrame();
            _appendGPSData();
            _finaliseFrame();
            sendFrame = true;
        }

        scheduleIndex = (scheduleIndex + 1) % _telemetryFrameScheduleCount;

        return sendFrame;
    }

    void Telemetry::setGPSData(float latitude, float longitude, float altitude, float speed, float course, uint8_t satellites)
    {
        _telemetryData.gps.latitude = latitude * 10000000;
        _telemetryData.gps.longitude = longitude * 10000000;
        _telemetryData.gps.altitude = (constrain(altitude, 0, 5000 * 100) / 100) + 1000;
        _telemetryData.gps.speed = ((speed * 36 + 50) / 100);
        _telemetryData.gps.groundCourse = (course * 100);
        _telemetryData.gps.satellites = satellites;
    }

    void Telemetry::sendTelemetryData(DevBoards *db)
    {
        uint8_t *buffer = SerialBuffer::getBuffer();
        size_t length = SerialBuffer::getLength();

        db->write(buffer, length);
    }

    void Telemetry::_initialiseFrame()
    {
        SerialBuffer::reset();
        SerialBuffer::writeU8(CRSF_SYNC_BYTE);
    }

    void Telemetry::_appendGPSData()
    {
        SerialBuffer::writeU8(CRSF_FRAME_GPS_PAYLOAD_SIZE + CRSF_FRAME_LENGTH_TYPE_CRC);
        SerialBuffer::writeU8(CRSF_FRAMETYPE_GPS);

        SerialBuffer::write32BE(_telemetryData.gps.latitude);
        SerialBuffer::write32BE(_telemetryData.gps.longitude);
        SerialBuffer::writeU16BE(_telemetryData.gps.speed);
        SerialBuffer::writeU16BE(_telemetryData.gps.groundCourse);
        SerialBuffer::writeU16BE(_telemetryData.gps.altitude);
        SerialBuffer::writeU8(_telemetryData.gps.satellites);
    }

    void Telemetry::_finaliseFrame()
    {
        uint8_t *buffer = SerialBuffer::getBuffer();
        uint8_t length = SerialBuffer::getLength();
        uint8_t crc = CRC::calculate(2, buffer[2], buffer, length);

        SerialBuffer::writeU8(crc);
    }
} // namespace serialReceiver
