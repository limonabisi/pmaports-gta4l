# pmaports for the Samsung Galaxy Tab A7 10.4 2020 (gta4l, SM-T500/T505)

postmarketOS packages for gta4l, one folder per package (the same layout as
[LinuxForGta4l/postmarketos-packages-gta4l](https://github.com/LinuxForGta4l/postmarketos-packages-gta4l)).

## Building

Copy the package folders into `device/testing/` of your pmaports checkout
(`pmbootstrap config aports` shows where it is), then:

```sh
pmbootstrap init        # vendor: samsung, device: gta4l, UI: plasma-desktop
pmbootstrap install
pmbootstrap export      # boot.img and the rootfs image
```

The `sha512sums` are filled in. If you change a file, regenerate them with
`pmbootstrap checksum <package>`.

The tablet boots through U-Boot: build it from `u-boot-gta4l/` (see its
README) and wrap the pmOS `boot.img` with `UBOOT-BOOTIMG.py`, which also
writes the Odin tar to flash as AP.

## Packages

| Package | What it is |
|---|---|
| `linux-postmarketos-qcom-sm6115` | Kernel 7.2: [LinuxForGta4l/linux](https://github.com/LinuxForGta4l/linux) `gta4l-7.2` at `c91d0e9` plus 11 patches, one per driver and one for the DTS. The same series is the `gta4l-7.2-clean` branch of [limonabisi/linux-gta4l](https://github.com/limonabisi/linux-gta4l/tree/gta4l-7.2-clean). |
| `device-samsung-gta4l` | deviceinfo, firmware import from the tablet's own partitions (read with mtools, never mounted), speaker tuning import, ALSA UCM, wake-up source rule, PAM fix for the systemd user manager, battery count kept across reboots; lighter defaults (no accessibility bus, KWallet or X11 tray bridges, 64 MB journal, Wi-Fi power saving off, weekly TRIM). |
| `device-samsung-gta4l-sensors` | Accelerometer, light sensor and compass through the ADSP (hexagonrpcd + libssc + iio-sensor-proxy). |
| `hexagonrpcd-gta4l` | hexagonrpcd with one patch: it also serves `sns_reg_version` next to the sensor registry. |
| `firmware-samsung-gta4l` | Firmware packages. |
| `u-boot-gta4l/` | U-Boot patches (charging in U-Boot, charge symbol and percentage, ABL command line pass-through) and `UBOOT-BOOTIMG.py`. |

## Status

Working: display, touch (NT36523 lide-hsd), Wi-Fi, Bluetooth, speakers,
microphone, headset jack switching, GPU (Adreno 610, Turnip), battery,
charging (the PMI632 charger watchdog is kept by the kernel), offline
charging (in U-Boot, with the percentage), Bluetooth RFCOMM/BNEP, sleep (s2idle), accelerometer/light/compass, Venus video decoding
(1080p H.264 about 99 fps, HEVC about 66 fps), USB OTG at 1.5 A, Ethernet
over USB, U-Boot.

Known problems:
- Venus encoding and VP9 decoding are off: the Samsung firmware (6.0.49) predates the fixes of 6.0.55. H.264 and HEVC decoding work.
- USB OTG and charging at the same time is not possible: the PMI632 has no
  USB PD PHY, so a hub cannot swap the power role.
- Headphones: only the left channel plays; the right channel's data does not
  reach the WCD937x (its HPHR PDM watchdog trips).
- The ADSP crashes on resume from s2idle while the sensor domain is in use;
  `gta4l-sensors-sleep.service` stops the sensor services around sleep.
- Battery level: the coulomb count now follows the voltage when they disagree
  by more than 25 points on battery; still being verified on the tablet.
- The sensor configuration and registry are taken from the tablet's
  vendor/persist data and are not packaged here.
