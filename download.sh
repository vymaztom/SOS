dnf -y group install "Development Tools"
git clone https://github.com/OtterBrowser/otter-browser.git
cd otter-browser

dnf -y install qt5-qtbase-devel
dnf -y install qt5-qtmultimedia-devel
dnf -y install qt5-qtsvg-devel
dnf -y install qt5-qtwebkit-devel

mkdir build && cd build && cmake .. && make && make install
