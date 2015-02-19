# HEX to DFUSE converter

This is a simple Intel-hex to STDfuSe file converter

## Compile
```$ make```

## Usage
```$ DFUFileConverter -i inputfile.hex -o outputfile.dfu```

## TODO
- Pass additional DFU File information via commandline (They are now as defines in src/dfu_write.cpp
- Get actual filesize in src/dfu_write.cpp


