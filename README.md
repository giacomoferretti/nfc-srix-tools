# NFC SRIX Tools
A repository containing tools to read/write NFC ST SRI512 and SRIX4K tags.

## TODO
* Add SRI512 support

## Build
You can use the provided `build.sh` or do these simple steps:
```shell script
mkdir build
cd build
cmake ..
make
```

## Usage
```text
Usage: ./nfc-srix-read [-h] [-v] [-a] [-s] [-u] [-r] [-t x4k|512] [-o dump.bin]

Options:
  -h           Shows this help message
  -v           Enables verbose - print debugging data
  -a           Enables -s and -u flags together
  -s           Prints system block
  -u           Prints UID
  -r           Fix read direction
  -t x4k|512   Select SRIX4K or SRI512 tag type [default: x4k]
  -o dump.bin  Dump EEPROM to file
```
