# pmaports for the Samsung Galaxy Tab A7 10.4 2020 (gta4l, SM-T500/T505)

postmarketOS packages for gta4l. Copy `device/testing/*` into your pmaports
checkout (`pmbootstrap config aports` shows where it is), then:

```sh
pmbootstrap checksum linux-postmarketos-qcom-sm6115 device-samsung-gta4l hexagonrpcd-gta4l firmware-samsung-gta4l
pmbootstrap init        # vendor: samsung, device: gta4l
pmbootstrap install
```

The `sha512sums` are left empty on purpose: generate them with the first
command above.

## Packages

| Package | What it is |
|---|---|
| `linux-postmarketos-qcom-sm6115` | Kernel 7.2: [LinuxForGta4l/linux](https://github.com/LinuxForGta4l/linux) `gta4l-7.2` at `c91d0e9` plus the `.patch` files in this folder. The same tree with the patches applied is at [limonabisi/linux-gta4l](https://github.com/limonabisi/linux-gta4l). |
| `device-samsung-gta4l` | deviceinfo, firmware import from the tablet's own partitions, speaker tuning import, ALSA UCM, headset jack handling, USB network gadget id, battery count kept across reboots. |
| `device-samsung-gta4l-charger` | Charging screen when a charger is plugged into the powered-off tablet (`androidboot.mode=charger`). |
| `device-samsung-gta4l-sensors` | Accelerometer, light sensor and compass through the ADSP (hexagonrpcd + libssc + iio-sensor-proxy). |
| `hexagonrpcd-gta4l` | hexagonrpcd with one patch: it also serves `sns_reg_version` next to the sensor registry. |
| `firmware-samsung-gta4l` | Firmware packages. |

## Status

Working: display, touch (NT36523 lide-hsd), Wi-Fi, Bluetooth, speakers,
microphone, GPU (Adreno 610, Turnip), battery, offline charging, sleep
(s2idle), accelerometer/light/compass.

Not yet: video decoding (Venus probes with Samsung firmware 6.0.49 but no
frames come out, so it is blacklisted in `gta4l-venus.conf`); headset not
fully tested; the sensor configuration and registry are taken from the
tablet's vendor/persist data and are not packaged here.

Patches whose subject starts with `TEST` are still being tried on hardware.
