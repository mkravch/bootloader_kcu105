
# Decribtion

Purpose of this project is to write/read firmware on flash by lan connection. 


## How to build a project

For building a project, you have to use the [GNU Make tool](https://www.gnu.org/software/make/). If you're a 
Windows user please checkout [this page](https://wiki.analog.com/resources/fpga/docs/build#windows_environment_setup), to see how you can install this tool.

To build a project just **cd** to the project and run make:
```
 [~]cd projects/bootloader/kcu105
 [~]make
```

## Software

After building the project, run Vivado -> open project -> Launch SDK (exported location - /sw_src; workspace - /sw_src)

In SDK file/new/project -> specify name, next -> empty application -> finish

in system.mss add lwip141

## How to use

1. Make firmware and convert it to bin format.
2. In load_firmware.py specify fileName and run script 



