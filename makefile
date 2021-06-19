all : src/find.c bin/
	gcc src/find.c -o bin/find

bin/ :
	mkdir bin/

testdir/ :
	./create_test_dir.sh

clean : bin/ testdir/
	rm -r bin/
	rm -r testdir/