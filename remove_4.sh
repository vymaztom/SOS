for file in $(rpm -qa | grep $1)
do
	echo "  removing $file"
	sudo yum -y remove $file
done
