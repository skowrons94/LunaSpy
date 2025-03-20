# Descritpion

Simple C++ code to spy on the acquisition of LunaDAQ. It obtains the data in parallel exploiting the spy ports that XDAQ creates and creates a TSocket (i.e. ROOT socket) that then spreads the TH1D objects to the clients. Additionally, the software tries to connect to the Graphite database located on ```lunaserver``` to send the data statistics for each channel, as the event rate, the pile-up rate and the dead time.

# Requirements

ROOT and Boost libraries are required. The Boost libraries should already be available if ROOT is correctly installed.

# Build / Installation

To build and install the repository:

```bash
git clone https://github.com/skowrons94/LunaSpy.git
cd RUReader
mkdir build 
cd build/
cmake ..
make 
sudo mv RUSpy /usr/local/bin/RUSpy
```

## Usage
Example usage:

```bash
build/RUSpy -d V1724 0 8 -n 1
```

where ```V1724``` is the board name, ```0``` is the board software (0 for DPP-PHA and 1 for DPP-PSD), ```8``` is the number of channels and ```-n 1``` is the run number. The CAEN boards must be listed in order of its ID in the chain.
