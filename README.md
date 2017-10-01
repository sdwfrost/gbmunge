# gbmunge

Munge GenBank files into FASTA and tab-separated metadata

## Usage

```sh
gbmunge [-h] -i <Genbank_file> -f <sequence_output> -o <metadata_output> [-t]
```

- `Genbank_file`: filename of GenBank-formatted sequence file (normally downloaded as `sequence.gb`)
- `sequence_output`: filename of FASTA output
- `metadata_output`: filename of tab-separated metadata
- `-t`: flag to
    - only output sequences with collection dates
    - to name sequences as {accession}\_{collection\_date}

## Building

```sh
git clone https://github.com/sdwfrost/gbmunge
cd gbmunge
make
```

This will build `gbmunge` in the `src/` directory. Add the directory to the path, or move the executable somewhere.