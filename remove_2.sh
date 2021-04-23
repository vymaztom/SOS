
function remove {
	for target1 in "$@" ; do
		rm -fv $target1
	done
}

DIR1=$HOME/all_modules.txt
DIR2=$HOME/current_modules.txt
# hledání všech dostupných modulů jádra
find /lib/modules/$(uname -r) -type f -name '*.ko*' > $DIR1
# objevovaní cest modulů jádra
for module in ` lsmod | cut -f1 -d" " | tail -n +2 ` ; do
	filename=`modinfo $module -n`
	echo "$filename " >> $DIR2
done
# seřazení souborů pro vytváření podobné struktury
sort $DIR1 | uniq > file1.sorted
sort $DIR2 | uniq > file2.sorted
# využití funkce remove k vymazání nepoužitých modulů jádra
remove `comm -23 file1.sorted file2.sorted`

# Smazání nepoužívaných firmware
remove `find/usr/lib/firmware-atime +2`

# Smazání záložních obrazů a vygenerování konfiguračního souboru
find /boot -maxdepth 1 -type f -name *rescue* -delete
grub2-mkconfig -o /boot/grub2/grub.cfg
