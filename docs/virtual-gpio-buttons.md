# Virtual GPIO buttons (experimental)

This feature is **disabled by default**. Set `CONFIG_CANNECTIVITY_VIRTUAL_BUTTONS=y` in your application configuration only after defining board-specific GPIO aliases.

Define up to eight aliases named `yfa-button0` through `yfa-button7`, each pointing to a devicetree node with a `gpios` property. Example syntax (replace GPIO controller and pin with verified **unused** 3.3 V GPIOs for your board):

```dts
/ {
    aliases {
        yfa-button0 = &yfa_button_0;
    };
    yfa_button_0: yfa_button_0 {
        gpios = <&gpio0 0 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    };
};
```

**The pin 0 above is illustrative, not a validated Waveshare RP2350-CAN pin assignment.** Do not use it without checking the board schematic and existing CAN, USB, LED, termination and boot pin allocations. Buttons connect between a verified 3.3 V GPIO input and board ground; never connect vehicle 12 V to a GPIO.

Frames are injected only into the USB gs_usb RX FIFO (channel 0); they are **not** transmitted onto the physical CAN bus. Frame format: standard CAN ID `0x555`, DLC 2, data[0] = button state mask (alias 0 -> bit 0, alias 7 -> bit 7), data[1] = rolling 8-bit sequence counter. Logical 1 means pressed. Polling is every 10 ms, with ~30 ms stable-state debounce. A state snapshot is queued when USB CAN channel 0 starts; if it stops, the current state is sent again after restart. No heartbeat while unchanged.

Treat 0x555 as a device-local control frame in host applications; distinguish from real bus traffic when logging and do not replay it onto the physical CAN bus.

## Validation before hardware use

- Compile the firmware with the actual board target and overlays.
- Check that all configured pins are free and that the buttons are active-low.
- Verify first state snapshot, press, release, multi-button state and sequence wrap.
- Verify listen-only mode and observe the physical bus with a separate adapter: no 0x555 transmission.
- Check that sustained high-rate CAN traffic still records without loss.

## Waveshare RP2350-CAN proposed button pins

Waveshare documents a Pico-compatible 40-pin header. Its XL2515 SPI CAN controller uses GPIO8–GPIO12; do not use those for buttons. Onboard functions also occupy GPIO23–GPIO25 and GPIO29. Proposed independent button inputs, **pending verification against the active firmware devicetree and schematic**, are GPIO16, GPIO17, GPIO18, GPIO19, GPIO20, GPIO21, GPIO22 and GPIO26. These correspond to Pico header pins 21, 22, 24, 25, 26, 27, 29 and 31 respectively. GPIO26 can also serve as an ADC input; do not use it simultaneously for another purpose.

Reference: https://www.waveshare.com/wiki/RP2350-CAN and https://bridle.tiac-systems.net/doc/4.3/bridle/boards/waveshare/rp2350/doc/index.html

No board overlay is enabled until the project's actual board target and existing pin assignments are verified.

## Reconstructed Bridle build target

Bridle's Waveshare RP2350-CAN documentation explicitly provides a CANnectivity firmware build using the Cortex-M33 target. Board naming depends on the Bridle version:

- Bridle 4.3/4.4: `waveshare_rp2350_can/rp2350a/m33`
- Current Bridle documentation: `waveshare_rp2350_can/rp2350a/m33_0`

For a current Bridle workspace with the CANnectivity module checked out, build the opt-in GPIO feature using:

```sh
west build -b waveshare_rp2350_can/rp2350a/m33_0 -p always -d build/waveshare_rp2350_can cannectivity/app -- -DEXTRA_DTC_OVERLAY_FILE=overlays/waveshare-rp2350-can-buttons.overlay -DEXTRA_CONF_FILE=overlays/virtual-buttons.conf
```

The command uses the included Kconfig fragment `app/overlays/virtual-buttons.conf`. Check the Bridle release and application configuration before building. UF2 output is typically under `build/waveshare_rp2350_can/zephyr/zephyr.uf2`.

Official references: https://bridle.tiac-systems.net/doc/latest/bridle/boards/waveshare/rp2350_can/doc/index.html and https://bridle.tiac-systems.net/doc/4.3/bridle/boards/waveshare/rp2350/doc/index.html
