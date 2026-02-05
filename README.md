# gbmunge

Munge GenBank files into FASTA sequences and tab-separated metadata.

This little C program will extract the following information from a GenBank file:

- name
- accession
- length
- submission date
- host
- country (supports both `/country` and `/geo_loc_name` qualifiers)
- collection date

In addition to extracting this information, dates are reformatted e.g. `31-DEC-2001` becomes `2001-12-31`, which makes them more digestible to downstream software like BEAST, and country names are cleaned and matched to ISO3 codes.

## Usage

```sh
gbmunge [-h] -i <Genbank_file> -f <sequence_output> -o <metadata_output> [-t] [-s]
```

- `Genbank_file`: filename of GenBank-formatted sequence file (normally downloaded as `sequence.gb`)
- `sequence_output`: filename of FASTA output
- `metadata_output`: filename of tab-separated metadata
- `-t`: flag to
    - only output sequences with collection dates (of any precision)
    - to name sequences as {accession}\_{collection\_date}
- `-s`: flag to include sequences in tab-delimited file

## Building

### Linux and macOS

```sh
git clone https://github.com/sdwfrost/gbmunge
cd gbmunge
make
```

This will build `gbmunge` in the `src/` directory. Add the directory to the path, or move the executable somewhere.

### Windows

There are several options for building on Windows:

1. **Using WSL (Windows Subsystem for Linux)** (Recommended):
   ```sh
   # Install WSL with Ubuntu, then in the WSL terminal:
   sudo apt update
   sudo apt install build-essential
   cd gbmunge
   make
   ```

2. **Using MSYS2/MinGW**:
   ```sh
   # Install MSYS2, then in MSYS2 terminal:
   pacman -S mingw-w64-x86_64-gcc make
   cd gbmunge
   make
   ```

3. **Using Visual Studio with vcpkg**:
   Building natively with MSVC requires a POSIX-compatible regex library:
   ```sh
   # Install PCRE2 via vcpkg
   vcpkg install pcre2:x64-windows
   
   # Compile with PCRE2 support (modify Makefile or compile manually)
   cl /DGBMUNGE_USE_PCRE2 /I<vcpkg_include_path> gbfp.c gbmunge.c /link pcre2-8.lib
   ```

4. **Using TRE regex library**:
   Download TRE from https://github.com/laurikari/tre and place the POSIX-compatible `regex.h` in the `src/` directory.

## Testing

A Genbank file of MERS Coronavirus sequences is provided in the `test/` directory.

```sh
cd test
../src/gbmunge -i sequence.gb -f sequence.fas -o sequence.txt -t
```

Here are the first few lines of output in `sequence.txt`:

**name**|**accession**|**length**|**submission\_date**|**host**|**country\_original**|**country**|**countrycode**|**collection\_date**
-----|-----|-----|-----|-----|-----|-----|-----|-----
JX869059_2012-06-13|JX869059|30119|2012-12-04|Homo sapiens|NA|NA|NA|2012-06-13
KC164505_2012-09-11|KC164505|30111|2013-07-12|Homo sapiens|United Kingdom|United Kingdom|GBR|2012-09-11
KC667074_2012-09-19|KC667074|30112|2013-04-30|Homo sapiens|United Kingdom: England|United Kingdom|GBR|2012-09-19
KC776174_2012-04|KC776174|30030|2013-03-25|Homo sapiens|Jordan|Jordan|JOR|2012-04

## Credits

This code uses a slightly modified version of the [GBParsy](https://link.springer.com/article/10.1186/1471-2105-9-321) parser downloaded from the [Google Code Archive](https://code.google.com/archive/p/gbfp/). I found that the parsing of the LOCUS field wasn't working properly.
