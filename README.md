# bmpflash

A utility for accessing SPI flash devices using the Black Magic Debug Probe.

This project is currently still in early stages and under heavy development.

## Building from source

## Installation

Binary releases are not yet available and the utility must be built from
source.

## Building from source

The project is configured and built using the Meson build system, you will need to create a build directory
and then configure the build depending on what you want.

```sh
meson setup build
```

#### In case of errors during the setup

Some newer versions of meson have changed the syntax of the build script and you may need to modify the script.
Find the line in the file `meson.build` with `'b_debug=if-release'` and change it to `'debug=false'` and try rerunning the setup command.

You now should have a `build` directory from where you can build the app, this is also where your executable will appear.

The command `meson compile` will build the default targets, you may omit `-C build` if you run the command from within the `build` directory:

```sh
meson compile -C build
```

You should now see the resulting executable in `build`:

## Usage

From the help text that's printed with the command `bmpflash -h`:

```
bmpflash - Black Magic Probe companion utility for SPI Flash provisioning and usage

Usage:
	bmpflash [options] {action} [actionOptions]

Options:
	-h, --help           Display this help message and exit
	--version            Display the program version information and exit
	-v, --verbosity UINT Set the program output verbosity

Actions:
	info                 Display information about attached Black Magic Probes
	sfdp                 Display the SFDP (Serial Flash Discoverable Parameters) information for a Flash chip
	provision            Provision a BMP's on-board Flash for use with the auto-programming command in standalone mode
	read                 Read the contents of a Flash chip into the file specified
	write                Write the contents of the file specified into a Flash chip
```

* bmpflash info will give you an overview of what the tool can see
* bmpflash sfdp -b <ext|int> (where <ext|int> intends to convey you need to pick one of "ext" or "int" for the external and internal SPI busses) then gives you the Serial Flash Discoverable Parameters (SFDP) data for any chip found on the bus
* bmpflash read and bmpflash write then additionally take a file name after which should contain (or will be written with, in the case of read) the raw SPI Flash data you want to poke with
* bmpflash provision is for writing a BMP v2.3+'s internal SPI Flash with provisioning data for the upcoming auto-programming/one-touch-programming support

## Hardware connection

In order to communicate with your SPI flash chip, you will need to connect the bmp to your chip. This diagram provides the proper hookups

<img src="https://black-magic.org/_images/unified-legend.svg" width="640" />

Connect as follows:
* PICO - the SPI data input of the flash chip
* POCI - the SPI data output of the flash chip
* SCLK - the SPI clock input of the flash chip
* CS - the chip select input of the flash chip

