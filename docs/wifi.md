# WiFi: updates and network MIDI

The Pi Zero 2 W has onboard WiFi (2.4 GHz). MiniDexed brings it up at boot and then offers:

- **FTP** on port 21 (user `admin`, password `admin`) for updating the firmware, config and voice banks without touching the SD card.
- **AppleMIDI / RTP-MIDI** on port 5004, announced over mDNS as `ZicDexed`. This is the Bluetooth stand-in: Circle has no Bluetooth stack, so wireless MIDI goes over WiFi instead.

`config/minidexed.ini` already has `NetworkEnabled=1`, `NetworkType=wlan`, `NetworkDHCP=1`, `NetworkHostname=ZicDexed` and `NetworkFTPEnabled=1`.

## One-time SD card setup

1. Run `./build.sh`. It downloads the WiFi firmware into `out/firmware/` (set `WIFI_FIRMWARE=0` to skip, for example when offline).
2. Copy `out/firmware/` to the card as `firmware/`.
3. Copy `out/wpa_supplicant.conf.example` to the card as `wpa_supplicant.conf` and set your `country`, `ssid` and `psk`. Keep the file to `country=` and the `network={}` block: Circle's supplicant is a cut-down build, and options such as `ctrl_interface` or `update_config` make it reject the whole file, leaving WiFi stuck at "joining AP".
4. Boot. If the firmware or `wpa_supplicant.conf` is missing, the log shows `Failed to initialize WLAN, maybe firmware files are missing?` and the synth runs without network.

Add `NetworkSyslogEnabled=1` and `NetworkSyslogServerIPAddress=<your PC>` in the ini to watch the boot log with `syslogserver.py` from MiniDexed when something does not connect.

## Updating over WiFi

With the synth on the network, from the repo root:

```
./build.sh
pip install requests zeroconf
python3 build/MiniDexed/updater.py
```

Pick **Local build** to upload `kernel8.img` as built, or a release to pull from GitHub. The script finds the unit through mDNS (`_ftp._tcp`), uploads, and the synth reboots when the FTP session ends. Any FTP client works too: connect to `ZicDexed.local` as `admin`/`admin`, and files under `/SD` are the card.

The FTP login is fixed in MiniDexed and has no encryption, so only enable it on a network you trust. Set `NetworkFTPEnabled=0` to turn it off.

## Network MIDI

- **macOS:** Audio MIDI Setup, Window, Show MIDI Studio, Network. Add a session, and `ZicDexed` shows up under Directory; Connect.
- **Windows:** install rtpMIDI (Tobias Erichsen), add `ZicDexed` from the directory (or by IP, port 5004), Connect. It appears as a MIDI port in your DAW.
- **iPad / iPhone:** CoreMIDI network sessions work from apps such as AUM or Midi Wrench.
- **Linux:** `rtpmidid` or `aconnect` over a `rtpmidid` session.

Latency is a few milliseconds on a good 2.4 GHz link but can spike on a busy one. For gigging use USB.

## Not tested yet

No hardware run of this change yet. Check on the device: WiFi joins, the IP shows on the display, `updater.py` finds the unit, and a DAW sees the AppleMIDI port.
