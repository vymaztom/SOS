# Smazani logu
find /var/log -name "*.log*" -delete
# Smazani souboru mezipameti
rm -rfv /var/cache/*
# Obnoveni databaze rpm
rpm --rebuilddb

# odstranění manuálních stránek a souborů dokumentů
rm -rfv /usr/share/doc/
rm -rfv /usr/share/man/
rm -rfv /usr/share/info/
# rozparsovaní výstupu příkazu locale
system_lang=`locale | cut -f2 -d= | cut -f1 -d. | head -n 1 `
# odstranění nepoužívaných lokalizací
for target in ` find /usr/share/locale -maxdepth 1 -not -name $system_lang * ` ; do
	rm -fv $target/LC_MESSAGES/*
done
