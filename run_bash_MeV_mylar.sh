#!/bin/bash

for energy in 0.02 0.03 0.05 0.1 0.5 1.0 2.0 3.0 4.0
do
    cd build
    echo "Staring energy ${energy} MeV"
    
    #500 keV
    sed -i -e 28c"/gun/energy ${energy} MeV" Compton_10MeV.mac
    time ./Pol01 Compton_10MeV.mac > output_compton${energy}MeV_100umMylar.txt
    mv compton.root compton_${energy}MeV_100umMylar.root
    echo "Simulation completed for 500 keV"
    cd ..
done
