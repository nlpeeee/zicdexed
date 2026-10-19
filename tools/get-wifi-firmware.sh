#!/bin/bash
# Downloads the Pi Zero 2 W WiFi firmware into ./firmware (run from where you want the folder).
# Same files as get-wifi-firmware.ps1 and as build.sh's out/firmware.
set -euo pipefail
base=https://github.com/RPi-Distro/firmware-nonfree/raw/c9d3ae6584ab79d19a4f94ccf701e888f9f87a53/debian/config/brcm80211
mkdir -p firmware
get() { curl -fsSL "$base/$1" -o "firmware/$2"; }
get cypress/cyfmac43430-sdio.bin brcmfmac43430-sdio.bin
get brcm/brcmfmac43430-sdio.txt brcmfmac43430-sdio.txt
get cypress/cyfmac43430-sdio.clm_blob brcmfmac43430-sdio.clm_blob
for n in 43436 43436s 43456; do
	get brcm/brcmfmac$n-sdio.bin brcmfmac$n-sdio.bin
	get brcm/brcmfmac$n-sdio.txt brcmfmac$n-sdio.txt
done
get brcm/brcmfmac43436-sdio.clm_blob brcmfmac43436-sdio.clm_blob
get brcm/brcmfmac43456-sdio.clm_blob brcmfmac43456-sdio.clm_blob
for suffix in "" ".raspberrypi,5-model-b"; do
	get cypress/cyfmac43455-sdio-minimal.bin "brcmfmac43455-sdio$suffix.bin"
	get brcm/brcmfmac43455-sdio.txt "brcmfmac43455-sdio$suffix.txt"
	get cypress/cyfmac43455-sdio.clm_blob "brcmfmac43455-sdio$suffix.clm_blob"
done
echo "WiFi firmware is in ./firmware: copy that folder to the SD card root."
