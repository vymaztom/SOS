for file in $(rpm -qa | grep python)
do
	echo "  removing $file"
	sudo yum -y remove $file
done
