# UAT (ADS-B 978 MHz) test signal generator

## Disclaimer

This low power UAT test source is designed for a lab or an aircraft hangar use only.

## Legitimate use

Radio being used in the project is rated for 14 dBm (25 mW) of maximum transmit power.<br>
FCC compliant built-in RF filter reduces the power even more, down to approximately 1 mW on 978 MHz.<br> 
This power is sufficient to cover an area of just only few meters around the transmitter.<br>

## Validation

### Raw data

```
pi@raspberrypi:/run/tmp/dump978-master $ rtl_sdr -f 978000000 -s 2083334 -g 8 - | ./dump978
Found 1 device(s):
  0:  Generic, RTL2832U, SN: 77771111153705700

Using device 0: Generic RTL2832U
Found Rafael Micro R820T tuner
Exact sample rate is: 2083334.141630 Hz
[R82XX] PLL not locked!
Sampling at 2083334 S/s.
Tuned to 978000000 Hz.
Tuner gain set to 7.70 dB.
Reading samples in async mode...
-0d1abba154d8ec198ba602f0800000000074c28d855bfd0b4aa5c2a0000000000000;
...
```

### Text

```
pi@raspberrypi:/run/tmp/dump978-master $ rtl_sdr -f 978000000 -s 2083334 -g 8 - | ./dump978 | ./uat2text
Found 1 device(s):
  0:  Generic, RTL2832U, SN: 77771111153705700

Using device 0: Generic RTL2832U
Found Rafael Micro R820T tuner
Exact sample rate is: 2083334.141630 Hz
[R82XX] PLL not locked!
Sampling at 2083334 S/s.
Tuned to 978000000 Hz.
Tuner gain set to 7.70 dB.
Reading samples in async mode...
HDR:
 MDB Type:          1
 Address:           1ABBA1 (Fixed ADS-B Beacon Address)
SV:
 NIC:               0
 Latitude:          +59.6583
 Longitude:         +17.9617
 Altitude:          150 ft (barometric)
 Dimensions:        15.0m L x 11.5m W
 UTC coupling:      no
 TIS-B site ID:     0
MS:
 Emitter category:  Service vehicle
 Callsign:          RAMPTEST
 Emergency status:  No emergency
 UAT version:       2
 SIL:               3
 Transmit MSO:      18
 NACp:              10
 NACv:              2
 NICbaro:           1
 Capabilities:      CDTI ACAS
 Active modes:
 Target track type: true heading
AUXSV:
 Sec. altitude:     unavailable
...
```

### Map

```
pi@raspberrypi:/run/tmp $ rtl_sdr -f 978000000 -s 2083334 -g 8 - | ./dump978-master/dump978 | ./dump978-master/uat2json /run/tmp/dump1090-master/public_html/data/
Found 1 device(s):
  0:  Generic, RTL2832U, SN: 77771111153705700

Using device 0: Generic RTL2832U
Found Rafael Micro R820T tuner
Exact sample rate is: 2083334.141630 Hz
[R82XX] PLL not locked!
Sampling at 2083334 S/s.
Tuned to 978000000 Hz.
Tuner gain set to 7.70 dB.
Reading samples in async mode...
...
```

<img src="https://github.com/lyusupov/UAT-test-signal/raw/master/notes/pics/dump978.jpg" height="418" width="674">

## Data customization

1. Execute *UATEncoder.py* script with `<CallSign>` `<ICAO>` `<Latitude>` `<Longitude>` `<Altitude>` arguments:
```
$ python UATEncoder.py RAMPTEST 0x1ABBA1 59.6583 17.9617 137.8

#define UAT_DATA    "0d1abba154d8ec198ba602f0800000000074c28d855bfd0b4aa5c2a0000000000000"


```

2. Edit *UAT_data.h* file:
```
#ifndef UAT_DATA

#define UAT_DATA    "0d1abba154d8ec198ba602f0800000000074c28d855bfd0b4aa5c2a0000000000000"

#endif
```

3. Build your custom firmware and download it into your signal generator hardware.

