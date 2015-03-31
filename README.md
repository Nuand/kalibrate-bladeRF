#kalibrate-bladeRF instructions#

The bladeRF has an on-board VCTCXO (Voltage Controlled Temperature Compensated Crystal Oscillator) that is factory calibrated to within 20ppb. This calibration is partly found by reading the VCTCXO's output frequency with a frequency counter as the on-board trim DAC's output voltage is modified programmatically. The goal of the calibration procedure is to get the VCTCXO's output frequency to be as close to 38.4MHz as possible. Unfortunately due to the fact that components age, the factory calibration does not remain valid for more than a year at a time. To counteract the effects of component aging, kalibrate-bladeRF can be used to find a new DAC trim voltage to bring the VCTCXO's output frequency back to 38.4MHz. In the factory a 10ppb OCXO frequency counter is used to detect the error, kalibrate-bladeRF instead requires nothing more than a bladeRF and a GSM antenna. kalibrate-bladeRF works by listening to special GSM channels (timeslots) known as FCCHs (Frequency Correction Channel) to determine its frequency offset from the basestation.

## Important Information ##

## Back Up Existing Calibration Data ###
It is ***very*** important that you first back up the factory-calibrated VCTCXO
trim value prior to attempting to change it. This [bladeRF wiki
page](https://github.com/Nuand/bladeRF/wiki/bladeRF-CLI-Tips-and-Tricks#Backing_up_and_restoring_calibration_data)
describes how to do this. At a minimum, write down and keep the ''VCTCXO DAC
calibration'' value reported by the bladeRF-cli:
```
$ bladeRF-cli -i
bladeRF> info

  Serial #:                 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  VCTCXO DAC calibration:   0x8a41  <---------------- [Save this value]
  FPGA size:                40 KLE
  FPGA loaded:              yes
  USB bus:                  12
  USB address:              2
  USB speed:                SuperSpeed
  Backend:                  libusb
  Instance:                 0

```

## Temporarily Testing New Calibration Values ##
Note that the `bladeRF-cli` program allows you to temporarily and immediately
apply a VCTCXO trim value, using the `set trimdac <value>` command.

Before writing a new value to flash, it is ***highly*** recommend that you
first test out any new values using the aforementioned `bladeRF-cli` command.
When using the `set trimdac <value>` command, this trim value will be used by
the hardware until this command issued again with a different value, or the
device is reset.

For example, if a calibration value of 0x932b is desired:

```
$ bladeRF-cli -i
bladeRF> set trimdac 0x932b
bladeRF> quit
```

or alternatively...

```
bladeRF-cli -e 'set trimdac 0x932b'
```

Note that the bladeRF-cli `info` command will still show the value in flash, rather than the temporarily applied value.

Once content with this value, the wiki page linked earlier can be followed to write this value to flash, if desired.

### Channel Selection ###
This implementation currently does not include digital filtering (issue #4).  Until this is addressed, one needs to identify a channel that does not have any strong neighboring signals within the hardware's minimum 1.5 MHz LPF bandwidth setting, and specify the use of that channel.


## Compiling ##

Once a recent version of libbladeRF is installed run the following commands to install kalibrate-bladeRF:
<pre>
# ./bootstrap
# ./configure
# make
</pre>

## Running ##

The basic idea is to scan and find a nearby GSM basestation with a strong signal. Once a suitable basestation is found, kalibrate should be run in calibration mode.

#### Step one ####

Using an antenna that can tune to 850MHz/950MHz scan GSM850.
<pre>./kal -s GSM850</pre>
If nothing shows up try another band like GSM900, or increase the amount of time kalibrate spends on each channel by adding a `multiple` argument.
<pre>./kal -s GSM850 -m4</pre>

Sample out:
<pre>
Actual filter bandwidth = 1500 kHz
rxvga1 = 20 dB
rxvga2 = 30 dB
kal: Scanning for GSM-850 base stations.
GSM-850:
        chan: 128 (869.2MHz + 312Hz)    power: 30407.42
        chan: 129 (869.4MHz + 316Hz)    power: 31074.89
        chan: 130 (869.6MHz + 316Hz)    power: 31640.16
</pre>
In this capture all of the channels are about identical in power and frequency offset (ie. 312Hz, 316Hz). ARFCNs with more than a few kHz of frequency offset should be avoided.
For the next step ARFCN 128 is selected.

#### Step two ####

Begin calibrating by specifying an ARFCN with argument -c. Based on the previous step, we are selecting ARFCN 128.

<pre>
./kal -c 128
</pre>

Alternatively argument `-w` can be specified to have kalibrate-bladeRF automatically update the on-board calibration table. ***It is not recommended that you use this argument, unless you have backed up your previous calibration value!***

<pre>
./kal -c 128 -w
</pre>

This step should take about 2 to 3 minutes to run. A series of dots representing detected FCCHs should appear as new DAC values are attempted.

#### Results ####

<pre>Found lowest offset of 2.069210Hz at 869.200000MHz (0.002381 ppm) using DAC trim 0xa20c</pre>
In this calibration run a VCTCXO trim DAC value of 0xa20c was found to calibrate the bladeRF's 38.4MHz crystal to within 2.381 parts per billion. Anything less than 50ppb and 20ppb is suitable for GSM and LTE applications, respectively.

#### Troubleshooting ####

Signal strength can greatly impact the performance of kalibrate-bladeRF. Sometimes channel fading can be combated by physically moving the bladeRF around.


---

Kalibrate, or kal, can scan for GSM base stations in a given frequency band and
can use those GSM base stations to calculate the local oscillator frequency
offset.

See http://thre.at/kalibrate 

Copyright (c) 2010, Joshua Lackey (jl@thre.at)
