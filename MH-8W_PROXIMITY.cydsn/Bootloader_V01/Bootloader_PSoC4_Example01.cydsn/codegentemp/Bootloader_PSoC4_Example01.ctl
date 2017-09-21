-- ======================================================================
-- Bootloader_PSoC4_Example01.ctl generated from Bootloader_PSoC4_Example01
-- 07/19/2017 at 16:25
-- This file is auto generated. ANY EDITS YOU MAKE MAY BE LOST WHEN THIS FILE IS REGENERATED!!!
-- ======================================================================

-- TopDesign
-- =============================================================================
-- The following directives assign pins to the locations specific for the
-- CY8CKIT-040 kit.
-- =============================================================================

-- === I2C ===
attribute port_location of \I2C_Slave:scl(0)\ : label is "PORT(1,2)";
attribute port_location of \I2C_Slave:sda(0)\ : label is "PORT(1,3)";

-- === RGB LED ===
attribute port_location of Bootloader_Status(0) : label is "PORT(0,2)"; -- BLUE LED
-- PSoC Clock Editor
-- Directives Editor
-- Analog Device Editor
