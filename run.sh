source /opt/esp-idf/export.sh

idf.py build

for device in $(ls /dev | grep ttyUSB); do
    idf.py -p /dev/$device flash
done

if [[ "$1" == "monitor" ]]; then
    if [ "$2" ]; then
        idf.py -p $2 monitor
    else
        ls /dev | grep ttyUSB
        printf "Specify a device: "
        read -r device
        idf.py -p /dev/$device monitor
    fi
fi