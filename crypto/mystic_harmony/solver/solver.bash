while true
do
	echo "connect to server..."
	python3 get.py > output.py
	echo "solving..."
	if sage solver.sage; then
		exit
	fi
done
