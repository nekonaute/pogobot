# Release 2.7

### Mute / Unmute IR emitters

- pogobot.h : Added uint8_t _master_mute variable.
- pogolib_infrared.c : _master_mute set to 0 in pogobot_infrared_ll_init.
- pogolib_infrared.c : Conditional use of IR emitters in pogobot_infrared_senndRawLong/ShortMSg.
- on_complete_valid_slip...c : Handling of received messages (DEADCAFE sets _master_mute to 1, BASECAFE sets _master_mute to 0).

New functions :
- cmd_ir.c Added : rc_unmute_ir_handler, rc_mute_handler, rc_start_handler and rc_reboot_handler.
- cmd_pogobot.c : Added is_muted_handler.

New RC commands :
- is_muted  : Checks whether the PogoBot is muted or not.
- rc_mute   : Stops all IR emissions.
- rc_unmute : Restarts all IR emissions.
- rc_reboot : Replaces the legacy rc_stop command.

### User read/writable section in flash memory

Added a new user read/writable section in the flash memory.
This section is 64kB long and can be written in 256 bytes pages.
It starts at 0x290000 and has a size of 0x10000.

New functions spi.c :
- erase_write_section_flash : Erases the entire user-writable section (64 kB).
- write_page_flash          : Writes 256 bytes to a page in the user-writable section.
- read_page_flash           : Reads 256 bytes to a page in the user-writable section.

### Clear IR flags (prevents IR receiver saturation)

This code snippet allows the user to clear IR RX interrupt flags manually.
It prevents incoming IR messages from raising the interruption flags while the robot is still downloading the user code.

Updates in pogobot_ir.c :
- Added a code snippet in ir_init to handle flag clearing at start up.

New function in pogobot_ir.c :
- IR_reset_interrupt_flags : Clear all IR RX interrupt flags manually.
