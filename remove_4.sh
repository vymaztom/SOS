for file in $(rpm -qa | grep python)
do
	echo "  removing $file"
	sudo yum -y remove $file
done

for file in $(rpm -qa | grep ssh)
do
	echo "  removing $file"
	sudo yum -y remove $file
done

for file in $(rpm -qa | grep vim)
do
	echo "  removing $file"
	sudo yum -y remove $file
done
