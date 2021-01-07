#!/usr/bin/env python
#

# Copyright (C) 2018-2021 Linar Yusupov

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

def long_frame_encode(callsign, icao, lat, lon, alt, ec, nic):

    payload_type = 1
    aq = 5       # Fixed ADS-B Beacon
    alt_type = 0 # Pressure Altitude

    long_frame_bytes = []
    long_frame_bytes.append((payload_type<<3) | aq) # 1

    long_frame_bytes.append((icao>>16) & 0xff)      # 2
    long_frame_bytes.append((icao>> 8) & 0xff)      # 3
    long_frame_bytes.append((icao    ) & 0xff)      # 4

    enc_lat = int(lat * (1 << 24) / 360)
    enc_lon = int(lon * (1 << 24) / 360)

    long_frame_bytes.append(( enc_lat>>15) & 0xff)  # 5
    long_frame_bytes.append(( enc_lat>> 7) & 0xff)  # 6
    long_frame_bytes.append(((enc_lat & 0x7F) << 1) | ((enc_lon>> 23) & 0x01))
    long_frame_bytes.append(( enc_lon>>15) & 0xff)  # 8
    long_frame_bytes.append(( enc_lon>> 7) & 0xff)  # 9
    long_frame_bytes.append(((enc_lon & 0x7F) << 1) | (alt_type & 0x01))

    enc_alt =	int(round((alt + 1000) / 25)) + 1

    long_frame_bytes.append(( enc_alt>> 4) & 0xff)  # 11
    long_frame_bytes.append(((enc_alt & 0x0F) << 4) | (nic & 0x0F))

    # Horizontal Velocity
    long_frame_bytes.append(0x80)  # on ground      # 13
    long_frame_bytes.append(0x00)                   # 14
    long_frame_bytes.append(0x00)                   # 15
    # Vertical Velocity
    long_frame_bytes.append(0x00)                   # 16
    long_frame_bytes.append(0x00)                   # 17

    base40_alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ  .."

    s1 = base40_alphabet.find(callsign[0])
    if s1 == -1:
      s1 = 36
    s2 = base40_alphabet.find(callsign[1])
    if s2 == -1:
      s2 = 36
    word16 = ec * 1600 + s1 * 40 + s2

    long_frame_bytes.append((word16>>8) & 0xff)     # 18
    long_frame_bytes.append((word16   ) & 0xff)     # 19

    s1 = base40_alphabet.find(callsign[2])
    if s1 == -1:
      s1 = 36
    s2 = base40_alphabet.find(callsign[3])
    if s2 == -1:
      s2 = 36
    s3 = base40_alphabet.find(callsign[4])
    if s3 == -1:
      s3 = 36
    word16 = s1 * 1600 + s2 * 40 + s3

    long_frame_bytes.append((word16>>8) & 0xff)     # 20
    long_frame_bytes.append((word16   ) & 0xff)     # 21

    s1 = base40_alphabet.find(callsign[5])
    if s1 == -1:
      s1 = 36
    s2 = base40_alphabet.find(callsign[6])
    if s2 == -1:
      s2 = 36
    s3 = base40_alphabet.find(callsign[7])
    if s3 == -1:
      s3 = 36
    word16 = s1 * 1600 + s2 * 40 + s3

    long_frame_bytes.append((word16>>8) & 0xff)     # 22
    long_frame_bytes.append((word16   ) & 0xff)     # 23

    long_frame_bytes.append(0x0b)                   # 24
    long_frame_bytes.append(0x4a)                   # 25
    long_frame_bytes.append(0xa5)                   # 26
    long_frame_bytes.append(0xc2)                   # 27
    long_frame_bytes.append(0xa0)                   # 28
    long_frame_bytes.append(0x00)                   # 29
    long_frame_bytes.append(0x00)                   # 30
    long_frame_bytes.append(0x00)                   # 31
    long_frame_bytes.append(0x00)                   # 32
    long_frame_bytes.append(0x00)                   # 33
    long_frame_bytes.append(0x00)                   # 34
    
    return long_frame_bytes
    
if __name__ == "__main__":

    from sys import argv, exit
    
    argc = len(argv)
    if argc != 6:
      print
      print 'Usage: '+ argv[0] +' <CallSign> <ICAO> <Latitude> <Longitude> <Altitude (ft)>'
      print
      print '    Example: '+ argv[0] +' RAMPTEST 0x1ABBA1 59.6583 17.9617 137.8'
      print
      exit(2)

    callsign = (argv[1].upper() + "        ")[:8]
    icao = int(argv[2], 16)
    lat = float(argv[3])
    lon = float(argv[4])
    alt = float(argv[5])

    if lat > 90:
      lat = 90
    if lat <= -90:
      lat = -89.999999

    if lon > 180:
      lon = 180
    if lon <= -180:
      lon = -179.999999

    ec = 18
    nic = 0    

    uat_message = long_frame_encode(callsign, icao, lat, lon, alt, ec, nic)

    # print '-'+''.join(format(x, '02x') for x in uat_message)+';'  # dump978 format

    print
    print '#define UAT_DATA    "'+''.join(format(x, '02x') for x in uat_message)+'"'
    print
