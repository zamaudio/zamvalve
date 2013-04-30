all:
	g++ -g wdf.cpp -o wdf -lm

clean:
	rm wdf

plot:
	make all
	./wdf > x
	echo 'plot "x" using 1:3 with lines, "x" using 1:2 with lines' | gnuplot --persist
