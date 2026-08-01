# Bluetooth DMA

## Status

Implementing

## Context

When the HM10 Bluetooth module is added to the system it will need a way to quickly transfer data without taking excessive CPU time.
To do this, DMA (Direct Memory Access) will be used to transfer the bytes directly to memory.

## How to implement

In the STM32CubeMX tool, enable DMA with USART3, connected to the HM10 module. Use the callback for idle, full, or complete.
Process the word using the `app_comm` module
