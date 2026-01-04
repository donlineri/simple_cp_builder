CFLAGS = -ggdb -Wall
LFLAGS = -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -lglfw -lfreetype -lcdd
LIBCDD = lib/libcdd.a
simple_cp_builder: simple_cp_builder.cpp glad.o space.o | $(LIBCDD)
	$(CXX) $(CFLAGS) $< glad.o space.o -I/usr/include/freetype2 \
	-I/usr/include/libpng16 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 \
	-I/usr/lib64/glib-2.0/include -I include -L lib -o $@ $(LFLAGS)
dontdelete: simple_cp_builder.cpp glad.o space.o | $(LIBCDD)
	$(CXX) -DDONTDELETE $(CFLAGS) $< glad.o space.o -I/usr/include/freetype2 \
	-I/usr/include/libpng16 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 \
	-I/usr/lib64/glib-2.0/include -I include -L lib -o simple_cp_builder $(LFLAGS)
glad.o: glad.c include/glad/glad.h
	$(CC) $(CFLAGS) -I include -c $< -o $@
space.o: space.cpp include/space.hpp
	$(CXX) $(CFLAGS) $< -I/usr/include/freetype2 -I/usr/include/libpng16 \
	-I/usr/include/harfbuzz -I/usr/include/glib-2.0 \
	-I/usr/lib64/glib-2.0/include -I include -L lib -c -o $@ $(LFLAGS)
$(LIBCDD): cddlib-*
	mkdir -p lib
	cd $^ && ./bootstrap && ./configure
	$(MAKE) -C $^
	find ./$^/lib-src/ -name 'libcdd.a' -exec cp {} lib/ \;
	find ./$^/lib-src/ -name '*.h' -exec cp {} include/ \;
clean:
	$(MAKE) -C cddlib-* clean
	rm -f glad.o space.o simple_cp_builder
	rm -f lib/libcdd.a
	cd include && rm -f cddtypes.h setoper.h cddmp_f.h cdd.h cddtypes_f.h \
		cddmp.h cdd_f.h splitmix64.h
