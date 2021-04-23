dnf -y install xorg-x11-server-Xorg xinit
dnf -y install wget zip
unzip otter.zip -d /
rm -f otter.zip
dnf -y remove wget zip

dnf -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
dnf -y install qt5-qtwebkit
touch ~/.xinitrc
echo "/otter-browser https://vutbr.cz" >  ~/.xinitrc
