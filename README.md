# NFC SRIX Tools
A repository containing tools to read/write NFC ST SRI512 and SRIX4K tags.

## Known issues
* On 32bit machines, it should give an error

## TODOs
* Complete rewrite (I made it a year and never wrote in C before)

## Prerequisites
* [libnfc](https://github.com/nfc-tools/libnfc)

## Build
You can use the provided `build.sh` or follow these simple steps:
```bash
mkdir build
cd build
cmake ..
make
```

## Tools
* `srix-dump` - Dump EEPROM to file
* `srix-read` - Read dump file
* `srix-restore` - Restore dump to tag
* `srix-reset` - Reset OTP bits

## Examples
### srix-dump
Dump to console: `./srix-dump`

Dump to file: `./srix-dump file.bin`

Usage:
```text
Usage: ./srix-dump [dump.bin] [-h] [-v] [-u] [-s] [-a] [-r] [-t x4k|512]

Optional arguments:
  [dump.bin]   dump EEPROM to file

Options:
  -h           show this help message
  -v           enable verbose - print debugging data
  -s           print system block
  -u           print UID
  -a           enable -s and -u flags together
  -r           fix read direction
  -t x4k|512   select SRIX4K or SRI512 tag type [default: x4k]
```

### srix-read
Usage:
```text
Usage: ./srix-read <dump.bin> [-h] [-v] [-c 1|2] [-t x4k|512]

Necessary arguments:
  <dump.bin>   path to the dump file

Options:
  -h           show this help message
  -v           enable verbose - print debugging data
  -c 1|2       erint on one or two columns [default: 1]
  -t x4k|512   select SRIX4K or SRI512 tag type [default: x4k]
```

### srix-restore
Usage:
```text
Usage: ./srix-restore <dump.bin> [-h] [-v] [-t x4k|512]

Necessary arguments:
  <dump.bin>   path to the dump file

Options:
  -h           show this help message
  -v           enable verbose - print debugging data
  -t x4k|512   select SRIX4K or SRI512 tag type [default: x4k]
```

### srix-reset
Usage:
```text
Usage: ./srix-reset [-h] [-v]

Options:
  -h           show this help message
  -v           enable verbose - print debugging data
```