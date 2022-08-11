sod.o: sod_release_118/sod.c
	gcc -c sod_release_118/sod.c


a.out: depth_to_xyz.cpp sod.o
	g++ -fpermissive -o $@ depth_to_xyz.cpp sod.o -Isod_release_118