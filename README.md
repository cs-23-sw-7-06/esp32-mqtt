# mqtt-esp32
## Dependencies
Arch Linux
```sh
# using yay
yay -S esp-idf
#######################
# the supported way
git clone https://aur.archlinux.org/packages/esp-idf.git
cd esp-idf
makepkg -si
```

Other Linux:

Get the dependency using wget, and change any references in `run.sh` or `c_cpp_properties` to the path of this dependency
```sh
# replace with any other release
wget https://github.com/espressif/esp-idf/releases/download/v5.1.1/esp-idf-v5.1.1.zip
unzip esp-idf-v5.1.1.zip
```

## Configuration
```sh
source /opt/esp-idf/export.sh # replace with your esp-idf installation path
idf.py menuconfig
```
The configuration contains some custom entries defined in `main/Kconfig.projbuild`. some of the variables defined in here are placeholders and have to be defined before this project can run.

## Usage
A wrapper script `run.sh` is defined, it builds the project, finds all `ttyUSB#` devices in `/dev` and flashes them, and optionally monitors one of them. This is useful to deploy the software to a fleet of devices in one go
```sh
bash run.sh
