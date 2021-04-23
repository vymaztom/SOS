echo "NetworkManager" > /etc/dnf/protected.d/NetworkManager.conf
for line in `rpm -qa --queryformat "%{NAME}\n" ` ; do
	echo "What require package ${line}:"
	mapfile -t output << ( dnf repoquery -installed --queryformat "%{name}" -whatrequires $line )
	echo "${output[@]}"
	if [ ${#output[@]} -eq 0 ]
	then
		echo "Remove package ${line}"
		dnf -y remove $line
	elif [ ${#output[@]} -eq 1 ]
	then
		if [ "${output[0]}" == "$line" ]
		then
			echo "Remove package ${line}"
			dnf -y remove $line
		fi
	fi
done
