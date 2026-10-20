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
