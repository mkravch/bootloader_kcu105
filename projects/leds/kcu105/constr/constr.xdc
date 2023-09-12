################################################################################
#   Clocks
################################################################################
# System clock 300 MHz
create_clock -name clk_300_mhz -period 3.333 [get_ports clk_300_p_i]

################################################################################
# System Clock 300 MHz LVDS (ug917, p.31, Table 1-8)
################################################################################
set_property PACKAGE_PIN AK17 [get_ports clk_300_p_i]
set_property IOSTANDARD LVDS [get_ports clk_300_p_i]
set_property PACKAGE_PIN AK16 [get_ports clk_300_n_i]
set_property IOSTANDARD LVDS [get_ports clk_300_n_i]

################################################################################
# LEDS  User Pushbuttons p.65
################################################################################
set_property PACKAGE_PIN P23 [get_ports {leds_o[7]}]
set_property PACKAGE_PIN R23 [get_ports {leds_o[6]}]
set_property PACKAGE_PIN M22 [get_ports {leds_o[5]}]
set_property PACKAGE_PIN N22 [get_ports {leds_o[4]}]
set_property PACKAGE_PIN P21 [get_ports {leds_o[3]}]
set_property PACKAGE_PIN P20 [get_ports {leds_o[2]}]
set_property PACKAGE_PIN H23 [get_ports {leds_o[1]}]
set_property PACKAGE_PIN AP8 [get_ports {leds_o[0]}]
set_property IOSTANDARD  LVCMOS18 [get_ports {leds_o[*]}]


set_property BITSTREAM.CONFIG.SPI_32BIT_ADDR YES [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]