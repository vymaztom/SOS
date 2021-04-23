for file in $(rpm -qa | grep python)
do
  echo "  removing $file"
  rm -f $file
done
