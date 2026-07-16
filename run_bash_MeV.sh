#!/bin/bash

for energy in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 25 30 35 40 45 50 55 60 65 
do
    
    echo "Staring energy ${energy} keV"
    cd src
    sed -i -e 103c"\ \ fParticleGun->SetParticleEnergy(${energy}*keV);" PrimaryGeneratorAction.cc
    cd ..

    cd build-detector
    make -j 24
    time ./Pol01 xray_betatron.mac > output_compton${energy}keV.txt
    mv compton.root compton_${energy}keV_detector.root
    echo "Simulation completed for ${energy} KeV"
    cd ..
done
