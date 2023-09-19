export PATH=/media/l/1A62ABBF011B1997/tool/mipsel-mt-linux-gnu/bin:$PATH
make CROSS_TARGET=mips-linux-gnu NOTEST=1 NOPCSC=1 NOCURL=1 NODTAPI=1 static -j8
