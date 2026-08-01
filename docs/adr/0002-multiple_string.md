# 0002 - Multiple Strings

## Status

Implementing

## Context

To allow the user to add multiple strings, a set of architecture decisions must be made. These decisions largely encompass how the storage of
these strings should be manipulated by the user, and the design constraint of the display and EEPROM storage.

## Decision

To allow multiple strings to be stored, a set of commmands and design constraints must be implemented. These changes are proposed below:

### New Commands

- `ADD` - Adds a new str to the list
- `REM` - Remove the *last* string from the list
- `CLR` - Removes *all* strings from the list

### Architecture changes

As you can see, this will require a `char` array of fixed length with a seperate variable holding the current position. As such, the following
code changes are proposed:

- `app_com` module added to handle command interpretation. Will also contain string array and current position variable
- **5 string maximum** imposed on string array
- **128 byte/char maximum *per string*** to accomodate all strings within the 8K EEPROM. Each string will have a fixed location in memory to
accomodate their storage.

## Consequences

- **Positive:** Multiple strings can be stored, and EEPROM will be more fully utilized
- **Negative:** Each string needs a fixed amount of space in EEPROM with a known start address restriction
