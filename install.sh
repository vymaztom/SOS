dnf -y install qt5-qtbase-devel
dnf -y install qt5-qtmultimedia-devel
dnf -y install qt5-qtsvg-devel
dnf -y install qt5-qtwebkit-devel

cd otter-browser-master
mkdir build
cd build
cmake ..
make
make install
