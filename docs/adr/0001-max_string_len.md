# 0001 - Maximum String Length

## Status

Implementing

## Context

The `24LC08B` I2C EEPROM can store up to 8 kBits of data, but writing long strings followed by short strings
 presents an issue where the short string will only overwrite part of the long string, after which a null
 terminator will be placed and the remainder of the longer string will remain. This is acceptable behavior for
 purely reading, but it is not clean or structured.

Moreover, this approach makes it impossible to reserve a part of the EEPROM for other configuration information.
 As such, a restriction on the maximum length of a string must be created. This will also allow for a more
 efficient page write operation to erase remaining data when a new string is written.

## Decision

The EEPROM can store 8 kBits of data which equates to 1,024 bytes or chars. This is far more than is necessary for a quote, so I think it reasonable to restrict the user to **1 block, or 256 bytes/chars, of space**. Considering the nature of a scrolling display this seems like far more than enough.

This length also allows the microcontroller to efficiently wipe that one block when new data is inserted, or when the user sends an ERASE command.

## Consequences

- **Positive:** The EEPROM is now structured. It can be programmed with additional configuration information in reserved blocks
- **Positive:** The text stored in the EEPROM can now be erased quickly
- **Negative:** The user has a maximum string length. It is very large for the purposes of this system, but still exists
